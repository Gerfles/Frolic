#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// //
#include "core/utilities.hpp"
#include "fc_buffer.hpp"
#include "fc_pipeline.hpp"
#include "fc_image.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
/* #include <glm/vec4.hpp> */
#include <glm/mat4x4.hpp>
/* #include <glm/vec3.hpp> */
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include <memory>

namespace fc
{
   // *-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FcPipeline;
  class FrameAssets;
  class SceneDataUbo;



  struct BillboardPushes
  {
     // ?? do pushes care about alignment??
     // TODO vec4 used to simplify alignment. may want to do differently but also
     // could use the w component to signify point/vector (eg, point lignt vs directional light)
     glm::vec4 position; // (x,y,z,0) = vector, (x,y,z,1) = point
     float width;
     float height;
  };

  class FcBillboard
  {
   private:
     FcImage mTexture;
     VkDescriptorSet mDescriptor{nullptr};
     BillboardPushes mPush;

   public:
      // - CTORS -
     FcBillboard(float width, float height, glm::vec4 color);
     FcBillboard() = default; // { mTextureId = 0; }
     FcBillboard& operator=(const FcBillboard&) = delete;
     // All three needed to use emplace_back within billboard renderer
     /* FcBillboard(const FcBillboard&) = delete; */
     /* FcBillboard(FcBillboard&&) = delete; */
     /* FcBillboard& operator=(FcBillboard&&) = delete; */

     void placeInHandleTable();
     BillboardPushes& PushComponent() { return mPush; }
     void loadTexture(std::filesystem::path &filename);

     VkDescriptorSet getDescriptor() { return mDescriptor; };
     // TODO pass by reference and TEST
     void setPosition(glm::vec4 position) { mPush.position = position; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     glm::vec4 Position() { return mPush.position; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
  };


  // TODO starting with this one, we could make Ubos more modular, that way we could just
  // rebind them to other pipelines instead of resetting a lot of matrices and vectors etc...
  // LEARN the benefits and drawbacks to just binding an oversize UBO even if we only need
  // a couple items from that UBO
  struct BillboardUbo
  {
     glm::mat4 view;
     glm::mat4 projection;
  };



  class FcBillboardRenderer
  {
   private:
     FcPipeline mPipeline;
     std::vector<std::shared_ptr<FcBillboard>> mBillboards;
     BillboardUbo mUbo;
     FcBuffer mUboBuffer;
     VkDescriptorSetLayout mUboDescriptorSetLayout;
     VkDescriptorSet mUboDescriptorSet;
     VkDescriptorSetLayout mImageDescriptorSetLayout;

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcBillboardRenderer() = default;
     ~FcBillboardRenderer() = default;
     FcBillboardRenderer(const FcBillboardRenderer&) = delete;
     FcBillboardRenderer &operator=(const FcBillboardRenderer&) = delete;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void addBillboard(FcBillboard& billboard);
     void loadBillboard(std::filesystem::path& filename);
     void draw(VkCommandBuffer cmd, SceneDataUbo& sceneData, FrameAssets& currentFrame);
     void buildPipelines();
     void sortBillboardsByDistance(glm::vec3& cameraPosition);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
  };

} // namespace fc _END_
