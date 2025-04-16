#include "fc_player.hpp"

#include "core/fc_camera.hpp"
#include "core/fc_game_object.hpp"
#include "core/platform.hpp"
// LIBRARIES
#include <limits>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace fc
{

  // TODO modify moveOLD to incorporate mouse and see how it performs
  void FcPlayer::move(float dt)
  {
    // TODO find a way to make this branchless... maybe fill an array with 0 or 1 and multiply all
    // actions by that factor without if statements
    if (pInput->keyDown(keys.moveForward)) mVelocity.z = -1;
    if (pInput->keyDown(keys.moveBackward)) mVelocity.z = 1;
    if (pInput->keyDown(keys.moveRight)) mVelocity.x = 1;
    if (pInput->keyDown(keys.moveLeft)) mVelocity.x = -1;
    // TODO make move up and move down be independent of the view direction
    if (pInput->keyDown(keys.moveUp)) mVelocity.y = 1;
    if (pInput->keyDown(keys.moveDown)) mVelocity.y = -1;

    if (pInput->keyUp(keys.moveForward)) mVelocity.z = 0;
    if (pInput->keyUp(keys.moveBackward)) mVelocity.z = 0;
    if (pInput->keyUp(keys.moveRight)) mVelocity.x = 0;
    if (pInput->keyUp(keys.moveLeft)) mVelocity.x = 0;
    if (pInput->keyUp(keys.moveUp)) mVelocity.y = 0;
    if (pInput->keyUp(keys.moveDown)) mVelocity.y = 0;

    // rotation via key
    if (pInput->keyDown(keys.lookRight)) mYaw += 100 * dt * mLookSpeed;
    if (pInput->keyDown(keys.lookLeft)) mYaw -= 100 * dt * mLookSpeed;


    if (pInput->mouseHit(pInput->MOUSE_RIGHT))
    {
      mLookSpeed = (mLookSpeed == 0.0f) ? 0.01f : 0.0f;
    }


    if (pInput->mouseInWindow())
    {
      int mouseX, mouseY;
      pInput->RelativeMousePosition(mouseX, mouseY);

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

    glm::quat pitch = glm::angleAxis(mPitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yaw = glm::angleAxis(mYaw, glm::vec3{0.f, -1.f, 0.f});

    mRotationMatrix = glm::toMat4(yaw * pitch);
    //mRotationMatrix = glm::toMat4(yaw)  * glm::toMat4(pitch);
    mPosition += glm::vec3(mRotationMatrix * glm::vec4(mVelocity * mMoveSpeed, 0.f)) * dt;
  }


  void FcPlayer::moveNew(float dt)
  {
    // TODO find a way to make this branchless... maybe fill an array with 0 or 1 and multiply all
    // actions by that factor without if statements

    if (pInput->mouseHit(pInput->MOUSE_RIGHT))
    {
      mLookSpeed = (mLookSpeed == 0.0f) ? 0.01f : 0.0f;
    }

    if (pInput->mouseInWindow())
    {
      int mouseX, mouseY;
      pInput->RelativeMousePosition(mouseX, mouseY);

      mYaw += mouseX * mLookSpeed * dt;
      mPitch += mouseY * mLookSpeed * dt;
    }

    // limit pitch values between about +/- 85ish degrees
    mPitch = glm::clamp(mPitch, -1.5f, 1.5f);

    mYaw = glm::mod(mYaw, glm::two_pi<float>());

    // make sure not to normalize a zero vector
    // epsilon is a better check since comparing a float to zero has its limitations
    mForwardDir.x = sin(mYaw) * sin(mPitch);
    mForwardDir.y = sin(mPitch);
    mForwardDir.z = cos(mYaw) * sin(mPitch);


    //glm::vec3 forwardDir{sin(mYaw), 0.f, -cos(mYaw)};
    mForwardDir = glm::normalize(mForwardDir);
    glm::vec3 upDir{0.f, -1.f, 0.f};
    glm::vec3 rightDir = glm::cross(mForwardDir, upDir);
    rightDir = glm::normalize(rightDir);
    upDir = glm::cross(rightDir, mForwardDir);
    /* const glm::vec3 rightDir{-forwardDir.z, 0.f, forwardDir.x}; */


    // move direction
    glm::vec3 moveDir{0.f};
    if (pInput->keyDown(keys.moveForward)) moveDir += mForwardDir;
    if (pInput->keyDown(keys.moveBackward)) moveDir -= mForwardDir;
    if (pInput->keyDown(keys.moveRight)) moveDir += rightDir;
    if (pInput->keyDown(keys.moveLeft)) moveDir -= rightDir;
    if (pInput->keyDown(keys.moveUp)) moveDir += upDir;
    if (pInput->keyDown(keys.moveDown)) moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
    {
      mPosition += mMoveSpeed * dt *glm::normalize(moveDir);
    }

    // TODO re-formulate this fucntion to efficiently change the camera parameters
    glm::vec3 rotate{mPitch, mYaw, 0.0f};
    mCamera.setViewYXZ(mPosition, -rotate);
  }



  // TODO compare this method with the new one and performance TEST
  void FcPlayer::moveOLD(float dt)
  {
    // TODO find a way to make this branchless... maybe fill an array with 0 or 1 and multiply all
    // actions by that factor without if statements
    if (pInput->keyDown(keys.lookRight)) mYaw = 1.f;
    if (pInput->keyDown(keys.lookLeft)) mYaw = -1.f;
    if (pInput->keyDown(keys.lookUp)) mPitch = 1.f;
    if (pInput->keyDown(keys.lookDown)) mPitch = -1.f;

 if (pInput->mouseHit(pInput->MOUSE_RIGHT))
    {
      mLookSpeed = (mLookSpeed == 0.0f) ? 0.01f : 0.0f;
    }

    if (pInput->mouseInWindow())
    {
      int mouseX, mouseY;
      pInput->RelativeMousePosition(mouseX, mouseY);

      mYaw -= 1.f * mouseX * mLookSpeed * dt;
      mPitch -= 1.f * mouseY * mLookSpeed * dt;
    }

    // limit pitch values between about +/- 85ish degrees
    mPitch = glm::clamp(mPitch, -1.5f, 1.5f);

    mYaw = glm::mod(mYaw, glm::two_pi<float>());

    // make sure not to normalize a zero vector
    // epsilon is a better check since comparing a float to zero has its limitations


    glm::vec3 forwardDir{sin(mYaw), 0.f, -cos(mYaw)};
    forwardDir = glm::normalize(forwardDir);
    glm::vec3 upDir{0.f, 1.f, 0.f};
    glm::vec3 rightDir = glm::cross(forwardDir, upDir);
    rightDir = glm::normalize(rightDir);
    upDir = glm::cross(forwardDir, rightDir);
    /* const glm::vec3 rightDir{-forwardDir.z, 0.f, forwardDir.x}; */


    // move direction
    glm::vec3 moveDir{0.f};
    if (pInput->keyDown(keys.moveForward)) moveDir += forwardDir;
    if (pInput->keyDown(keys.moveBackward)) moveDir -= forwardDir;
    if (pInput->keyDown(keys.moveRight)) moveDir += rightDir;
    if (pInput->keyDown(keys.moveLeft)) moveDir -= rightDir;
    if (pInput->keyDown(keys.moveUp)) moveDir += upDir;
    if (pInput->keyDown(keys.moveDown)) moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
    {
      mPosition += mMoveSpeed * dt *glm::normalize(moveDir);
    }

    // TODO re-formulate this fucntion to efficiently change the camera parameters
    glm::vec3 rotate{mPitch, mYaw, 0.0f};
    mCamera.setViewYXZ(mPosition, rotate);

    // //glm::vec3 rotate{0};
    // // TODO find a way to make this branchless... maybe fill an array with 0 or 1 and multiply all
    // // actions by that factor without if statements

    // // rotation via key
    // // if (mInput.keyDown(keys.lookRight)) rotate.y += 100 * dt * mLookSpeed;
    // // if (mInput.keyDown(keys.lookLeft)) rotate.y -= 100 * dt * mLookSpeed;

    // if (mInput.mouseHit(mInput.MOUSE_RIGHT))
    // {
    //   mEnableLookAround = !mEnableLookAround;
    // }

    // if (mInput.mouseInWindow() && mEnableLookAround)
    // {
    //   int mouseX, mouseY;
    //   mInput.RelativeMousePosition(mouseX, mouseY);

    //   mYaw += 1.f * mouseX * mLookSpeed * dt;
    //   mPitch -= 1.f * mouseY * mLookSpeed * dt;
    // }

    // // limit pitch values between about +/- 85ish degrees
    // mPitch = glm::clamp(mPitch, -1.5f, 1.5f);

    // mYaw = glm::mod(mYaw, glm::two_pi<float>());

    // // if (mInput.keyDown(keys.lookRight)) mYaw = 1.f;
    // // if (mInput.keyDown(keys.lookLeft)) mYaw = -1.f;
    // // if (mInput.keyDown(keys.lookUp)) mPitch = 1.f;
    // // if (mInput.keyDown(keys.lookDown)) mPitch = -1.f;

    // // make sure not to normalize a zero vector
    // // epsilon is a better check since comparing a float to zero has its limitations
    // // glm::vec3 rotate{mYaw, mPitch, 0.0f};
    // // if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
    // // {
    // //   rotate += mLookSpeed * dt * glm::normalize(rotate);
    // // }

    // //mYaw = rotate.y;
    // float sinPitch = sin(mPitch); // 0
    // float cosPitch = cos(mPitch);// 1
    // float sinYaw = sin(mYaw);// 0
    // float cosYaw = cos(mYaw);// 1

    // glm::vec3 forwardDir{sinPitch * cosYaw, sinPitch * sinYaw, cosPitch};
    // forwardDir = glm::normalize(forwardDir);
    // glm::vec3 upDir = glm::vec3(0,-1,0);
    // //glm::vec3 upDir{cosPitch * sinYaw, cosPitch * cosYaw, -sinPitch};
    // //upDir = glm::normalize(upDir);
    // glm::vec3 rightDir = glm::cross(forwardDir, upDir);
    // /* glm::vec3 rightDir{-sinYaw, cosYaw, 0.f}; */
    // rightDir = glm::normalize(rightDir);
    // // No need to normalize since both are perpendicular unit vectors
    // upDir = glm::cross(forwardDir, rightDir);


    // // move direction
    // glm::vec3 moveDir{0.f};
    // if (mInput.keyDown(keys.moveForward)) moveDir += forwardDir;
    // if (mInput.keyDown(keys.moveBackward)) moveDir -= forwardDir;
    // if (mInput.keyDown(keys.moveRight)) moveDir += rightDir;
    // if (mInput.keyDown(keys.moveLeft)) moveDir -= rightDir;
    // if (mInput.keyDown(keys.moveUp)) moveDir += upDir;
    // if (mInput.keyDown(keys.moveDown)) moveDir -= upDir;

    // if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
    // {
    //   //mTransform.translation += mMoveSpeed * dt *glm::normalize(moveDir);
    //   mPosition += mMoveSpeed * dt * glm::normalize(moveDir);
    // }

    // // TODO re-formulate this fucntion to efficiently change the camera parameters
    // // camera.setViewYXZ(mTransform.translation, mTransform.rotation);
    // glm::vec3 rotate(mYaw, mPitch, 0.f);
    // const float c3 = glm::cos(rotate.z);
    // const float s3 = glm::sin(rotate.z);
    // const float c2 = glm::cos(rotate.x);
    // const float s2 = glm::sin(rotate.x);
    // const float c1 = glm::cos(rotate.y);
    // const float s1 = glm::sin(rotate.y);

    // const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    // const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    // const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};

    //  // TODO set view matrix in w/ one call!
    // //mRotationMatrix = glm::mat4{1.f};
    // mRotationMatrix[0][0] = u.x;
    // mRotationMatrix[1][0] = u.y;
    // mRotationMatrix[2][0] = u.z;
    // mRotationMatrix[0][1] = v.x;
    // mRotationMatrix[1][1] = v.y;
    // mRotationMatrix[2][1] = v.z;
    // mRotationMatrix[0][2] = w.x;
    // mRotationMatrix[1][2] = w.y;
    // mRotationMatrix[2][2] = w.z;
    // mRotationMatrix[3][0] = -glm::dot(u, mPosition);//mTransform.translation);
    // mRotationMatrix[3][1] = -glm::dot(v, mPosition);//mTransform.translation);
    // mRotationMatrix[3][2] = -glm::dot(w, mPosition);//mTransform.translation);
  }


  void FcPlayer::setPosition(const glm::vec3& position)
  {
    mPosition = position;
  }

  } /// NAMESPACE lve ///
