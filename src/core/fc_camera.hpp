//

#pragma once

#include "core/fc_game_object.hpp"
#include "glm/mat4x4.hpp"

namespace fc
{
  
  class FcCamera
  {
   private:
     glm::mat4 mViewMatrix{1.0f};
     glm::mat4 mProjectionMatrix{1.0f};
     glm::mat4 mInverseViewMatrix{1.0f};

   public:
     void setOrthographicProjection(float left, float right, float top
                                                , float bottom, float near, float far);
     void setPerspectiveProjection(float fovY, float aspect, float near, float far);
     void setViewDirection(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
     void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
     void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

      // - GETTERS -
     glm::mat4& View() { return mViewMatrix; }
     glm::mat4& Projection() { return mProjectionMatrix; }
     const glm::mat4& InverseView() const { return mInverseViewMatrix; }
     const glm::vec3 Position() const { return glm::vec3(mInverseViewMatrix[3]); }
//     TransformComponent& Transform() { return mTransform; }
  };



  
} // _END_ namespace fc
