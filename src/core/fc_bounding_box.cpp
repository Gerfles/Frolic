//>--- fc_bounding_box.cpp ---<//
#include "fc_bounding_box.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_mesh.hpp"
#include "fc_frame_assets.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  void FcBoundaryBox::init(const FcBounds &bounds) noexcept
  {
    mCorners[0] = glm::vec4{bounds.origin + glm::vec3{1.f, 1.f, 1.f} * bounds.extents, 1.f};
    mCorners[1] = glm::vec4{bounds.origin + glm::vec3{1.f, 1.f, -1.f} * bounds.extents, 1.f};
    mCorners[2] = glm::vec4{bounds.origin + glm::vec3{1.f, -1.f, 1.f} * bounds.extents, 1.f};
    mCorners[3] = glm::vec4{bounds.origin + glm::vec3{1.f, -1.f, -1.f} * bounds.extents, 1.f};
    mCorners[4] = glm::vec4{bounds.origin + glm::vec3{-1.f, 1.f, 1.f} * bounds.extents, 1.f};
    mCorners[5] = glm::vec4{bounds.origin + glm::vec3{-1.f, 1.f, -1.f} * bounds.extents, 1.f};
    mCorners[6] = glm::vec4{bounds.origin + glm::vec3{-1.f, -1.f, 1.f} * bounds.extents, 1.f};
    mCorners[7] = glm::vec4{bounds.origin + glm::vec3{-1.f, -1.f, -1.f} * bounds.extents, 1.f};
  }



  void FcBoundingBoxRenderer::buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout) noexcept
  {
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "Bounding Box Draw";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "bounding_box.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_GEOMETRY_BIT, "bounding_box.geom.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "bounding_box.frag.spv");

    // add push constants
    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BoundingBoxPushes);

    pipelineConfig.addPushConstants(pushConstantRange);

    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);

    // TODO Would be better to implement with line primitives but not sure if all implementations
    // can use lines... triangles are pretty much guaranteed
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.disableMultiSampling();
    pipelineConfig.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.disableBlending();

    mBoundingBoxPipeline.create(pipelineConfig);
  }


  // TODO think about embedding command buffer within current frame
  void FcBoundingBoxRenderer::draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection
                                   ,FrameAssets& currentFrame, int boundingBoxID) noexcept
  {
    {
      mBoundingBoxPipeline.bind(cmd);
      mBoundingBoxPipeline.bindDescriptorSet(cmd, currentFrame.sceneDataDescriptorSet, 0);

      // Draw all bounding boxes, if signaled by BoxId being == -1 (default value)
      // also make sure we don't try and draw a bounding box that doesn't exist
      // TODO reset bounding box to -1 withing calling function then remove this check or
      // find a better way to access from ImGUI
      if (boundingBoxID < 0 || boundingBoxID >= drawCollection.numSurfaces)
      {
        // TODO need to incorporate transparent surfaces!!
        for (auto& materialCollection : drawCollection.opaqueSurfaces)
        {
          for (const FcSubmesh& subMesh : materialCollection.second)
          {
            drawSurface(cmd, subMesh);
          }
        }
      }
      // TODO incorporate this code into the above to avoid duplication
      else // otherwise, just draw the object that we are told to
      {
        const FcSubmesh& subMesh = drawCollection.getSurfaceAtIndex(boundingBoxID);
        drawSurface(cmd, subMesh);
      }
    }
  }

  void  FcBoundingBoxRenderer::drawSurface(VkCommandBuffer cmd, const FcSubmesh& subMesh) noexcept
  {
    // Send the bounding box to the shaders
    BoundingBoxPushes pushConstants;
    pushConstants.modelMatrix = subMesh.node->worldTransform;
    pushConstants.origin = glm::vec4(subMesh.bounds.origin, 1.f);
    pushConstants.extents = glm::vec4(subMesh.bounds.extents, 0.f);

    vkCmdPushConstants(cmd, mBoundingBoxPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                       , 0, sizeof(BoundingBoxPushes), &pushConstants);

    // TODO update to utilize sascha method for quads
    vkCmdDraw(cmd, 36, 1, 0, 0);
  }

  //
  //
  void FcBoundingBoxRenderer::destroy() noexcept
  {
    mBoundingBoxPipeline.destroy();
  }

}// --- namespace fc --- (END)
