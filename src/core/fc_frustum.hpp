// fc_frustum.hpp
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/glm.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>

namespace fc
{

  // TODO!!! FIX CLANGDs AUTO FORMAT!
  // TODO fix this and all enums that need to be made into enum class
enum Sides {
  RIGHT = 1,
  LEFT = 0,
  TOP = 2,
  BOTTOM = 3,
  FRONT = 5,
  BACK = 4,
  NUM_SIDES = 6
  };

  class FcFrustum
  {
   private:
     std::array<glm::vec4, 6> planes;

   public:
     // TODO determine if there is a way to set the projection once and then only pass
     // view matrix
     void update(const glm::mat4& ViewProjection);
     void normalize();
     // TODO check the models in fcRenderer using this instead of hard coded
     bool checkSphere(glm::vec3& pos, float radius);
     const std::array<glm::vec4, 6>& Planes() const { return planes; }
  };

}// --- namespace fc --- (END)
