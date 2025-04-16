//  fc_materials.cpp

#include "fc_scene_renderer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_renderer.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>


namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-   BOILERPLATE BIT-WISE ENUM OPS   *-*-*-*-*-*-*-*-*-*-*- //
  MaterialFeatures operator| (MaterialFeatures lhs, MaterialFeatures rhs)
  {
    using FeaturesType = std::underlying_type<MaterialFeatures>::type;
    return MaterialFeatures(static_cast<FeaturesType>(lhs) | static_cast<FeaturesType>(rhs));
  }
  MaterialFeatures operator& (MaterialFeatures lhs, MaterialFeatures rhs)
  {
    using FeaturesType = std::underlying_type<fc::MaterialFeatures>::type;
    return MaterialFeatures(static_cast<FeaturesType>(lhs) & static_cast<FeaturesType>(rhs));
  }
  MaterialFeatures& operator|= (MaterialFeatures& lhs, MaterialFeatures const& rhs)
  {
    lhs = lhs | rhs;
    return lhs;
  }
  MaterialFeatures& operator&= (MaterialFeatures& lhs, MaterialFeatures const& rhs)
  {
    lhs = lhs & rhs;
    return lhs;
  }
  MaterialFeatures operator~ (MaterialFeatures const & rhs)
  {
    using FeaturesType = std::underlying_type<MaterialFeatures>::type;
    return MaterialFeatures(~static_cast<FeaturesType>(rhs));
  }
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //



  void FcSceneRenderer::init(std::vector<FrameData>& frames)
  {
    fcLog("ScenRenderer::init()");

    mSceneData.viewProj = glm::mat4(1.f);

    fcLog("SetVIEWPROJ");

    // TODO take advantage of the fact that Descriptor params can be reset once it spits out the SET
    // TODO probably delete from here
    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

    // *-*-*-*-*-*-*-*-*-*-*-*-   FRAME DATA INITIALIZATION   *-*-*-*-*-*-*-*-*-*-*-*- //
    mSceneDataBuffer.allocate(sizeof(SceneDataUbo), FcBufferTypes::Uniform);

    // TODO create temporary storage for this in descClerk so we can just write the
    // descriptorSet and layout on the fly and destroy layout if not needed
    // TODO see if layout is not needed.
    FcDescriptorBindInfo sceneDescriptorBinding{};
    sceneDescriptorBinding.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                      , VK_SHADER_STAGE_VERTEX_BIT
                                      // TODO DELETE after separating model from scene
                                      | VK_SHADER_STAGE_FRAGMENT_BIT
                                      | VK_SHADER_STAGE_GEOMETRY_BIT);


    // TODO find out if there is any cost associated with binding to multiple un-needed stages...
    //, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneDescriptorBinding.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mSceneDataBuffer
                                        , sizeof(SceneDataUbo), 0);

    // create descriptorSet for sceneData
    mSceneDataDescriptorLayout = descClerk.createDescriptorSetLayout(sceneDescriptorBinding);

    // Allocate a descriptorSet to each frame buffer
    for (FrameData& frame : frames)
    {
      frame.sceneDataDescriptorSet = descClerk.createDescriptorSet(
        mSceneDataDescriptorLayout, sceneDescriptorBinding);
    }


  }


  // TODO remove dependency on FcRenderer pointer
  void FcSceneRenderer::buildPipelines(FcRenderer *renderer)
  {
    fcLog("FcSceneRendere::buildPipelines()");
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OPAQUE PIPELINE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // TODO addshader() func
    FcPipelineConfig pipelineConfig{3};
    pipelineConfig.name = "Opaque Pipeline";
    pipelineConfig.shaders[0].filename = "mesh.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "brdf.frag.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineConfig.shaders[2].filename = "explode.geom.spv";
    pipelineConfig.shaders[2].stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;

    // add push constants for the model & normal matrices and address of vertex buffer
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(DrawPushConstants);
    //
    pipelineConfig.addPushConstants(matrixRange);

    // Add push for the amount to expand the polygons
    VkPushConstantRange expansionFactorRange;
    expansionFactorRange.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    expansionFactorRange.offset = sizeof(DrawPushConstants);
    expansionFactorRange.size = sizeof(float);
    //
    pipelineConfig.addPushConstants(expansionFactorRange);

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    // create the descriptor set layout for the material
    FcDescriptorBindInfo bindInfo{};
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        , VK_SHADER_STAGE_FRAGMENT_BIT);
    bindInfo.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        , VK_SHADER_STAGE_FRAGMENT_BIT);
    bindInfo.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        , VK_SHADER_STAGE_FRAGMENT_BIT);
    bindInfo.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        , VK_SHADER_STAGE_FRAGMENT_BIT);
    bindInfo.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        , VK_SHADER_STAGE_FRAGMENT_BIT);
    // TODO check to see if we even need a member variable for the below?? could it be temporary
    mMaterialDescriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);

    // place the scene descriptor layout in the first set (0), then cubemap, then material
    // TODO find a better way to pass these descriptor sets around etc...
    // ?? Are they even needed in here??
    pipelineConfig.addDescriptorSetLayout(mSceneDataDescriptorLayout);
    // Add single image descriptors for sky box image
    pipelineConfig.addSingleImageDescriptorSetLayout();
    // Add single image descriptors for shadow map image
    pipelineConfig.addSingleImageDescriptorSetLayout();
    // Finally add the descriptor set layout for materials
    pipelineConfig.addDescriptorSetLayout(mMaterialDescriptorLayout);

    // TODO find a way to do this systematically with the format of the draw/depth image
    // ... probably by adding a pipeline builder to renderer and calling from frolic
    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    // TODO front face
    pipelineConfig.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);

    // TODO prefer config via:
    //pipelineConfig.enableMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    //pipelineConfig.disableMultiSampling();
    //pipelineConfig.disableBlending();
    //pipelineConfig.enableBlendingAlpha();
    //pipelineConfig.enableBlendingAdditive();
    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // TODO make pipeline and config and descriptors friend classes
    mOpaquePipeline.create(pipelineConfig);

    // *-*-*-*-*-*-*-*-*-*-*-*-*-   TRANSPARENTE PIPELINE   *-*-*-*-*-*-*-*-*-*-*-*-*- //
    // using the same pipeline config, alter slightly for transparent models
    // TODO make sure the transparent pipeline handles things like shadow differently
    pipelineConfig.name = "Transparent Pipeline";
    pipelineConfig.enableBlendingAdditive();
    pipelineConfig.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    mTransparentPipeline.create(pipelineConfig);

    initNormalDrawPipeline();
    initBoundingBoxPipeline();
  }



  void FcSceneRenderer::initNormalDrawPipeline()
  {
    fcLog("FcSceneRenderer::initNormalDrawPipelines()");
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
    matrixRange.size = sizeof(DrawPushConstants);

    pipelineConfig.addPushConstants(matrixRange);
    pipelineConfig.addDescriptorSetLayout(mSceneDataDescriptorLayout);
    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    // TODO front face
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);
    // TODO prefer config via:
    //pipelineConfig.enableMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    //pipelineConfig.disableMultiSampling();

    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    mNormalDrawPipeline.create(pipelineConfig);
  }


  void FcSceneRenderer::initBoundingBoxPipeline()
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
    pushConstantRange.size = sizeof(BoundingBoxPushConstants);

    pipelineConfig.addPushConstants(pushConstantRange);

    pipelineConfig.addDescriptorSetLayout(mSceneDataDescriptorLayout);

    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    // ?? Would be better to implement with line primitives but not sure if all implementations
    // can use lines... triangles are pretty much guaranteed
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);
    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.disableBlending();

    mBoundingBoxPipeline.create(pipelineConfig);
  }


  // sort objects by pipeline and material prior to drawing them
  void FcSceneRenderer::SortByVisibilityAndMaterial(FcDrawCollection& drawCollection)
  {
    // TODO should consider sorting when first adding objects to draw, unless something changes
    // or perhaps just inserting objects into draw via a hashmap. One thing to consider though
    // is that we also perform visibility checks before we sort
    // TODO should also make sure to sort using more than one thread
    // Sort rendered objects according to material type and if the same sorted by indexBuffer
    // A lot of big game engines do this to reduce the number of pipeline/descriptor set binds

    // TODO should only have the the resize happen when we first add object to render then
    // change IFF we add more scene objects
    mSortedObjectIndices.resize(drawCollection.opaqueSurfaces.size());

    // Only place the meshes whose bounding box is within the view frustrum
    for (uint32_t i = 0; i < drawCollection.opaqueSurfaces.size(); i++)
    {
      // BUG the bounding boxes are excluding visible objects for some reason
      // May only be on the sponza gltf...
      // BUG also when no objects are rendered, causes an error in seting the scissors and viewport
      // TODO implement normal arrows and bounding boxes
      // if (mainDrawContext.opaqueSurfaces[i].isVisible(pSceneData->viewProj))
      // {
      //   sortedOpaqueIndices.push_back(i);
      // }

      // BUG For now, just add all objects
      mSortedObjectIndices[i] = i;

    }

    // ?? couldn't we sort drawn meshes into a set of vectors that're already sorted by material
    // and keep drawn object in linked list every iteration (unless removed manually) instead
    // of clearing the draw list every update...

    // TODO sort algorithm could be improved by calculating a sort key, and then our sortedOpaqueIndices
    // would be something like 20bits draw index and 44 bits for sort key/hash
    // sort the opaque surfaces by material and mesh
    std::sort(mSortedObjectIndices.begin(), mSortedObjectIndices.end(), [&](const auto& iA, const auto& iB)
     {
       const FcRenderObject& A = drawCollection.opaqueSurfaces[iA];
       const FcRenderObject& B = drawCollection.opaqueSurfaces[iB];
       if (A.material == B.material)
       {
         return A.indexBuffer < B.indexBuffer;
       }
       else
       {
         return A.material < B.material;
       }
     });
  }




  void FcSceneRenderer::draw(VkCommandBuffer cmd
                             , FcDrawCollection& drawCollection, FrameData& currentFrame)
  {
    SortByVisibilityAndMaterial(drawCollection);

    // Reset the previously used draw instruments for the new draw call
    // defined outside of the draw function, this is the state we will try to skip
    mPreviousPipeline = nullptr;
    mPreviousMaterial = nullptr;
    mPreviousIndexBuffer = VK_NULL_HANDLE;

    // // TODO delete above but follow the sorted version (add sorting, etc.)
    // for (FcRenderObject& surface : drawCollection.opaqueSurfaces)
    // {
    //   drawSurface(cmd, surface);
    // }

    // TODO sort once and store the sorted indices
    // First draw the opaque objects using the captured Lambda
    for (uint32_t surfaceIndex : mSortedObjectIndices)
    {
      drawSurface(cmd, drawCollection.opaqueSurfaces[surfaceIndex], currentFrame);
    }

    // Afterwards, we can draw the transparent ones using the captured Lambda
    for (FcRenderObject& surface : drawCollection.transparentSurfaces)
    {
      drawSurface(cmd, surface, currentFrame);
    }
  }



  void FcSceneRenderer::drawSurface(VkCommandBuffer cmd
                                    , const FcRenderObject& surface, FrameData& currentFrame)
  {
    if (surface.material != mPreviousMaterial)
    {
      mPreviousMaterial = surface.material;

      // Only rebind pipeline and material descriptors if the material changed
      // TODO have each object track state of its own descriptorSets
      // There are only two pipelines so far so should just draw all opaque, then all transparent
      if (surface.material->pPipeline != mPreviousPipeline)
      {
        mPreviousPipeline = surface.material->pPipeline;
        surface.bindPipeline(cmd);
        surface.bindDescriptorSet(cmd, currentFrame.sceneDataDescriptorSet, 0);
        surface.bindDescriptorSet(cmd, currentFrame.skyBoxDescriptorSet, 1);
        surface.bindDescriptorSet(cmd, currentFrame.shadowMapDescriptorSet, 2);
      }

      surface.bindDescriptorSet(cmd, surface.material->materialSet, 3);
    }

    // Only bind index buffer if it has changed
    if (surface.indexBuffer != mPreviousIndexBuffer)
    {
      mPreviousIndexBuffer = surface.indexBuffer;
      surface.bindIndexBuffer(cmd);
    }

    // Calculate final mesh matrix
    DrawPushConstants pushConstants;
    pushConstants.vertexBuffer = surface.vertexBufferAddress;
    pushConstants.worldMatrix = surface.transform;
    pushConstants.normalTransform = surface.invModelMatrix;

    //
    vkCmdPushConstants(cmd, surface.material->pPipeline->Layout()
                       , VK_SHADER_STAGE_VERTEX_BIT
                       , 0, sizeof(DrawPushConstants), &pushConstants);
    //
    // Note here that we have to offset from the initially pushed data since we
    // are really just filling a range alloted to us in total...
    vkCmdPushConstants(cmd, surface.material->pPipeline->Layout()
                       , VK_SHADER_STAGE_GEOMETRY_BIT
                       , sizeof(DrawPushConstants), sizeof(float), &expansionFactor);

    vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);

    // TODO
    // add counters for triangles and draws calls
    // stats.objectsRendered++;
    // stats.triangleCount += surface.indexCount / 3;
  }


