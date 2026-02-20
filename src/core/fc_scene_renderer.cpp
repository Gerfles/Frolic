//>  fc_scene_renderer.cpp <//
#include "fc_scene_renderer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_mesh.hpp"
#include "fc_node.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_draw_collection.hpp"
#include "core/fc_gpu.hpp"
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


  void FcSceneRenderer::init(VkDescriptorSetLayout sceneDescriptorLayout, glm::mat4& viewProj, std::vector<FrameAssets>& frames)
  {
    pViewProjection = &viewProj;
    buildPipelines(sceneDescriptorLayout, frames);

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

  //
  // TODO remove dependency on FcRenderer pointer
  void FcSceneRenderer::buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout, std::vector<FrameAssets>& frames)
  {
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OPAQUE PIPELINE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "Opaque Pipeline";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "mesh.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "scene3.frag.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_GEOMETRY_BIT, "explode.geom.spv");

    // add push constants for the model & normal matrices and address of vertex buffer
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(ScenePushConstants);
    //
    pipelineConfig.addPushConstants(matrixRange);

    // Add push for the expansion effect -> amount to expand the polygons
    VkPushConstantRange expansionFactorRange;
    expansionFactorRange.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    expansionFactorRange.offset = sizeof(ScenePushConstants);
    expansionFactorRange.size = sizeof(float);
    //
    pipelineConfig.addPushConstants(expansionFactorRange);

    // place the scene descriptor layout in the first set (0), then cubemap, then material
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);

    // Add single image descriptors for sky box image
    pipelineConfig.addSingleImageDescriptorSetLayout();

    // Add single image descriptors for shadow map image
    pipelineConfig.addSingleImageDescriptorSetLayout();

    // *-*-*-*-*-*-*-*-*-*-*-   WITHOUT BINDLESS DESCRIPTORS   *-*-*-*-*-*-*-*-*-*-*- //
    // create the descriptor set layout for the material
    FcDescriptorBindInfo bindInfo{};
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    mMaterialDescriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    // Finally add the descriptor set layout for materials
    pipelineConfig.addDescriptorSetLayout(mMaterialDescriptorLayout);

    FcDescriptorBindInfo bindlessBindInfo;
    bindlessBindInfo.enableBindlessTextures();
    VkDescriptorSetLayout bindlessLayout = descClerk.createDescriptorSetLayout(bindlessBindInfo);

    for (FrameAssets& frame : frames)
    {
      frame.sceneBindlessTextureSet = descClerk.createDescriptorSet(bindlessLayout, bindlessBindInfo);
    }

    pipelineConfig.addDescriptorSetLayout(bindlessLayout);

    pipelineConfig.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineConfig.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);

    mOpaquePipeline.create(pipelineConfig);

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   WIREFRAME PIPELINE   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // TODO check for capability
    bool isWireframeAvailable = true;
    if (isWireframeAvailable)
    {
      pipelineConfig.name = "Scene Wireframe Pipeline";
      pipelineConfig.setPolygonMode(VK_POLYGON_MODE_LINE);
      pipelineConfig.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
      mWireframePipeline.create(pipelineConfig);
    }

    // *-*-*-*-*-*-*-*-*-*-*-*-*-   TRANSPARENTE PIPELINE   *-*-*-*-*-*-*-*-*-*-*-*-*- //
    // using the same pipeline config, alter slightly for transparent models
    pipelineConfig.name = "Transparent Pipeline";
    pipelineConfig.enableBlendingAdditive();
    pipelineConfig.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.enableDepthtest(VK_FALSE, VK_COMPARE_OP_GREATER_OR_EQUAL);

    mTransparentPipeline.create(pipelineConfig);
  }


  // BUG culling is not working properly as there are many triangles drawn
  // while no objects in view frustum even when rotating more towards the objects will
  // often reduce that count to zero just before the object actually appears on screen
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
        FcSubmesh& subMesh = drawCollection.opaqueSurfaces[i].second[index];

        if (subMesh.isVisible(*pViewProjection))
        {
          drawCollection.visibleSurfaceIndices[i].push_back(index);
        }
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



  //
  //
  void FcSceneRenderer::draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection,
                             FrameAssets& currentFrame, bool shouldDrawWireFrame)
  {
    // *-*-*-*-*-*-*-*-*-*-*-*-   ATTEMPTING DRAW INDIRECT   *-*-*-*-*-*-*-*-*-*-*-*- //

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END DI ATTEMPT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    // Reset the draw statistics
    drawCollection.stats.triangleCount = 0;

    sortByVisibility(drawCollection);

    // Reset the previously used draw instruments for the new draw call
    // defined outside of the draw function, this is the state we will try to skip
    mPreviousIndexBuffer = VK_NULL_HANDLE;

    // First draw the opaque mesh nodes in draw collection
    if (shouldDrawWireFrame)
    {
      mWireframePipeline.bind(cmd);
      pCurrentPipeline = &mWireframePipeline;
    }
    else
    {
      mOpaquePipeline.bind(cmd);
      pCurrentPipeline = &mOpaquePipeline;
    }


    // TODO follow this protocol for each subRenderer,
    // TODO group these together inside frameAssets for each renderer
    // ?? for some reason the renderer still works somewhat well without binding skybox etc.??
    mExternalDescriptors[0] = currentFrame.sceneDataDescriptorSet;
    mExternalDescriptors[1] = currentFrame.skyBoxDescriptorSet;
    mExternalDescriptors[2] = currentFrame.shadowMapDescriptorSet;

    mOpaquePipeline.bindDescriptorSets(cmd, mExternalDescriptors, 0);

    mOpaquePipeline.bindDescriptorSet(cmd, currentFrame.sceneBindlessTextureSet, 4);

    // First draw all the opaque meshNode objects in draw collection
    for (size_t i = 0; i < drawCollection.opaqueSurfaces.size(); ++i)
    {
      mOpaquePipeline.bindDescriptorSet(cmd,
                                        drawCollection.opaqueSurfaces[i].first->materialSet, 3);

      for (size_t index : drawCollection.visibleSurfaceIndices[i])
      {
        const FcSubmesh& subMesh = drawCollection.opaqueSurfaces[i].second[index];
        drawSurface(cmd, subMesh);

        // Update the engine statistics
        drawCollection.stats.triangleCount += subMesh.indexCount / 3;
        drawCollection.stats.objectsRendered += 1;
      }
    }


    // Next, we draw all the transparent MeshNodes in draw collection
    mTransparentPipeline.bind(cmd);
    pCurrentPipeline = &mTransparentPipeline;

    for (size_t i = 0; i < drawCollection.transparentSurfaces.size(); ++i)
    {
      mOpaquePipeline.bindDescriptorSet(cmd,
                                        drawCollection.transparentSurfaces[i].first->materialSet, 3);

      for (size_t index : drawCollection.visibleSurfaceIndices[i])
      {
        const FcSubmesh& subMesh = drawCollection.transparentSurfaces[i].second[index];
        drawSurface(cmd, subMesh);

        // Update the engine statistics
        drawCollection.stats.triangleCount += subMesh.indexCount / 3;
        drawCollection.stats.objectsRendered += 1;
      }
    }
  }

  //
  //
  void FcSceneRenderer::drawSurface(VkCommandBuffer cmd, const FcSubmesh& subMesh) noexcept
  {
    // Only rebind pipeline and material descriptors if the material changed
    // TODO have each object track state of its own descriptorSets
    // There are only two pipelines so far so should just draw all opaque, then all transparent

    // Only bind index buffer if it has changed
    // TODO could make this branchless if all submeshes rendered in groups/sequentially
    if (subMesh.parent.lock()->IndexBuffer() != mPreviousIndexBuffer)
    {
      mPreviousIndexBuffer = subMesh.parent.lock()->IndexBuffer();
      subMesh.parent.lock()->bindIndexBuffer(cmd);
    }

    vkCmdPushConstants(cmd, pCurrentPipeline->Layout()
                       , VK_SHADER_STAGE_VERTEX_BIT
                       , 0, sizeof(ScenePushConstants), subMesh.getSceneConstantsPtr());

    // Note here that we have to offset from the initially pushed data since we
    // are really just filling a range alloted to us in total.
    vkCmdPushConstants(cmd, pCurrentPipeline->Layout()
                       , VK_SHADER_STAGE_GEOMETRY_BIT
                       , sizeof(ScenePushConstants), sizeof(float), &expansionFactor);

    vkCmdDrawIndexed(cmd, subMesh.indexCount, 1, subMesh.startIndex, 0, 0);
  }


  void FcSceneRenderer::destroy()
  {
    vkDestroyDescriptorSetLayout(FcLocator::Device(), mMaterialDescriptorLayout, nullptr);
    mOpaquePipeline.destroy();
    mTransparentPipeline.destroy();
  }
}
