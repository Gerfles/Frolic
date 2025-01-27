#pragma once

//#include "core/fc_game_object.hpp"
#include "glm/mat4x4.hpp"


namespace fc
{
  class FcPlayer;

  class FcCamera
  {
   private:
     glm::mat4 mViewMatrix{1.0f};
     glm::vec4 mInverseView{1.0f};
     glm::mat4 mProjectionMatrix{1.0f};
     //glm::mat4 mInverseViewMatrix{1.0f};
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     glm::vec3 mVelocity{0.f};
     glm::vec3 mPosition{0.f};
     // vertical rotation
     float mPitch{0.f};
     // Horizontal rotation
     float mYaw{0.f};
     glm::mat4 mCameraRotation{1.0f};

     public:
     //void processSDLEvent(SDL_Event& e);

     // TODO should initialize the camera and allow a bind function call that binds the
     // camera to an object (ie player, rail, bad guy, etc)
     void update(FcPlayer& player);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void setOrthographicProjection(float left, float right, float top
                                                , float bottom, float near, float far);
     void setPerspectiveProjection(float fovY, float aspect, float near, float far);
     void setViewDirection(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
     void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
     void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

      // - GETTERS -
     glm::mat4 getRotationMatrix();
     // TODO find out which view return method is better
     glm::mat4 getViewMatrix();
     //glm::mat4& View() { return mViewMatrix; }
     glm::mat4& Projection() { return mProjectionMatrix; }
     const glm::vec4& InverseView() const { return mInverseView; }
     const glm::vec4 Position() const { return glm::vec4(mPosition, 1.0f); }
//     TransformComponent& Transform() { return mTransform; }
  };




} // _END_ namespace fc
