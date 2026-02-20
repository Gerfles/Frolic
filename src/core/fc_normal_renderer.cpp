//> fc_normal_renderer.cpp <//
#include "fc_normal_renderer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
/* #include "fc_node.hpp" */
/* #include "fc_draw_collection.hpp" */
#include "fc_mesh.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  void FcNormalRenderer::buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout)
  {
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "Normal Draw Pipeline";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "normal_display.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_GEOMETRY_BIT, "normal_display.geom.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "normal_display.frag.spv");

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(ScenePushConstants);

    pipelineConfig.addPushConstants(matrixRange);
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);
    /* pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT); */
    /* pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT); */
    /* pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST); */
    /* pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL); */
    // TODO front face
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    /* pipelineConfig.enableMultiSampling(VK_SAMPLE_COUNT_1_BIT); */
    pipelineConfig.disableMultiSampling();
    // TODO prefer config via:
    //pipelineConfig.enableMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    //pipelineConfig.disableMultiSampling();

    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.enableBlendingAlpha();

    mNormalDrawPipeline.create(pipelineConfig);
  }


  // TODO need to implement a method that only draws the visible objects and
  void FcNormalRenderer::draw(VkCommandBuffer cmd,
                              FcDrawCollection& drawCollection,
                              FrameAssets& currentFrame)
  {
    mNormalDrawPipeline.bind(cmd);
    mNormalDrawPipeline.bindDescriptorSet(cmd, currentFrame.sceneDataDescriptorSet, 0);

    // TODO could also draw the vectors for transparent objects but bypassed here
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


  void FcNormalRenderer::destroy()
  {
    mNormalDrawPipeline.destroy();
  }

}// --- namespace fc --- (END)
