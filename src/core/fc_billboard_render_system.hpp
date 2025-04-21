#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// //
// #include "fc_pipeline.hpp"
#include "core/fc_pipeline.hpp"
#include "fc_image.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include <memory>

namespace fc
{
   // *-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FcPipeline;

   // should this be inside billboard class
  struct BillboardPushes
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
     FcImage mTexture;
     VkDescriptorSet mDescriptor{nullptr};
     uint32_t mHandleIndex; // ?? find out if we even need a handle index
     BillboardPushes mPush;

   public:
      // - CTORS -
     FcBillboard(float width, float height, glm::vec4 color);
     FcBillboard() = default; // { mTextureId = 0; }
     FcBillboard(const FcBillboard&) = delete;
     FcBillboard& operator=(const FcBillboard&) = delete;
     FcBillboard(FcBillboard&&) = delete;
     FcBillboard& operator=(FcBillboard&&) = delete;

     void placeInHandleTable();
     BillboardPushes& PushComponent() { return mPush; }
     void loadTexture(std::string filename, VkDescriptorSetLayout layout);
     void loadTexture(VkDescriptorSetLayout layout);
      //uint32_t TextureId() { return mTextureId; }
      //void setTextureId(uint32_t textureId) { mTextureId = textureId; }

     VkDescriptorSet getDescriptor() { return mDescriptor; };
  };


  class FcBillboardRenderer
  {
   private:
     FcPipeline mPipeline;
      //
   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcBillboardRenderer() = default;
     ~FcBillboardRenderer() = default;
     FcBillboardRenderer(const FcBillboardRenderer&) = delete;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OVERLOADS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcBillboardRenderer &operator=(const FcBillboardRenderer&) = delete;
     //
     /* void draw(glm::vec3 cameraPosition, uint32_t swapchainImageIndex, SceneDataUbo& ubo); */
     void createPipeline();
     void sortBillboardsByDistance(glm::vec3& cameraPosition);
  };

} // namespace fc _END_
