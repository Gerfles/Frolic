#pragma once

// core
#include "core/fc_camera.hpp"
#include "fc_input.hpp"
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
     float moveSpeed{2.f};
     float lookSpeed{1.5f};
     FcInput& mInput;
     TransformComponent mTransform{};

public:
     FcPlayer(FcInput& input) : mInput{input} {}
      //
     void move(float dt, FcCamera& camera);
     void setPosition(const glm::vec3& position);

  };


} /// NAMESPACE lve ///
