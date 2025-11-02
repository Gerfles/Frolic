//>_ fc_billboard_renderer.hpp _<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_buffer.hpp"
#include "fc_pipeline.hpp"
#include "fc_billboard.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <memory>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECLS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  class SceneDataUbo;
  class FrameAssets;

  // TODO could make Ubos more modular, that way we could just
  // rebind them to other pipelines instead of resetting a lot of matrices and vectors etc...
  // LEARN the benefits and drawbacks to just binding an oversize UBO even if we only need
  // a couple items from that UBO. Ideally, we shoud store typical scene data in buffer on GPU
  // that we can just access from each shader.
  struct BillboardUbo
  {
     glm::mat4 view;
     glm::mat4 projection;
  };

  //
  //
  class FcBillboardRenderer
  {
   private:
     FcPipeline mPipeline;
     std::vector<std::shared_ptr<FcBillboard>> mBillboards;
     BillboardUbo mUbo;
     FcBuffer mUboBuffer;
     VkDescriptorSetLayout mUboDescriptorSetLayout;
     VkDescriptorSet mUboDescriptorSet;

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcBillboardRenderer() = default;
     ~FcBillboardRenderer() = default;
     FcBillboardRenderer(const FcBillboardRenderer&) = delete;
     FcBillboardRenderer(FcBillboardRenderer&&) = delete;
     FcBillboardRenderer& operator=(const FcBillboardRenderer&) = delete;
     FcBillboardRenderer& operator=(FcBillboardRenderer&&) = delete;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void addBillboard(FcBillboard& billboard);
     void draw(VkCommandBuffer cmd, SceneDataUbo& sceneData, FrameAssets& currentFrame);
     void buildPipelines();
     void sortBillboardsByDistance(glm::vec4& cameraPosition);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
  };

}// --- namespace fc --- (END)
