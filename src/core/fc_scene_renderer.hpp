//>--- fc_scene_renderer.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_types.hpp"
#include "fc_pipeline.hpp"
#include "core/fc_buffer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcDescriptorCollection; class FcMeshNode; class FcDrawCollection; class FcSubmesh; class FcImage;}
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcSceneRenderer
  {
   private:
     FcBuffer mSceneDataBuffer;
     FcPipeline mOpaquePipeline;
     FcPipeline mTransparentPipeline;
     FcPipeline mWireframePipeline;
     std::vector<uint32_t> mSortedObjectIndices;
     VkDescriptorSetLayout mMaterialDescriptorLayout;
     FcPipeline* pCurrentPipeline;
     glm::mat4* pViewProjection;
     VkBuffer mPreviousIndexBuffer;
     float expansionFactor{0};
     /* FcDescriptorCollection* pCollection; */

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DELETE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     /* VkDescriptorSetLayout mSceneDataDescriptorLayout; */
     /* VkDescriptorSet mShadowMapDescriptorSet; */
     /* VkDescriptorSet mUboAndShadowDescSet; */
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkDescriptorSet mSceneDescriptorSet;



     std::vector<VkDescriptorSet> mExternalDescriptors {2};
     void sortByVisibility(FcDrawCollection& drawCollection);
     uint32_t drawMeshNode(VkCommandBuffer cmd, const FcMeshNode& surface) noexcept;

     void drawSurface(VkCommandBuffer cmd, const FcSubmesh& surface) noexcept;
     void buildPipelines(FcImage& shadowMap);
   public:
     // TODO think about including a local descriptorClerk
     void init(glm::mat4& viewProj, FcDescriptorCollection& frame, FcImage& shadowMap);
     //
     FcPipeline* TransparentPipeline() { return &mTransparentPipeline; }
     //
     FcPipeline* OpaquePipeline() { return &mOpaquePipeline; }
     //
     inline void updateSceneData(SceneData& sceneData) noexcept
      { mSceneDataBuffer.write(&sceneData, sizeof(SceneData)); }
     void updateBindlessDescriptors();
     //
     void draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection, bool shouldDrawWireFrame) noexcept;
     //
     void destroy();
     float& ExpansionFactor() { return expansionFactor; }
     inline const FcBuffer& getSceneDataBuffer() const { return mSceneDataBuffer; }
     //DELETE
     /* inline const VkDescriptorSetLayout getSceneDescriptorLayout() const { return mSceneDataDescriptorLayout; } */

  };


}// --- namespace fc --- (END)
