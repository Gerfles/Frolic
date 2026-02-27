//>_ fc_scene_renderer.hpp _<//
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/mat4x4.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECLS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FrameAssets; class FcMeshNode; class FcDrawCollection; class FcSubmesh; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
    class FcSceneRenderer
    {
     private:
       FcPipeline mOpaquePipeline;
       FcPipeline mTransparentPipeline;
       FcPipeline mWireframePipeline;
       std::vector<uint32_t> mSortedObjectIndices;
       VkDescriptorSetLayout mMaterialDescriptorLayout;
       FcPipeline* pCurrentPipeline;
       glm::mat4* pViewProjection;
       VkBuffer mPreviousIndexBuffer;
       float expansionFactor{0};
       std::vector<VkDescriptorSet> mExternalDescriptors {3};
       void sortByVisibility(FcDrawCollection& drawCollection);
       uint32_t drawMeshNode(VkCommandBuffer cmd,
                             const FcMeshNode& surface,
                             FrameAssets& currentFrame);
       void drawSurface(VkCommandBuffer cmd, const FcSubmesh& surface) noexcept;
       void buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout, std::vector<FrameAssets>& frames);
     public:
       // TODO think about including a local descriptorClerk
       void init(VkDescriptorSetLayout sceneDescriptorLayout, glm::mat4& viewProj, std::vector<FrameAssets>& frames);
       FcPipeline* TransparentPipeline() { return &mTransparentPipeline; }
       FcPipeline* OpaquePipeline() { return &mOpaquePipeline; }
       //
       void draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection,
                 FrameAssets& currentFrame, bool shouldDrawWireFrame);
       //
       void destroy();
       float& ExpansionFactor() { return expansionFactor; }
    };


}// --- namespace fc --- (END)
