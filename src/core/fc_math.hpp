//>--- fc_math.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // glm::vec3 operator+(const glm::vec4& v1, const glm::vec3& v2);
  // glm::vec3 operator+(const glm::vec3& v1, const glm::vec4& v2);
  // glm::vec3 operator-(const glm::vec4& v1, const glm::vec3& v2);
  // glm::vec3 operator-(const glm::vec3& v1, const glm::vec4& v2);
  glm::vec3 vec3FromAddMixed(const glm::vec3& v1, const glm::vec4& v2);
  glm::vec3 vec3FromAddMixed(const glm::vec4& v1, const glm::vec3& v2);
  glm::vec4 vec4FromAddMixed(const glm::vec3& v1, const glm::vec4& v2);
  glm::vec3 vec4FromAddMixed(const glm::vec4& v1, const glm::vec3& v2);
  glm::vec3 vec3FromSubtractMixed(const glm::vec3& v1, const glm::vec4& v2);
  //
  inline glm::vec3 vec3FromSubtractMixed(const glm::vec4& v1, const glm::vec3& v2)
  {
    return glm::vec3{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
    /* return glm::vec3{v2.x - v1.x, v2.y - v1.y, v2.z - v1.z}; */
  }
  //
  glm::vec4 vec4FromSubtractMixed(const glm::vec3& v1, const glm::vec4& v2);
  glm::vec3 vec4FromSubtractMixed(const glm::vec4& v1, const glm::vec3& v2);
}// --- namespace fc --- (END)
