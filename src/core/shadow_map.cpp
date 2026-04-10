//>--- shadow_map.cpp ---<//
#include "shadow_map.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/utilities.hpp"
#include "fc_frame_assets.hpp"
#include "fc_math.hpp"
#include "fc_mesh.hpp"
#include "fc_descriptors.hpp"
#include "fc_defaults.hpp"
#include "fc_locator.hpp"
#include "fc_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/gtc/matrix_transform.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  void FcShadowRenderer::init(FcDescriptorCollection& frame)
  {
    // -*-*-*-*-*-*-*-*-*-*-*-*-   CREATE SHADOW MAP IMAGE   -*-*-*-*-*-*-*-*-*-*-*-*- //
    // TODO this image should be device local only since no need to map for CPU... check that's the case
    // on this and all other images created with vma allocation
    mShadowMapImage.createImage(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, FcImageTypes::ShadowMap);

    populateImageMemoryBarrier(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               mShadowMapImage.getVkImage(),
                               mShadowMapShaderAccessBarrier);

    populateImageMemoryBarrier(VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                               mShadowMapImage.getVkImage(),
                               mShadowMapWriteAccessBarrier);

    mImgTranstionDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    mImgTranstionDependency.imageMemoryBarrierCount = 1;

    initPipelines(frame);

    // TODO allow debug mode to calculate the frustrum as close to the scene
    // as possible
    // // TODO create default up, center, etc. vectors for every time they are needed

    // mLightView = glm::lookAt(glm::vec3(47.0f, 25.0f, 23.0f)
    // /* mLightView = glm::lookAt(glm::vec3(2.0f, 14.0f, 2.0f) */
    //                          , glm::vec3(45.0f, 8.0f, 20.0f)
    //                          , glm::vec3(0.0f, 0.0f, 1.0f));
    mFrustum.setAll(3.5f, 31.f, -20.f, 20.f, -15.f, 10.f);

    updateLightSpaceTransform();
  }


  void FcShadowRenderer::initPipelines(FcDescriptorCollection& frames)
  {
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "Shadow pipeline";
    // TODO don't require the spv extension or shader extension (try instead)
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "shadow_map.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "shadow_map.frag.spv");

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(ShadowPushConsts);

    pipelineConfig.addPushConstants(matrixRange);

    // configure shadow pipeline
    pipelineConfig.disableColorAttachment();
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineConfig.disableMultiSampling();
    pipelineConfig.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.disableBlending();

    // Create the pipeline used to generate the shadow map
    mShadowPipeline.create(pipelineConfig);

    // Now create the debug pipeline (used to draw the shadow map to a flat quad)
    pipelineConfig.reset();
    pipelineConfig.name = "Shadow Draw Debug";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "shadow_map_display.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "shadow_map_display.frag.spv");

    // Configure pipeline
    pipelineConfig.disableColorAttachment();
    pipelineConfig.disableMultiSampling();
    pipelineConfig.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineConfig.disableDepthtest();
    pipelineConfig.disableBlending();

    // Set up descriptor sets
    pipelineConfig.attachImage(0, 0, mShadowMapImage, FcDefaults::Samplers.ShadowMap, VK_SHADER_STAGE_FRAGMENT_BIT);
    mShadowMapDescriptor = pipelineConfig.createDescriptorSet(0);

    // DELETE
    // TODO May be able to then remove descriptor set from shadowMap class
    frames.shadowMapDescriptorSet = mShadowMapDescriptor;

    mShadowDebugPipeline.create(pipelineConfig);
  }


  // TODO eliminate confusion about which of the two following functions to call
  void FcShadowRenderer::updateLightSource(glm::vec3 lightPos, glm::vec3 target)
  {
    // TODO find a strategy to account for the fact that the up vector may be pointing in the x dir, etc.
    /* lightPos = glm::normalize(lightPos); */
    mLightView = glm::lookAt(lightPos,target, glm::vec3(0.0f, 0.0f, -1.0f));

    updateLightSpaceTransform();
  }


  void FcShadowRenderer::updateLightSpaceTransform()
  {
    mLightProjection = orthographic(mFrustum.left, mFrustum.right, mFrustum.bottom
                                    , mFrustum.top, mFrustum.far, mFrustum.near);

    mLightSpaceTransform =  mLightProjection * mLightView;
  }


  // TODO add synchronization to be able to render map within current cmdBuffer
  void FcShadowRenderer::generateMap(FcDrawCollection& drawContext)
  {
    FcCommandBuffer& cmdBuffer = FcLocator::Renderer().beginCommandBuffer();
    VkCommandBuffer cmd = cmdBuffer.getVkCmdBuffer();

    mShadowMapImage.transitionLayoutCached(cmd, mShadowMapWriteAccessBarrier);

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

    /* shadowPCs.MVP = mLightSpaceTransform * sun; */

    for (auto& materialCollection : drawContext.opaqueSurfaces)
    {
      for (const FcSubmesh& submesh : materialCollection.second)
      {
        ShadowPushConsts shadowPCs;
        shadowPCs.vertexBuffer = submesh.node->mVertexBufferAddress;
        shadowPCs.MVP = mLightSpaceTransform * submesh.node->worldTransform;

        vkCmdPushConstants(cmd, mShadowPipeline.Layout()
                           , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConsts), &shadowPCs);

        submesh.parent.lock()->bindIndexBuffer(cmd);

        vkCmdDrawIndexed(cmd, submesh.indexCount, 1, submesh.startIndex, 0, 0);
      }
    }

    vkCmdEndRendering(cmd);

    mShadowMapImage.transitionLayoutCached(cmd, mShadowMapShaderAccessBarrier);

    FcLocator::Renderer().submitCmdBuffer(cmdBuffer);
  }


  void FcShadowRenderer::drawDebugMap(VkCommandBuffer cmd, FcDescriptorCollection& currentFrame)
  {
    // not necessary but should allow tile-based gpus a speed boost
    // https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering_local_read/README.html
    // VkMemoryBarrier2 memoryBarrier = {};
    // memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    // memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    // memoryBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    // memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    // memoryBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

    // VkDependencyInfo dependencyInfo{};
    // dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    // /* dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; */
    // dependencyInfo.memoryBarrierCount = 1;
    // dependencyInfo.pMemoryBarriers = &memoryBarrier;

    // vkCmdPipelineBarrier2(cmd, &dependencyInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      mShadowDebugPipeline.getVkPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowDebugPipeline.Layout()
                            , 0, 1, &mShadowMapDescriptor, 0, nullptr);

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
