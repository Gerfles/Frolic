#include "fc_player.hpp"

#include "core/fc_camera.hpp"
#include "core/fc_game_object.hpp"
// LIBRARIES
#include <limits>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>


namespace fc
{

  void FcPlayer::move(float dt, FcCamera& camera)
  {
    glm::vec3 rotate{0};
     // TODO find a way to make this branchless... maybe fill an array with 0 or 1 and multiply all
     // actions by that factor without if statements
    if (mInput.keyDown(keys.lookRight)) rotate.y = 100.5f;
    if (mInput.keyDown(keys.lookLeft)) rotate.y -= 6.f;
    if (mInput.keyDown(keys.lookUp)) rotate.x += 4.f;
    if (mInput.keyDown(keys.lookDown)) rotate.x -= 4.f;

    //rotate.x += static_cast<float>(mInput.getMouseX()) / 200.f;


     // make sure not to normalize a zero vector
     // epsilon is a better check since comparing a float to zero has its limitations
    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
    {
      mTransform.rotation += lookSpeed * dt * glm::normalize(rotate);
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
      mTransform.translation += moveSpeed * dt *glm::normalize(moveDir);
    }

     // TODO re-formulate this fucntion to efficiently change the camera parameters
    camera.setViewYXZ(mTransform.translation, mTransform.rotation);
  }


  void FcPlayer::setPosition(const glm::vec3& position)
  {
    mTransform.translation = position;
  }

} /// NAMESPACE lve ///
