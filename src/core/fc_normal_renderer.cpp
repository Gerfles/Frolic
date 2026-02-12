// fc_normal_renderer.cpp
#include "fc_normal_renderer.hpp"
/* #include "fc_scene_renderer.hpp" */
#include "fc_draw_collection.hpp"
#include "fc_surface.hpp"

namespace fc
{
  void FcNormalRenderer::buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout)
  {
    FcPipelineConfig pipelineConfig{3};
    pipelineConfig.name = "Normal Draw Pipeline";
    pipelineConfig.shaders[0].filename = "normal_display.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "normal_display.geom.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
    pipelineConfig.shaders[2].filename = "normal_display.frag.spv";
    pipelineConfig.shaders[2].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    // TODO see if we can get rid of this the fc_scene... include
    matrixRange.size = sizeof(ScenePushConstants);

    pipelineConfig.addPushConstants(matrixRange);
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);
    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    // TODO front face
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(VK_SAMPLE_COUNT_1_BIT);
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
        subMesh.parent->bindIndexBuffer(cmd);

        // // TODO update invModelMatrix when updating worldMatrix!!!!
        vkCmdPushConstants(cmd, mNormalDrawPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                           , 0, sizeof(ScenePushConstants), subMesh.parent.get());

        vkCmdDrawIndexed(cmd, subMesh.indexCount, 1, subMesh.startIndex, 0, 0);
      }
    }
  }


  void FcNormalRenderer::destroy()
  {
    mNormalDrawPipeline.destroy();
  }

}// --- namespace fc --- (END)
