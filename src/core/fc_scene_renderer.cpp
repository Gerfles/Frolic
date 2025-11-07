//>  fc_scene_renderer.cpp <//
#include "fc_scene_renderer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
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


  void FcSceneRenderer::init(VkDescriptorSetLayout sceneDescriptorLayout, glm::mat4& viewProj)
  {
    pViewProjection = &viewProj;
    buildPipelines(sceneDescriptorLayout);

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
  void FcSceneRenderer::buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout)
  {
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OPAQUE PIPELINE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // TODO addshader() func
    // TODO add separate pipeline for effects "explode, etc."
    FcPipelineConfig pipelineConfig{3};
    pipelineConfig.name = "Opaque Pipeline";
    pipelineConfig.shaders[0].filename = "mesh.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    /* pipelineConfig.shaders[1].filename = "brdf.frag.spv"; */
    /* pipelineConfig.shaders[1].filename = "scene.frag.spv"; */
    /* pipelineConfig.shaders[1].filename = "scene2.frag.spv"; */
    pipelineConfig.shaders[1].filename = "scene3.frag.spv";
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


    // // place the scene descriptor layout in the first set (0), then cubemap, then material
    // // TODO find a better way to pass these descriptor sets around etc...
    // // ?? Are they even needed in here??
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);

    // Add single image descriptors for sky box image
    pipelineConfig.addSingleImageDescriptorSetLayout();
    // Add single image descriptors for shadow map image
    pipelineConfig.addSingleImageDescriptorSetLayout();

    // *-*-*-*-*-*-*-*-*-*-*-   WITHOUT BINDLESS DESCRIPTORS   *-*-*-*-*-*-*-*-*-*-*- //
    // create the descriptor set layout for the material
    FcDescriptorBindInfo bindInfo{};
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    // bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    //                     , VK_SHADER_STAGE_FRAGMENT_BIT);
    // bindInfo.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    //                     , VK_SHADER_STAGE_FRAGMENT_BIT);
    // bindInfo.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    //                     , VK_SHADER_STAGE_FRAGMENT_BIT);
    // bindInfo.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    //                     , VK_SHADER_STAGE_FRAGMENT_BIT);
    // bindInfo.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    //                     , VK_SHADER_STAGE_FRAGMENT_BIT);
    // // TODO check to see if we even need a member variable for the below?? could it be temporary
    /* bindInfo.addBinding(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); */

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    mMaterialDescriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // Finally add the descriptor set layout for materials
    pipelineConfig.addDescriptorSetLayout(mMaterialDescriptorLayout);

    pipelineConfig.addDescriptorSetLayout(descClerk.mBindlessDescriptorLayout);

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
    // TODO make sure the transparent pipeline handles things like shadow differently
    pipelineConfig.name = "Transparent Pipeline";
    pipelineConfig.enableBlendingAdditive();
    pipelineConfig.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

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
        FcSurface& surface = drawCollection.opaqueSurfaces[i].second[index];

        if (surface.isVisible(*pViewProjection))
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


  // TODO maybe pass vector instead
  std::vector<IndirectBatch> FcSceneRenderer::compactDraws(FcSurface* objects, int count)
  {
    std::vector<IndirectBatch> draws;

    // IndirectBatch firstDraw;
    // firstDraw.mesh = objects[0].mesh;
    // firstDraw.material = objects[0].material;
    // firstDraw.first = 0;
    // firstDraw.count = 1;

    // draws.push_back(firstDraw);

    // for (int i = 0; i < count; i++)
    // {
    //   // compare the mesh and material with the end of the vector of draws
    //   bool sameMesh = objects[i].mesh == draws.back().mesh;
    //   bool sameMaterial = objects[i].material == draws.back().material;

    //   if (sameMesh && sameMaterial)
    //   {
    //     draws.back().count++;
    //   }
    //   else
    //   {
    //     // add a new draw
    //     IndirectBatch newDraw;
    //     newDraw.mesh = objects[i].mesh;
    //     newDraw.material = objects[i].material;
    //     newDraw.first = i;
    //     newDraw.count = 1;

    //     draws.push_back(newDraw);
    //   }
    // }

    return draws;
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

    // TODO just sort once and store the sorted indices unless something is added or removed
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

    // TO be used when no culling is desired
    // for (auto& materialCollection : drawCollection.opaqueSurfaces)
    // {
    //   mOpaquePipeline.bindDescriptorSet(cmd, materialCollection.first->materialSet, 3);

    //   for (FcSurface& surface : materialCollection.second)
    //   {
    //     drawSurface(cmd, surface);
    //   }
    // }

    VkDescriptorSet bindlessTextures = FcLocator::DescriptorClerk().mBindlessDescriptorSet;
    mOpaquePipeline.bindDescriptorSet(cmd, bindlessTextures, 4);

    for (size_t i = 0; i < drawCollection.opaqueSurfaces.size(); ++i)
    {
      // TODO find a way to only bind once
      mOpaquePipeline.bindDescriptorSet(cmd,
                                        drawCollection.opaqueSurfaces[i].first->materialSet, 3);

      for (size_t index : drawCollection.visibleSurfaceIndices[i])
      {
        const FcSurface& surface = drawCollection.opaqueSurfaces[i].second[index];

        drawSurface(cmd, surface);

        // Update the engine statistics
        drawCollection.stats.triangleCount += surface.mIndexCount / 3;
        drawCollection.stats.objectsRendered += 1;
      }
    }

    // Next, we draw all the transparent MeshNodes in draw collection
    mTransparentPipeline.bind(cmd);
    pCurrentPipeline = &mTransparentPipeline;

    // TODO update like above
    for (auto& materialCollection : drawCollection.transparentSurfaces)
    {
      mTransparentPipeline.bindDescriptorSet(cmd, materialCollection.first->materialSet, 3);

      for (FcSurface& surface : materialCollection.second)
      {
        drawSurface(cmd, surface);
      }
      // drawCollection.stats.triangleCount += surface.indexCount / 3;
      // drawCollection.stats.objectsRendered += 1;
    }
  }


  void FcSceneRenderer::drawSurface(VkCommandBuffer cmd, const FcSurface& surface) noexcept
  {
    // Only rebind pipeline and material descriptors if the material changed
    // TODO have each object track state of its own descriptorSets
    // There are only two pipelines so far so should just draw all opaque, then all transparent

    // Only bind index buffer if it has changed
    if (surface.mIndexBuffer.getVkBuffer() != mPreviousIndexBuffer)
    {
      mPreviousIndexBuffer = surface.mIndexBuffer.getVkBuffer();
      surface.bindIndexBuffer(cmd);
    }

    // TODO make all push constants address to matrix buffer and texture indices
    // Calculate final mesh matrix
    DrawPushConstants pushConstants;
    pushConstants.vertexBuffer = surface.mVertexBufferAddress;
    pushConstants.worldMatrix = surface.mTransform;
    pushConstants.normalTransform = surface.mInvModelMatrix;

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

    vkCmdDrawIndexed(cmd, surface.mIndexCount, 1, surface.mFirstIndex, 0, 0);

    // TODO
    // add counters for triangles and draws calls
    // currstats.objectsRendered++;
    // stats.triangleCount += surface.indexCount / 3;
  }


  void FcSceneRenderer::destroy()
  {
    vkDestroyDescriptorSetLayout(FcLocator::Device(), mMaterialDescriptorLayout, nullptr);
    mOpaquePipeline.destroy();
    mTransparentPipeline.destroy();
  }
}
