// fc_bounding_box.cpp
#include "fc_bounding_box.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"


//
namespace fc
{
  void FcBoundaryBox::init(FcBounds &bounds)
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



  void FcBoundingBoxRenderer::buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout)
  {
    FcPipelineConfig pipelineConfig{3};
    pipelineConfig.name = "Bounding Box Draw";
    pipelineConfig.shaders[0].filename = "bounding_box.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "bounding_box.geom.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
    pipelineConfig.shaders[2].filename = "bounding_box.frag.spv";
    pipelineConfig.shaders[2].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add push constants
    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BoundingBoxPushes);

    pipelineConfig.addPushConstants(pushConstantRange);

    // TODO limit this to what the bounding box pipeline actually needs!!!
    // !! figure out the stages of what all pipelines need so we can compare and
    // add appropriate descriptor sets without including unecessary data
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);

    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    // ?? Would be better to implement with line primitives but not sure if all implementations
    // can use lines... triangles are pretty much guaranteed
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.disableBlending();

    mBoundingBoxPipeline.create(pipelineConfig);
  }


  // TODO think about  embedding command buffer within current frame
  void FcBoundingBoxRenderer::draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection
                                   ,FrameAssets& currentFrame, int boundingBoxID)
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
          for (const FcSurface& surface : materialCollection.second)
          {
            // Send the bounding box to the shaders
            BoundingBoxPushes pushConstants;
            pushConstants.modelMatrix = surface.ModelMatrix();
            pushConstants.origin = glm::vec4(surface.Bounds().origin, 1.f);
            pushConstants.extents = glm::vec4(surface.Bounds().extents, 0.f);

            // ?? not sure that we should be using pushConstants for each draw command like this
            vkCmdPushConstants(cmd, mBoundingBoxPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                               , 0, sizeof(BoundingBoxPushes), &pushConstants);

            // TODO update to utilize sascha method for quads
            vkCmdDraw(cmd, 36, 1, 0, 0);
          }
        }
      }
      // TODO incorporate this code into the above to avoid duplication
      else // otherwise, just draw the object that we are told to
      {
        const FcSurface& surface = drawCollection.getSurfaceAtIndex(boundingBoxID);

        // Send the bounding box to the shaders
        BoundingBoxPushes pushConstants;
        pushConstants.modelMatrix = surface.ModelMatrix();
        pushConstants.origin = glm::vec4(surface.Bounds().origin, 1.f);
        pushConstants.extents = glm::vec4(surface.Bounds().extents, 0.f);

        vkCmdPushConstants(cmd, mBoundingBoxPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                           , 0, sizeof(BoundingBoxPushes), &pushConstants);

        // TODO update to utilize sascha method for quads
        vkCmdDraw(cmd, 36, 1, 0, 0);
      }
    }
  }

  //
  //
  void FcBoundingBoxRenderer::destroy()
  {
    mBoundingBoxPipeline.destroy();
  }

}// --- namespace fc --- (END)
