//>--- fc_billboard.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/vec3.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string_view>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcRenderer; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
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
     inline FcBillboard(float width = 1.0f, float height = 1.0f) noexcept
       : mWidth{width} , mHeight{height} {}

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void loadTexture(FcRenderer& renderer, float width,float height, std::string_view filename);
     //
     inline void setPosition(const glm::vec3& position) noexcept { mPosition = position; }
     //
     inline void setPosition(const float x, const float y, const float z) noexcept
      { mPosition.x = x; mPosition.y = y; mPosition.z = z; }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline const glm::vec3& Position() const noexcept { return mPosition; }
  };

}// --- namespace fc --- (END)
