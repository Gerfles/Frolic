#include "shadow_map.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_descriptors.hpp"
#include "fc_locator.hpp"
#include "fc_renderer.hpp"
#include "utilities.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

namespace fc
{

  void FcShadowMap::init(FcRenderer* renderer)
  {
    pRenderer = renderer;
    // -*-*-*-*-*-*-*-*-*-*-*-*-   CREATE SHADOW MAP IMAGE   -*-*-*-*-*-*-*-*-*-*-*-*- //
    // TODO this image should be device local only since no need to map for CPU... check that's the case
    // on this and all other images created with vma allocation
    VkImageUsageFlags imgUse{};

    depthFormat = VK_FORMAT_D32_SFLOAT;

    imgUse = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    mShadowMapImage.create(VkExtent3D{shadowMapSize, shadowMapSize, 1}, depthFormat, imgUse
                           , VK_IMAGE_ASPECT_DEPTH_BIT, VK_SAMPLE_COUNT_1_BIT);
    createSampler();
    initPipelines();

    // TODO allow debug mode to calculate the frustrum as close to the scene
    // as possible
    // // TODO create default up, center, etc. vectors for every time they are needed

    mLightView = glm::lookAt(glm::vec3(2.0f, 14.0f, 2.0f)
                             , glm::vec3(0.0f, 0.0f, 0.0f)
                             , glm::vec3(0.0f, 1.0f, 1.0f));
    mFrustum.setAll(-15.f, 15.f, -15.f, 15.f, .1f, 15.f);

    updateLightSpaceTransform();
  }


