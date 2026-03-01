//>--- fc_player.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_camera.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_scancode.h"
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/quaternion.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcInput; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  class FcPlayer
  {
   private:
     struct KeyBindings
     {
        int moveLeft = SDL_SCANCODE_A;
        int moveRight = SDL_SCANCODE_D;
        int moveForward = SDL_SCANCODE_W;
        int moveBackward = SDL_SCANCODE_S;
        int moveUp = SDL_SCANCODE_E;
        int moveDown = SDL_SCANCODE_Q;
        int lookLeft = SDL_SCANCODE_LEFT;
        int lookRight = SDL_SCANCODE_RIGHT;
        int lookUp = SDL_SCANCODE_UP;
        int lookDown = SDL_SCANCODE_DOWN;
     };

      // TODO add buttons for game controller
     struct ButtonBindings
     {
        int moveLeft = SDL_SCANCODE_LEFT;
     };

     KeyBindings keys{};
     float mMoveSpeed{5.f};
     float mLookSpeed{0.01f};
     FcInput* pInput;
     glm::vec3 mPosition{0.f};
     glm::vec3 mForwardDir;
     glm::vec3 mVelocity{0.f};
     float mYaw{0.f};
     float mPitch{0.f};
     glm::quat mYawRotation;
     glm::quat mPitchRotation;
     glm::mat4 mRotationMatrix {1.0f};
     FcCamera mCamera{mRotationMatrix, mPosition, mPosition, mPosition};

public:
     FcPlayer(FcInput* input) : pInput{input} {}
     FcPlayer() {};// = default;
     void init(FcInput* input) { pInput = input; }
     //
     void move(float dt);
     void moveNew(float dt);
     void moveOLD(float dt);
     FcCamera& Camera() { return mCamera; }
     void setPosition(const glm::vec3& position);
     const glm::vec3 position() const { return mPosition; }
     const glm::vec3 velocity() const { return mVelocity; }
     const float& getMoveSpeed() const { return mMoveSpeed; }
     void setMoveSpeed(const float moveSpeed) { mMoveSpeed = moveSpeed; }
     //
     float lookSpeed() { return mLookSpeed; }
     const glm::mat4 rotationMatrix() const { return mRotationMatrix; }

  };


}// --- namespace fc --- (END)
