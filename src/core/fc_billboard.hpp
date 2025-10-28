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


  struct BillboardPushes
  {
     glm::vec3 position;
     float width;
     float height;
     u32 textureIndex;
  };

  class FcBillboard
  {
   private:
     BillboardPushes mPush;

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline FcBillboard(float width = 0.5f, float height = 0.5f)
      {mPush.width = width; mPush.height = height;}
     FcBillboard& operator=(const FcBillboard&) = delete;
     FcBillboard(const FcBillboard&) = delete;
     FcBillboard(FcBillboard&&) = delete;
     FcBillboard& operator=(FcBillboard&&) = delete;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MODIFIERS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void loadTexture(FcRenderer& renderer, float width, float height, std::string_view filename);
     inline void setPosition(const glm::vec4& position) { mPush.position = position; }
     inline void setPosition(const float x, const float y, const float z)
      { mPush.position.x = x; mPush.position.y = y; mPush.position.z = z; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline const BillboardPushes& PushComponent() { return mPush; }
     inline const glm::vec3 Position() const { return mPush.position; }
  };

}// --- namespace fc --- (END)
