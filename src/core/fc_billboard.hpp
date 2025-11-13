//>_ fc_billboard.hpp _<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
#include "fc_descriptors.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/vec3.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string_view>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FcRenderer;

  // BillboardPushes are not created directly but instead are the values that the shader
  // will recieve from pushing an Billboard object with vkCmdPushConstants and using this
  // struct as the size of the data sent. Billboard must have the first members exactly
  // match this structure. This will save us from having to copy to a separate structure
  // every time
  struct BillboardPushes
  {
     glm::vec3 position;
     float width;
     float height;
     u32 textureIndex;
     // This structure should not be used directly but instead serves as a size indicator
     BillboardPushes() = delete;
  };

  //
  //
  // The base class for any 2D view-aligned object
  class FcBillboard
  {
   private:
     glm::vec3 mPosition;
     float mWidth;
     float mHeight;
     u32 mTextureIndex;

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcBillboard& operator=(const FcBillboard&) = delete;
     FcBillboard(const FcBillboard&) = delete;
     FcBillboard(FcBillboard&&) = delete;
     FcBillboard& operator=(FcBillboard&&) = delete;
     //
     inline FcBillboard(float width = 1.0f, float height = 1.0f) : mWidth{width} , mHeight{height} {}

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void loadTexture(FcRenderer& renderer, float width,
                      float height, std::string_view filename);
     //
     inline void setPosition(const glm::vec3& position) { mPosition = position; }
     //
     inline void setPosition(const float x, const float y, const float z)
      { mPosition.x = x; mPosition.y = y; mPosition.z = z; }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline const glm::vec3 Position() const { return mPosition; }
     //
     inline const glm::vec3& position() const { return mPosition; }
  };

}// --- namespace fc --- (END)