  void FcShadowMap::initPipelines()
  {
    // TODO have addStage instead of size initialization or keep as optional but add checks for segfaults
    FcPipelineConfig pipelineConfig{2};
    pipelineConfig.name = "Shadow pipeline";
    pipelineConfig.shaders[0].filename = "shadow_map.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "shadow_map.frag.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(ShadowPushConstants);

    pipelineConfig.addPushConstants(matrixRange);

    //
    pipelineConfig.setDepthFormat(depthFormat);
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineConfig.setMultiSampling(VK_SAMPLE_COUNT_1_BIT);
//    pipelineConfig.disableDepthtest();
    pipelineConfig.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.disableBlending();

    // Create the pipeline used to generate the shadow map
    mShadowPipeline.create(pipelineConfig);

    // TODO delete parts of redundant pipeline config -> just have one config
    // Create the debug pipeline (used to draw the shadow map to a flat quad)
    FcPipelineConfig debugPipelineConfig{2};
    debugPipelineConfig.name = "Shadow Draw Debug";
    // TODO don't require the spv extension or shader extension (try instead)
    debugPipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    debugPipelineConfig.shaders[0].filename = "shadow_map_display.vert.spv";
    debugPipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
    debugPipelineConfig.shaders[1].filename = "shadow_map_display.frag.spv";

    // add push constants
    VkPushConstantRange pushRange;
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(ShadowPushConstants);

    debugPipelineConfig.addPushConstants(pushRange);

    //
    debugPipelineConfig.setDepthFormat(depthFormat);
    debugPipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    debugPipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    debugPipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    debugPipelineConfig.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    debugPipelineConfig.setMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    debugPipelineConfig.disableDepthtest();

    debugPipelineConfig.disableBlending();


    FcDescriptorBindInfo bindInfo{};
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mShadowMapImage
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mShadowSampler);

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    mShadowMapDescriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);
    mShadowMapDescriptorSet = descClerk.createDescriptorSet(mShadowMapDescriptorLayout, bindInfo);

    //attachMap();
    // Now create the descriptor set layout used by opaque pipeline and draw debug
    // TODO streamline this process:
    // 1. try to reduce redundancy for addBinding and attachImage...
    // 2. pass only the image -> the attachImage method should determine the rest...
    // 3.
    // FcDescriptorBindInfo bindInfo{};
    // bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    // // bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mShadowMapImage
    // //                      , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mShadowSampler);

    // FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    // mShadowMapDescriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);
    //mShadowMapDescriptorSet = descClerk.createDescriptorSet(mShadowMapDescriptorLayout, bindInfo);

    debugPipelineConfig.addDescriptorSetLayout(mShadowMapDescriptorLayout);

    mShadowDebugPipeline.create(debugPipelineConfig);
  }



  void FcShadowMap::createSampler()
  {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // How to render when image is magnified on the screen
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    // How to render when image is minified on the screen
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    // How to handle wrap in the U (x) direction
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    // How to handle wrap in the V (y) direction
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    // How to handle wrap in the W (z) direction
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    // Border beyond texture (when clamp to border is used--good for shadow maps)
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    // WILL USE NORMALIZED COORD. (coords will be between 0-1)
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // Mipmap interpolation mode (between two levels of mipmaps)
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // used to force vulkan to use lower level of detail and mip level
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;

    // maximum level of detail to pick mip level
    samplerInfo.maxLod = 1.0f;
    // enable anisotropy
    samplerInfo.anisotropyEnable = VK_FALSE;
    // TODO should allow this to be user definable or at least profiled at install/runtime
    // Amount of anisotropic samples being taken
    samplerInfo.maxAnisotropy = VK_SAMPLE_COUNT_1_BIT;


    FcGpu& gpu = FcLocator::Gpu();
    if (vkCreateSampler(gpu.getVkDevice(), &samplerInfo, nullptr, &mShadowSampler) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }
  }


  void FcShadowMap::updateLightSource(glm::vec3 lightPos, glm::vec3 target)
  {
    // TODO find a strategy to account for the fact that the up vector may be pointing in the x dir, etc.
    mLightView = glm::lookAt(lightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));

    updateLightSpaceTransform();
  }


  void FcShadowMap::updateLightSpaceTransform()
  {
    const glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f,-1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.5f, 0.0f,
                                     0.0f, 0.0f, 0.5f, 1.0f);

    mLightProjection = orthographic(mFrustum.left, mFrustum.right, mFrustum.bottom
                                    , mFrustum.top, mFrustum.far, mFrustum.near);

    mLightSpaceTransform =  clip * mLightProjection * mLightView;
  }


  void FcShadowMap::generateMap(VkCommandBuffer cmd, DrawContext& drawContext)
  {
    mShadowMapImage.transitionImage(cmd, VK_IMAGE_LAYOUT_UNDEFINED
                                    , VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    // TODO extrapolate other
    VkRenderingAttachmentInfo shadowMapAttachment{};
    shadowMapAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    shadowMapAttachment.imageView = mShadowMapImage.ImageView();
    shadowMapAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    shadowMapAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadowMapAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadowMapAttachment.clearValue.depthStencil = {0.0f, 0};

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    VkExtent2D mapExtent{mShadowMapImage.getExtent().width, mShadowMapImage.getExtent().height};
    //std::cout << "Shadow Map Resolution: " << mapExtent.width << " x " << mapExtent.height;
    renderInfo.renderArea = VkRect2D{VkOffset2D{0,0}, mapExtent};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 0;
    renderInfo.pDepthAttachment = &shadowMapAttachment;

    vkCmdBeginRendering(cmd, &renderInfo);

    VkViewport shadowViewport{};
    shadowViewport.x = 0;
    shadowViewport.y = 0;
    shadowViewport.width = mapExtent.width;
    shadowViewport.height = mapExtent.height;
    shadowViewport.minDepth = 0.f;
    // NOTE: this is essential! Otherwise maxDepth defaults to 1.0 and gl_FragCoord.z = 0
    shadowViewport.maxDepth = 1.0f;

    VkRect2D shadowScissors{};
    shadowScissors.offset = {0,0};
    shadowScissors.extent = mapExtent;

    mShadowPipeline.bind(cmd);

    vkCmdSetViewport(cmd, 0, 1, &shadowViewport);
    vkCmdSetScissor(cmd, 0, 1, &shadowScissors);

    ShadowPushConstants shadowPCs;
    shadowPCs.lightSpaceMatrix = mLightSpaceTransform;

    for (RenderObject& surface : drawContext.opaqueSurfaces)
    {
      shadowPCs.vertexBuffer = surface.vertexBufferAddress;
      shadowPCs.modelMatrix = surface.transform;

      vkCmdPushConstants(cmd, mShadowPipeline.Layout()
                         , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConstants), &shadowPCs);

      surface.bindIndexBuffer(cmd);
      vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);
    }

    vkCmdEndRendering(cmd);



    mShadowMapImage.transitionImage(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                                    , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    // colorImage.transitionImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //                            VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR, VK_IMAGE_ASPECT_COLOR_BIT);

    // not necessary but should allow tile-based gpus a speed boost
    // https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering_local_read/README.html
    VkMemoryBarrier2 memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    memoryBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;


    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &memoryBarrier;


    vkCmdPipelineBarrier2(cmd, &dependencyInfo);



    // NOTE: Here we are using the default VK_IMAGE_ASPECT_COLOR_BIT instead of passing VK_IMAGE_ASPECT_DEPTH_BIT
    // mShadowMapImage.transitionImage(cmd, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
    //                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    // mShadowMapImage.transitionImage(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    //                                 , VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR, VK_IMAGE_ASPECT_DEPTH_BIT);

    //pRenderer->submitCommandBuffer();

    // VkCommandBuffer cmd2 = pRenderer->beginCommandBuffer();
    // mShadowMapImage.transitionImage(cmd2, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    //                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // pRenderer->submitCommandBuffer();
  }

  // // TODO eliminate if possible
  // void FcShadowMap::attachMap()
  // {
  //   FcDescriptorBindInfo bindInfo{};
  //   bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);
  //   bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, colorImage
  //                        , VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR, mShadowSampler);

  //   FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
  //   mShadowMapDescriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);
  //   mShadowMapDescriptorSet = descClerk.createDescriptorSet(mShadowMapDescriptorLayout, bindInfo);
  // }


  void FcShadowMap::drawDebugMap(VkCommandBuffer cmd)
  {
    // mShadowMapImage.transitionImage(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    //                                 , VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR, VK_IMAGE_ASPECT_DEPTH_BIT);

    // colorImage.transitionImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //                            VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR, VK_IMAGE_ASPECT_COLOR_BIT);

    // not necessary but should allow tile-based gpus a speed boost
    // https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering_local_read/README.html
    VkMemoryBarrier2 memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    memoryBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &memoryBarrier;

    vkCmdPipelineBarrier2(cmd, &dependencyInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowDebugPipeline.getVkPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowDebugPipeline.Layout()
                            , 0, 1, &mShadowMapDescriptorSet, 0, nullptr);

    ShadowPushConstants push;
    // Display shadow map does not use model matrix so we use two of the elements of matrix to send znear and zfar
    push.modelMatrix = glm::mat4{1.0f};
    push.modelMatrix[0][0] = mFrustum.near;
    push.modelMatrix[1][1] = mFrustum.far;
    push.lightSpaceMatrix = mLightSpaceTransform;

    vkCmdPushConstants(cmd, mShadowDebugPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(ShadowPushConstants), &push);

    vkCmdDraw(cmd, 3, 1, 0, 0);
  }

}
