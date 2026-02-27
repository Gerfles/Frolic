//>--- fc_billboard_renderer.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_buffer.hpp"
#include "fc_pipeline.hpp"
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <memory>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECLS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class SceneDataUbo; class FrameAssets; class FcBillboard; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  class FcBillboardRenderer
  {
   private:
     FcPipeline mPipeline;
     //
     // TODO could make Ubos more modular, that way we could just
     // rebind them to other pipelines instead of resetting a lot of matrices and vectors etc...
     // LEARN the benefits and drawbacks to just binding an oversize UBO even if we only need
     // a couple items from that UBO. Ideally, we shoud store typical scene data in buffer on GPU
     // that we can just access from each shader.
     struct BillboardUbo
     {
        glm::mat4 view;
        glm::mat4 projection;
     } mBillboardUbo;
     //
     FcBuffer mUboBuffer;
     //
     std::vector<std::shared_ptr<FcBillboard>> mBillboards;

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcBillboardRenderer() = default;
     ~FcBillboardRenderer() = default;
     FcBillboardRenderer(const FcBillboardRenderer&) = delete;
     FcBillboardRenderer(FcBillboardRenderer&&) = delete;
     FcBillboardRenderer& operator=(const FcBillboardRenderer&) = delete;
     FcBillboardRenderer& operator=(FcBillboardRenderer&&) = delete;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void addBillboard(FcBillboard& billboard) noexcept;
     //
     void draw(VkCommandBuffer cmd, SceneDataUbo& sceneData, FrameAssets& currentFrame) noexcept;
     //
     void buildPipelines() noexcept;
     //
     void buildPipelines(std::vector<FrameAssets>& frames) noexcept;
     //
     void sortBillboardsByDistance(glm::vec4& cameraPosition) noexcept;

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy() noexcept;
  }; // ---   class FcBillboardRenderer --- (END);

}// --- namespace fc --- (END)
