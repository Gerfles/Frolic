#include "fc_player.hpp"

#include "core/fc_camera.hpp"
#include "core/fc_game_object.hpp"
#include "core/platform.hpp"
// LIBRARIES
#include <limits>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>

namespace fc
{

  // TODO modify moveOLD to incorporate mouse and see how it performs
  void FcPlayer::move(float dt)
  {
    // TODO find a way to make this branchless... maybe fill an array with 0 or 1 and multiply all
    // actions by that factor without if statements
    if (mInput.keyDown(keys.moveForward)) mVelocity.z = -1;
    if (mInput.keyDown(keys.moveBackward)) mVelocity.z = 1;
    if (mInput.keyDown(keys.moveRight)) mVelocity.x = 1;
    if (mInput.keyDown(keys.moveLeft)) mVelocity.x = -1;
    // TODO make move up and move down be independent of the view direction
    if (mInput.keyDown(keys.moveUp)) mVelocity.y = 1;
    if (mInput.keyDown(keys.moveDown)) mVelocity.y = -1;

    if (mInput.keyUp(keys.moveForward)) mVelocity.z = 0;
    if (mInput.keyUp(keys.moveBackward)) mVelocity.z = 0;
    if (mInput.keyUp(keys.moveRight)) mVelocity.x = 0;
    if (mInput.keyUp(keys.moveLeft)) mVelocity.x = 0;
    if (mInput.keyUp(keys.moveUp)) mVelocity.y = 0;
    if (mInput.keyUp(keys.moveDown)) mVelocity.y = 0;

    // rotation via key
    if (mInput.keyDown(keys.lookRight)) mYaw += 100 * dt * mLookSpeed;
    if (mInput.keyDown(keys.lookLeft)) mYaw -= 100 * dt * mLookSpeed;


    if (mInput.mouseHit(mInput.MOUSE_RIGHT))
    {
      mEnableLookAround = !mEnableLookAround;
    }


    if (mInput.mouseInWindow() && mEnableLookAround)
    {
      int mouseX, mouseY;
      mInput.RelativeMousePosition(mouseX, mouseY);

      mYaw += mouseX * mLookSpeed * dt;
      mPitch -= mouseY * mLookSpeed * dt;

      // Limit looking up and down range
      mPitch = glm::clamp(mPitch, -1.5f, 1.5f);
    }


    // Keep from overflowing mYaw but may just check to see if that's a big  deal
    // if (mYaw > TWO_PI)
    // {
    //   mYaw -= TWO_PI;
    // }
    // if (mYaw < -TWO_PI)
    // {
    //   mYaw += TWO_PI;
    // }




    glm::quat pitchRotation = glm::angleAxis(mPitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yawRotation = glm::angleAxis(mYaw, glm::vec3{0.f, -1.f, 0.f});

    mRotationMatrix = glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);

    mPosition += glm::vec3(mRotationMatrix * glm::vec4(mVelocity * mMoveSpeed, 0.f)) * dt;
  }


  // TODO compare this method with the new one and performance TEST
  void FcPlayer::moveOLD(float dt)
  {
    glm::vec3 rotate{0};
    // TODO find a way to make this branchless... maybe fill an array with 0 or 1 and multiply all
    // actions by that factor without if statements
    if (mInput.keyDown(keys.lookRight)) rotate.y = 100.5f;
    if (mInput.keyDown(keys.lookLeft)) rotate.y -= 6.f;
    if (mInput.keyDown(keys.lookUp)) rotate.x += 4.f;
    if (mInput.keyDown(keys.lookDown)) rotate.x -= 4.f;



    // make sure not to normalize a zero vector
    // epsilon is a better check since comparing a float to zero has its limitations
    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
    {
      mTransform.rotation += mLookSpeed * dt * glm::normalize(rotate);
    }

    // limit pitch values between about +/- 85ish degrees
    mTransform.rotation.x = glm::clamp(mTransform.rotation.x, -1.5f, 1.5f);

    mTransform.rotation.y = glm::mod(mTransform.rotation.y, glm::two_pi<float>());

    float yaw = mTransform.rotation.y;
    const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
    const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
    const glm::vec3 upDir{0.f, -1.f, 0.f};

    // move direction
    glm::vec3 moveDir{0.f};
    if (mInput.keyDown(keys.moveForward)) moveDir += forwardDir;
    if (mInput.keyDown(keys.moveBackward)) moveDir -= forwardDir;
    if (mInput.keyDown(keys.moveRight)) moveDir += rightDir;
    if (mInput.keyDown(keys.moveLeft)) moveDir -= rightDir;
    if (mInput.keyDown(keys.moveUp)) moveDir += upDir;
    if (mInput.keyDown(keys.moveDown)) moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
    {
      mTransform.translation += mMoveSpeed * dt *glm::normalize(moveDir);
    }

    // TODO re-formulate this fucntion to efficiently change the camera parameters
    //camera.setViewYXZ(mTransform.translation, mTransform.rotation);
  }

  void FcPlayer::setPosition(const glm::vec3& position)
  {
    mPosition = position;
  }

  } /// NAMESPACE lve ///
