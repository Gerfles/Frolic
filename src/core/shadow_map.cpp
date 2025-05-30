#include "shadow_map.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_descriptors.hpp"
#include "fc_locator.hpp"
#include "fc_renderer.hpp"
#include "fc_defaults.hpp"
#include "utilities.hpp"
#include "fc_model.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

namespace fc
{

  void FcShadowMap::init(std::vector<FrameAssets>& frames)
  {
    depthFormat = VK_FORMAT_D32_SFLOAT;
    // -*-*-*-*-*-*-*-*-*-*-*-*-   CREATE SHADOW MAP IMAGE   -*-*-*-*-*-*-*-*-*-*-*-*- //
    // TODO this image should be device local only since no need to map for CPU... check that's the case
    // on this and all other images created with vma allocation

    mShadowMapImage.create(shadowMapSize, shadowMapSize, FcImageTypes::ShadowMap);

    initPipelines(frames);

    // TODO allow debug mode to calculate the frustrum as close to the scene
    // as possible
    // // TODO create default up, center, etc. vectors for every time they are needed

    // mLightView = glm::lookAt(glm::vec3(47.0f, 25.0f, 23.0f)
    // /* mLightView = glm::lookAt(glm::vec3(2.0f, 14.0f, 2.0f) */
    //                          , glm::vec3(45.0f, 8.0f, 20.0f)
    //                          , glm::vec3(0.0f, 0.0f, 1.0f));
    mFrustum.setAll(-15.f, 15.f, -15.f, 15.f, .1f, 15.f);

    updateLightSpaceTransform();
  }


  void FcShadowMap::initPipelines(std::vector<FrameAssets>& frames)
  {
    // TODO have addStage instead of size initialization or keep as optional but add
    // checks for segfaults and also create better default values for pipeline
    // configurations
    FcPipelineConfig shadowPipeline{2};
    shadowPipeline.name = "Shadow pipeline";
    shadowPipeline.shaders[0].filename = "shadow_map.vert.spv";
    shadowPipeline.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    shadowPipeline.shaders[1].filename = "shadow_map.frag.spv";
    shadowPipeline.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(ShadowPushConsts);

    shadowPipeline.addPushConstants(matrixRange);
    //
    shadowPipeline.setDepthFormat(depthFormat);
    shadowPipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    shadowPipeline.setPolygonMode(VK_POLYGON_MODE_FILL);
    shadowPipeline.setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    shadowPipeline.setMultiSampling(VK_SAMPLE_COUNT_1_BIT);

    // shadowPipeline.disableDepthtest();
    shadowPipeline.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    shadowPipeline.disableBlending();

    // Create the pipeline used to generate the shadow map
    mShadowPipeline.create(shadowPipeline);

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
    pushRange.size = sizeof(ShadowPushConsts);
    //
    debugPipelineConfig.addPushConstants(pushRange);

    // Configure pipeline
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

    // Now create the descriptor set layout used by opaque pipeline and draw debug
    // TODO streamline this process:
    // 1. try to reduce redundancy for addBinding and attachImage...
    // 2. pass only the image -> the attachImage method should determine the rest...
    // 3.

    VkDescriptorSetLayout descriptorSetLayout;

    // TODO make Layout a temp object instead of class object
    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    descriptorSetLayout = descClerk.createDescriptorSetLayout(bindInfo);

    bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mShadowMapImage
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                         , FcDefaults::Samplers.ShadowMap);



    debugPipelineConfig.addDescriptorSetLayout(descriptorSetLayout);

    mShadowDebugPipeline.create(debugPipelineConfig);


    // Allocate a descriptorSet to each frame buffer
    for (FrameAssets& frame : frames)
    {
      // TODO May be able to then remove descriptor set from shadowMap class
      frame.shadowMapDescriptorSet = descClerk.createDescriptorSet(descriptorSetLayout,
                                                                   bindInfo);
    }
  }


  // TODO eliminate confusion about which of the two following functions to call
  void FcShadowMap::updateLightSource(glm::vec3 lightPos, glm::vec3 target)
  {
    // TODO find a strategy to account for the fact that the up vector may be pointing in the x dir, etc.
    /* lightPos = glm::normalize(lightPos); */
    mLightView = glm::lookAt(lightPos,target, glm::vec3(0.0f, 0.0f, -1.0f));

    updateLightSpaceTransform();
  }


  void FcShadowMap::updateLightSpaceTransform()
  {
    mLightProjection = orthographic(mFrustum.left, mFrustum.right, mFrustum.bottom
                                    , mFrustum.top, mFrustum.far, mFrustum.near);

    mLightSpaceTransform =  mLightProjection * mLightView;
  }


  void FcShadowMap::generateMap(VkCommandBuffer cmd, FcDrawCollection& drawContext)
  {
    mShadowMapImage.transitionLayout(cmd,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_ASPECT_DEPTH_BIT);
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
    VkExtent2D mapExtent{mShadowMapImage.Width(), mShadowMapImage.Height()};
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
    // NOTE: setting maxDepth is essential! (source of painful error)
    // Otherwise maxDepth defaults to 0 and gl_FragCoord.z = 0
    shadowViewport.maxDepth = 1.0f;

    VkRect2D shadowScissors{};
    shadowScissors.offset = {0,0};
    shadowScissors.extent = mapExtent;

    mShadowPipeline.bind(cmd);

    vkCmdSetViewport(cmd, 0, 1, &shadowViewport);
    vkCmdSetScissor(cmd, 0, 1, &shadowScissors);

    /* shadowPCs.MVP = mLightSpaceTransform * sur; */

    for (auto& materialCollection : drawContext.opaqueSurfaces)
    {
      for (const FcSurface& surface : materialCollection.second)
      {
        ShadowPushConsts shadowPCs;
        shadowPCs.vertexBuffer = surface.vertexBufferAddress;
        shadowPCs.MVP = mLightSpaceTransform * surface.transform;
        /* shadowPCs.modelMatrix = surface.transform; */

        vkCmdPushConstants(cmd, mShadowPipeline.Layout()
                           , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConsts), &shadowPCs);

        surface.bindIndexBuffer(cmd);

        vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);
      }
    }

    vkCmdEndRendering(cmd);

    mShadowMapImage.transitionLayout(cmd,
                                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_IMAGE_ASPECT_DEPTH_BIT);

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


  void FcShadowMap::drawDebugMap(VkCommandBuffer cmd, FrameAssets& currentFrame)
  {
    // mShadowMapImage.transition(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
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
                            , 0, 1, &currentFrame.shadowMapDescriptorSet, 0, nullptr);

    ShadowPushConsts push;
    // TODO develop own push constants for shadow maps
    // TODO check if data exceeds typical PC limits
    // Display shadow map does not use model matrix so we use two of the elements of matrix to send znear and zfar
    push.MVP = glm::mat4();
    push.MVP[0][0] = mFrustum.near;
    push.MVP[1][1] = mFrustum.far;

    vkCmdPushConstants(cmd, mShadowDebugPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(ShadowPushConsts), &push);

    vkCmdDraw(cmd, 3, 1, 0, 0);
  }

}
