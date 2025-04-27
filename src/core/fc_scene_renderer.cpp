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



  void FcSceneRenderer::init(glm::mat4& viewProj)
  {
    pViewProjection = &viewProj;
    // FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

    // // *-*-*-*-*-*-*-*-*-*-*-*-   FRAME DATA INITIALIZATION   *-*-*-*-*-*-*-*-*-*-*-*- //

    // // TODO create temporary storage for this in descClerk so we can just write the
    // // descriptorSet and layout on the fly and destroy layout if not needed
    // // TODO see if layout is not needed.
    // FcDescriptorBindInfo sceneDescriptorBinding{};
    // sceneDescriptorBinding.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    //                                   , VK_SHADER_STAGE_VERTEX_BIT
    //                                   // TODO DELETE after separating model from scene
    //                                   | VK_SHADER_STAGE_FRAGMENT_BIT
    //                                   | VK_SHADER_STAGE_GEOMETRY_BIT);

    // // TODO find out if there is any cost associated with binding to multiple un-needed stages...
    // //, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    // sceneDescriptorBinding.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mSceneDataBuffer
    //                                     , sizeof(SceneDataUbo), 0);

    // // create descriptorSet for sceneData
    // mSceneDataDescriptorLayout = descClerk.createDescriptorSetLayout(sceneDescriptorBinding);

    // // Allocate a descriptorSet to each frame buffer
    // for (FrameAssets& frame : frames)
    // {
    //   frame.sceneDataDescriptorSet = descClerk.createDescriptorSet(
    //     mSceneDataDescriptorLayout, sceneDescriptorBinding);
    // }
  }


  // TODO remove dependency on FcRenderer pointer
  void FcSceneRenderer::buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout)
  {
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
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);
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
  }



  // sort objects by pipeline and material prior to drawing them
  void FcSceneRenderer::sortByVisibility(FcDrawCollection& drawCollection)
  {
    // // TODO should consider sorting when first adding objects to draw, unless something changes
    // // or perhaps just inserting objects into draw via a hashmap. One thing to consider though
    // // is that we also perform visibility checks before we sort
    // // TODO should also make sure to sort using more than one thread
    // // Sort rendered objects according to material type and if the same sorted by indexBuffer
    // // A lot of big game engines do this to reduce the number of pipeline/descriptor set binds

    // // TODO should only have the the resize happen when we first add object to render then
    // // change IFF we add more scene objects

    // Only place the meshes whose bounding box is within the view frustrum
    for (size_t i = 0; i < drawCollection.opaqueSurfaces.size(); ++i)
    {
      drawCollection.visibleSurfaceIndices[i].clear();
      // BUG the bounding boxes are excluding visible objects for some reason
      // May only be on the sponza gltf...
      for (size_t index = 0; index < drawCollection.opaqueSurfaces[i].second.size(); ++index)
      {
        FcSurface& surface = drawCollection.opaqueSurfaces[i].second[index];

        // if (surface.isVisible(*pViewProjection))
        // {
          drawCollection.visibleSurfaceIndices[i].push_back(index);
          // }
      }
    }

      // // ?? couldn't we sort drawn meshes into a set of vectors that're already sorted by material
      // // and keep drawn object in linked list every iteration (unless removed manually) instead
      // // of clearing the draw list every update...


      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // TODO should consider sorting when first adding objects to draw, unless something changes
      // or perhaps just inserting objects into draw via a hashmap. One thing to consider though
      // is that we also perform visibility checks before we sort
      // TODO should also make sure to sort using more than one thread
      // Sort rendered objects according to material type and if the same sorted by indexBuffer
      // A lot of big game engines do this to reduce the number of pipeline/descriptor set binds

      // TODO should only have the the resize happen when we first add object to render then
      // change IFF we add more scene objects

      // ?? couldn't we sort drawn meshes into a set of vectors that're already sorted by material
      // and keep drawn object in linked list every iteration (unless removed manually) instead
      // of clearing the draw list every update...

      // TODO sort algorithm could be improved by calculating a sort key, and then our sortedOpaqueIndices
      // would be something like 20bits draw index and 44 bits for sort key/hash
      // sort the opaque surfaces by material and mesh
    }


  void FcSceneRenderer::draw(VkCommandBuffer cmd
                             , FcDrawCollection& drawCollection, FrameAssets& currentFrame)
  {
    // TODO just sort once and store the sorted indices unless something changes
    sortByVisibility(drawCollection);

    // Reset the previously used draw instruments for the new draw call
    // defined outside of the draw function, this is the state we will try to skip

    mPreviousIndexBuffer = VK_NULL_HANDLE;

    // First draw the opaque mesh nodes in draw collection
    mOpaquePipeline.bind(cmd);
    pCurrentPipeline = &mOpaquePipeline;

    // TODO follow this protocol for each subRenderer,
    // TODO group these together inside frameAssets for each renderer
    mExternalDescriptors[0] = currentFrame.sceneDataDescriptorSet;
    mExternalDescriptors[1] = currentFrame.skyBoxDescriptorSet;
    mExternalDescriptors[2] = currentFrame.shadowMapDescriptorSet;
    mOpaquePipeline.bindDescriptorSets(cmd, mExternalDescriptors, 0);

    for (size_t i = 0; i < drawCollection.opaqueSurfaces.size(); ++i)
    {
      mOpaquePipeline.bindDescriptorSet(cmd, drawCollection.opaqueSurfaces[i].first->materialSet, 3);

      /* drawSurface(cmd, drawCollection.opaqueSurfaces[i].second[0]); */
      for (size_t index : drawCollection.visibleSurfaceIndices[i])
      {
        drawSurface(cmd, drawCollection.opaqueSurfaces[i].second[index]);
      }
      // drawCollection.stats.triangleCount += drawMeshNode(cmd, meshNode, currentFrame);
      // drawCollection.stats.objectsRendered += meshNode.mMesh->Surfaces().size();
    }

    // Next, we draw all the transparent MeshNodes in draw collection
    mTransparentPipeline.bind(cmd);
    pCurrentPipeline = &mTransparentPipeline;

    // TODO update like above
    // ?? Do I need to re-bind these
    mTransparentPipeline.bindDescriptorSet(cmd, currentFrame.sceneDataDescriptorSet, 0);
    mTransparentPipeline.bindDescriptorSet(cmd, currentFrame.skyBoxDescriptorSet, 1);
    mTransparentPipeline.bindDescriptorSet(cmd, currentFrame.shadowMapDescriptorSet, 2);

    for (auto& materialCollection : drawCollection.transparentSurfaces)
    {
      mTransparentPipeline.bindDescriptorSet(cmd, materialCollection.first->materialSet, 3);

      for (FcSurface& surface : materialCollection.second)
      {
        drawSurface(cmd, surface);
      }
      // drawCollection.stats.triangleCount += drawMeshNode(cmd, meshNode, currentFrame);
      // drawCollection.stats.objectsRendered += meshNode.mMesh->Surfaces().size();
    }
  }

  [[deprecated("use draw surface")]]
  uint32_t FcSceneRenderer::drawMeshNode(VkCommandBuffer cmd
                                           , const FcMeshNode& meshNode, FrameAssets& currentFrame)
    {
      // uint32_t triangleCount = 0;

      // for (const FcSurface* surface : meshNode.visibleSurfaces)
      // {
      //   // Only rebind material descriptors if the material changed
      //   // TODO have each object track state of its own descriptorSets
      //   if (surface->material.get() != mPreviousMaterial)
      //   {
      //     mPreviousMaterial = surface->material.get();
      //     pCurrentPipeline->bindDescriptors(cmd, surface->material->materialSet, 3);
      //   }

      //   // Only bind index buffer if it has changed
      //   if (meshNode.mMesh->IndexBuffer() != mPreviousIndexBuffer)
      //   {
      //     mPreviousIndexBuffer = meshNode.mMesh->IndexBuffer();
      //     meshNode.mMesh->bindIndexBuffer(cmd);
      //   }

      //   // Calculate final mesh matrix
      //   DrawPushConstants pushConstants;
      //   pushConstants.vertexBuffer = meshNode.mMesh->VertexBufferAddress();
      //   pushConstants.worldMatrix = meshNode.worldTransform;
      //   pushConstants.normalTransform = glm::inverse(glm::transpose(meshNode.worldTransform));

      //   //
      //   vkCmdPushConstants(cmd, pCurrentPipeline->Layout()
      //                      , VK_SHADER_STAGE_VERTEX_BIT
      //                      , 0, sizeof(DrawPushConstants), &pushConstants);
      //   //
      //   // Note here that we have to offset from the initially pushed data since we
      //   // are really just filling a range alloted to us in total...
      //   vkCmdPushConstants(cmd, pCurrentPipeline->Layout()
      //                      , VK_SHADER_STAGE_GEOMETRY_BIT
      //                      , sizeof(DrawPushConstants), sizeof(float), &expansionFactor);

      //   vkCmdDrawIndexed(cmd, surface->indexCount, 1, surface->startIndex, 0, 0);

      //   // TODO
      //   // add counters for triangles and draws calls
      //   triangleCount += surface->indexCount / 3;
      // }
      // return triangleCount;
      return 0;
    }



    void FcSceneRenderer::drawSurface(VkCommandBuffer cmd, const FcSurface& surface)
    {
      // Only rebind pipeline and material descriptors if the material changed
      // TODO have each object track state of its own descriptorSets
      // There are only two pipelines so far so should just draw all opaque, then all transparent

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
      vkCmdPushConstants(cmd, pCurrentPipeline->Layout()
                         , VK_SHADER_STAGE_VERTEX_BIT
                         , 0, sizeof(DrawPushConstants), &pushConstants);
      //
      // Note here that we have to offset from the initially pushed data since we
      // are really just filling a range alloted to us in total...
      vkCmdPushConstants(cmd, pCurrentPipeline->Layout()
                         , VK_SHADER_STAGE_GEOMETRY_BIT
                         , sizeof(DrawPushConstants), sizeof(float), &expansionFactor);

      vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);

      // TODO
      // add counters for triangles and draws calls
      // currstats.objectsRendered++;
      // stats.triangleCount += surface.indexCount / 3;
    }



    void FcSceneRenderer::clearResources(VkDevice device)
    {
      mOpaquePipeline.destroy();
      mTransparentPipeline.destroy();
      vkDestroyDescriptorSetLayout(device, mMaterialDescriptorLayout, nullptr);
    }
  }
