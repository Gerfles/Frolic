// fc_frustum.cpp
#include "fc_frustum.hpp"

namespace fc
{
  void FcFrustum::update(const glm::mat4& viewProjection)
  {
    planes[RIGHT].x = viewProjection[0].w - viewProjection[0].x;
    planes[RIGHT].y = viewProjection[1].w - viewProjection[1].x;
    planes[RIGHT].z = viewProjection[2].w - viewProjection[2].x;
    planes[RIGHT].w = viewProjection[3].w - viewProjection[3].x;
    //
    planes[LEFT].x = viewProjection[0].w + viewProjection[0].x;
    planes[LEFT].y = viewProjection[1].w + viewProjection[1].x;
    planes[LEFT].z = viewProjection[2].w + viewProjection[2].x;
    planes[LEFT].w = viewProjection[3].w + viewProjection[3].x;
    //
    planes[TOP].x = viewProjection[0].w - viewProjection[0].y;
    planes[TOP].y = viewProjection[1].w - viewProjection[1].y;
    planes[TOP].z = viewProjection[2].w - viewProjection[2].y;
    planes[TOP].w = viewProjection[3].w - viewProjection[3].y;
    //
    planes[BOTTOM].x = viewProjection[0].w + viewProjection[0].y;
    planes[BOTTOM].y = viewProjection[1].w + viewProjection[1].y;
    planes[BOTTOM].z = viewProjection[2].w + viewProjection[2].y;
    planes[BOTTOM].w = viewProjection[3].w + viewProjection[3].y;
    //
    planes[FRONT].x = viewProjection[0].w - viewProjection[0].z;
    planes[FRONT].y = viewProjection[1].w - viewProjection[1].z;
    planes[FRONT].z = viewProjection[2].w - viewProjection[2].z;
    planes[FRONT].w = viewProjection[3].w - viewProjection[3].z;
    //
    planes[BACK].x = viewProjection[0].w + viewProjection[0].z;
    planes[BACK].y = viewProjection[1].w + viewProjection[1].z;
    planes[BACK].z = viewProjection[2].w + viewProjection[2].z;
    planes[BACK].w = viewProjection[3].w + viewProjection[3].z;
  }


  // ?? TODO should make this part of update if we end up needing to normalize for all ops
  void FcFrustum::normalize()
  {
    for(size_t i = 0; i < NUM_SIDES; ++i)
    {
      float length = std::sqrtf(planes[i].x * planes[i].x
                                + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
      planes[i] /= length;
    }
  }


  bool FcFrustum::checkSphere(glm::vec3& pos, float radius)
  {
    for(size_t i = 0; i < NUM_SIDES; ++i)
    {
      if ((planes[i].x * pos.x) + (planes[i].y * pos.y) + (planes[i].z * pos.z)
          + planes[i].w <= -radius)
      {
        return false;
      }
    }
    return true;
  }
}// --- namespace fc --- (END)
