#pragma once

// core
#include "core/fc_camera.hpp"
#include "fc_input.hpp"
#include "fc_game_object.hpp"
// libs
#include "SDL2/SDL_scancode.h"
#include "SDL2/SDL_video.h"


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
     FcInput& mInput;
     glm::vec3 mPosition{0.f,0.f,0.f};
     glm::vec3 mVelocity{0.f,0.f,0.f};
     float mYaw{0};
     float mPitch{0};
     glm::mat4 mRotationMatrix;
     // TODO get rid of Transformcomponent -> not lightweight
     TransformComponent mTransform{};


public:
     FcPlayer(FcInput& input) : mInput{input} {}
      //
     void move(float dt);
     void moveOLD(float dt);
     void setPosition(const glm::vec3& position);
     const glm::vec3 position() const { return mPosition; }
     const glm::vec3 velocity() const { return mVelocity; }
     float& moveSpeed() { return mMoveSpeed; }
     float& lookSpeed() { return mLookSpeed; }
     const glm::mat4 rotationMatrix() const { return mRotationMatrix; }
  };


} /// NAMESPACE lve ///
