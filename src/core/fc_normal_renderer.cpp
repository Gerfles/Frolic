//> fc_normal_renderer.cpp <//
#include "fc_normal_renderer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_descriptors.hpp"
#include "fc_frame_assets.hpp"
#include "fc_types.hpp"
#include "fc_mesh.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  void FcNormalRenderer::init(const FcBuffer& sceneDataBuffer)
  {
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "Normal Draw Pipeline";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "normal_display.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_GEOMETRY_BIT, "normal_display.geom.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "normal_display.frag.spv");

    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.disableMultiSampling();
    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.enableBlendingAlpha();

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(ScenePushConstants);
    pipelineConfig.addPushConstants(matrixRange);

    // Add the scene descriptor set layout (eye, view, proj, etc.)
    pipelineConfig.attachUniformBuffer(0, 0, sceneDataBuffer, sizeof(SceneData), 0,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT);

    // Create the descriptorset we will bind when drawing
    mDescriptorSet = pipelineConfig.createDescriptorSet(0);

    mNormalDrawPipeline.create(pipelineConfig);
  }


  //
  // TODO need to implement a method that only draws the visible objects and
  void FcNormalRenderer::draw(VkCommandBuffer cmd,
                              FcDrawCollection& drawCollection,
                              FcDescriptorCollection& currentFrame)
  {
    mNormalDrawPipeline.bind(cmd);
    mNormalDrawPipeline.bindDescriptorSet(cmd, mDescriptorSet, 0);

    // TODO should also draw the vectors for transparent objects but bypassed here
    for (auto& materialCollection : drawCollection.opaqueSurfaces)
    {
      for (const FcSubmesh& subMesh : materialCollection.second)
      {
        subMesh.parent.lock()->bindIndexBuffer(cmd);

        vkCmdPushConstants(cmd, mNormalDrawPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                           , 0, sizeof(ScenePushConstants), subMesh.getSceneConstantsPtr());

        vkCmdDrawIndexed(cmd, subMesh.indexCount, 1, subMesh.startIndex, 0, 0);
      }
    }
  }


  //
  void FcNormalRenderer::destroy()
  {
    FC_DEBUG_LOG("Destroying normal vector renderer...");
    mNormalDrawPipeline.destroy();
  }

}// --- namespace fc --- (END)