// TODO need to implement a method that only draws the visible objects and
  void FcSceneRenderer::drawNormals(VkCommandBuffer cmd
                                    , FcDrawCollection& drawCollection, FrameData& currentFrame)
  {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mNormalDrawPipeline.getVkPipeline());

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mNormalDrawPipeline.Layout()
                              , 0, 1, &currentFrame.sceneDataDescriptorSet, 0, nullptr);

    // TODO could also draw the vectors for transparent objects but bypassed here
    for (uint32_t& surfaceIndex : mSortedObjectIndices)
    {
      FcRenderObject& surface = drawCollection.opaqueSurfaces[surfaceIndex];

      surface.bindIndexBuffer(cmd);

      // Calculate final mesh matrix
      DrawPushConstants pushConstants;
      pushConstants.vertexBuffer = surface.vertexBufferAddress;
      pushConstants.worldMatrix = surface.transform;
      pushConstants.normalTransform = surface.invModelMatrix;

      vkCmdPushConstants(cmd, mNormalDrawPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                         , 0, sizeof(DrawPushConstants), &pushConstants);

      vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);
    }
  }





  void FcSceneRenderer::drawBoundingBoxes(VkCommandBuffer cmd, FcDrawCollection& drawCollection
                                          ,FrameData& currentFrame)
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mBoundingBoxPipeline.getVkPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mBoundingBoxPipeline.Layout()
                            , 0, 1, &currentFrame.sceneDataDescriptorSet, 0, nullptr);

    // Draw all bounding boxes if, signaled by BoxId being == -1 (default value)
      if (mBoundingBoxId < 0)
      {
        for (FcRenderObject& renderObject : drawCollection.opaqueSurfaces)
        {
          // Send the bounding box to the shaders
          BoundingBoxPushConstants pushConstants;
          pushConstants.modelMatrix = renderObject.transform;
          pushConstants.origin = glm::vec4(renderObject.bounds.origin, 1.f);
          pushConstants.extents = glm::vec4(renderObject.bounds.extents, 0.f);

          vkCmdPushConstants(cmd, mBoundingBoxPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                             , 0, sizeof(BoundingBoxPushConstants), &pushConstants);

          // TODO update to utilize sascha method for quads
          vkCmdDraw(cmd, 36, 1, 0, 0);
        }
      }
      else // otherwise, just draw the object that we are told to
      {
        // make sure we don't try and draw a bounding box that doesn't exist
        if (mBoundingBoxId >= drawCollection.opaqueSurfaces.size())
        {
          mBoundingBoxId = -1;
        }
        else
        {
          FcRenderObject& renderObject = drawCollection.opaqueSurfaces[mBoundingBoxId];
                    // Send the bounding box to the shaders
          BoundingBoxPushConstants pushConstants;
          pushConstants.modelMatrix = renderObject.transform;
          pushConstants.origin = glm::vec4(renderObject.bounds.origin, 1.f);
          pushConstants.extents = glm::vec4(renderObject.bounds.extents, 0.f);

          vkCmdPushConstants(cmd, mBoundingBoxPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                             , 0, sizeof(BoundingBoxPushConstants), &pushConstants);

          // TODO update to utilize sascha method for quads
          vkCmdDraw(cmd, 36, 1, 0, 0);
        }
      }
  }





  void FcSceneRenderer::clearResources(VkDevice device)
  {
    mOpaquePipeline.destroy();
    mTransparentPipeline.destroy();
    vkDestroyDescriptorSetLayout(device, mMaterialDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, mSceneDataDescriptorLayout, nullptr);

    // Destroy data buffer with scene data
    mSceneDataBuffer.destroy();
  }
}
