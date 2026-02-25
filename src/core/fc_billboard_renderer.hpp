//>_ fc_billboard_renderer.hpp _<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_buffer.hpp"
#include "fc_pipeline.hpp"
#include "platform.hpp"
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
  class FcBillboard;
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

  struct BillboardPushConstants
  {
     // BillboardPushes are not created directly but instead are the values that the shader
     // will recieve from pushing an Billboard object with vkCmdPushConstants and using this
     // struct as the size of the data sent. Billboard must have the first members exactly
     // match this structure. This will save us from having to copy to a separate structure
     // every time
     BillboardPushConstants() = delete;
     glm::vec3 position;
     float width;
     float height;
     u32 textureIndex;
  };

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
