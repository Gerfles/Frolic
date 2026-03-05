//>--- fc_math.cpp ---<//
#include "fc_math.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
    glm::mat4 perspective(float fovDegrees, float width, float height, float near, float far)
  {
    //float inverseAspect = height / width;
    float oneOverTanX = 1.0f / tan(fovDegrees * DEG_TO_RAD_OVER_TWO_FACTOR);
    float negFarOverDepth = -far / (far - near);

    return glm::mat4 {
      height * oneOverTanX / width, 0, 0, 0
                                        , 0, -oneOverTanX, 0, 0
                                                            , 0, 0, negFarOverDepth, -1
                                                                                   , 0, 0, near * negFarOverDepth, 0 };
  }



  glm::mat4 orthographic(float left, float right, float bottom, float top, float near, float far )
  {
    glm::mat4 orthographicProjectionMatrix = {
      // Column 1
      2.0f / (right - left),
      0.0f,
      0.0f,
      0.0f,
      // Column 2
      0.0f,
      -2.0f / (bottom - top),
      0.0f,
      0.0f,
	// Column 3
      0.0f,
      0.0f,
       1.0f / (near - far),
      0.0f,
	// Column 4
      -(right + left) / (right - left),
      -(bottom + top) / (bottom - top),
      near / (near - far),
      1.0f
    };

    return orthographicProjectionMatrix;
  }


} // --- namespace fc --- (END)
