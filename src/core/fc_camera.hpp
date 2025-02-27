#pragma once

//#include "core/fc_game_object.hpp"
#include "glm/mat4x4.hpp"

// TODO lots of redundancies with fc_player, may want to couple better
namespace fc
{
  class FcPlayer;

  class FcCamera
  {
   private:
     glm::mat4 mViewMatrix;
     glm::vec4 mInverseView;
     glm::mat4 mProjectionMatrix;
     //glm::mat4 mInverseViewMatrix{1.0f};
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     glm::vec3& mPosition; // EYE
     glm::vec3& mTarget; //
     glm::vec3 mUp{0.f, -1.f, 0.f}; //

     // glm::vec3 mVelocity{0.f};

     // // TODO might delete
     // float mPitch{0.f};     // vertical rotation
     // float mYaw{0.f};      // Horizontal rotation

     glm::mat4& mCameraRotation;

     public:
     //void processSDLEvent(SDL_Event& e);
     //FcCamera(){};
     FcCamera(glm::mat4& rotationMartrix, glm::vec3& position, glm::vec3& target, glm::vec3 up)
       : mCameraRotation{rotationMartrix}, mPosition{position}, mTarget{target}, mUp{up} {};
     // TODO should initialize the camera and allow a bind function call that binds the
     // camera to an object (ie player, rail, bad guy, etc)


     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void setOrthographicProjection(float left, float right, float top
                                                , float bottom, float near, float far);
     void setPerspectiveProjection(float fovDegrees, float width, float height, float near, float far);
     void setViewDirection(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
     void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
     void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

      // - GETTERS -
     glm::mat4 getRotationMatrix();
     // TODO find out which view return method is better
     glm::mat4 getViewMatrix();
     glm::mat4 getUvnViewMatrix();
     //glm::mat4& View() { return mViewMatrix; }
     glm::mat4& Projection() { return mProjectionMatrix; }
     const glm::vec4& InverseView() const { return mInverseView; }
     const glm::vec3 Position() const { return mPosition; }
     const glm::vec3 Target() const { return mTarget; }
//     TransformComponent& Transform() { return mTransform; }
  };




} // _END_ namespace fc
