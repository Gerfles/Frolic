#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <memory>


namespace fc
{
   // should this be inside billboard class
  struct BillboardPushComponent
  {
      // TODO vec4 used to simplify alignment. may want to do differently but also
      // could use the w component to signify point/vector (eg, point lignt vs directional light)
     glm::vec4 position {1.f}; // (x,y,z,0) = vector, (x,y,z,1) = point
     glm::vec4 color {1.f};
     float width;
     float height;
  };


  class FcBillboard
  {
   private:
     uint32_t mTextureId;
     uint32_t mHandleIndex; // ?? find out if we even need a handle index
     BillboardPushComponent mPush;

   public:
      // - CTORS -
     FcBillboard() = default; // { mTextureId = 0; }
     FcBillboard(float width, float height, glm::vec4 color);
     FcBillboard(const FcBillboard&) = delete;
     FcBillboard& operator=(const FcBillboard&) = delete;
     FcBillboard(FcBillboard&&) = default;
     FcBillboard& operator=(FcBillboard&&) = default;

     void placeInHandleTable();
     BillboardPushComponent& PushComponent() { return mPush; }
     uint32_t TextureId() { return mTextureId; }
     void setTextureId(uint32_t textureId) { mTextureId = textureId; }
  };


  class FcBillboardRenderSystem
  {
   private:
      //
   public:
     void createPipeline(FcPipeline& pipeline, VkRenderPass& renderPass);
     FcBillboardRenderSystem() = default;
     ~FcBillboardRenderSystem() = default;
     FcBillboardRenderSystem(const FcBillboardRenderSystem&) = delete;
     FcBillboardRenderSystem &operator=(const FcBillboardRenderSystem&) = delete;
      //
     void sortBillboardsByDistance(glm::vec3& cameraPosition);
  };

} // namespace fc _END_
