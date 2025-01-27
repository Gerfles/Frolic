#include "fc_camera.hpp"

#include "fc_player.hpp"
#include "core/utilities.hpp"
#include <limits>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"


namespace fc
{
  void FcCamera::update(FcPlayer& player)
  {
    mPosition = player.position();
    mVelocity = player.velocity();
    mCameraRotation = player.rotationMatrix();
    // glm::mat4 cameraRotation = getRotationMatrix();
    // mPosition += glm::vec3(cameraRotation * glm::vec4(mVelocity * 0.5f, 0.f));
  }

  glm::mat4 FcCamera::getViewMatrix()
  {
    // to create a correct model view, we need to move the world in the opposite direction
    // to the camera. So first create camera model matrix, then invert
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), mPosition);
    // glm::mat4 cameraRotation = getRotationMatrix();
    glm::mat4 inverseViewMatrix = (cameraTranslation * mCameraRotation);


    // COOL
    // inverseViewMatrix[1][1] *= -1;

    //glm::vec3
    mInverseView = inverseViewMatrix[3];
    //mInverseView = glm::vec3(inverseViewMatrix[3].x, inverseViewMatrix[3].y, inverseViewMatrix[3].z);
    //glm::vec3 mInverseView = glm::vec3(0,0,0);
    return glm::inverse(inverseViewMatrix);
    //return glm::inverse(cameraTranslation * mCameraRotation);
  }

  [[deprecated("use FcPlayer to update camera")]]
  glm::mat4 FcCamera::getRotationMatrix()
  {
    // fairly typical FPS style camera. We join the pitch and yaw rotations into
    // the final rotation matrix
    glm::quat pitchRotation = glm::angleAxis(mPitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yawRotation = glm::angleAxis(mYaw, glm::vec3{0.f, -1.f, 0.f});

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
  }


  void FcCamera::setOrthographicProjection(float left, float right, float top
                                           , float bottom, float near, float far)
  {
     // TODO compbine into one fcall
    mProjectionMatrix = glm::mat4{1.0f};
    mProjectionMatrix[0][0] = 2.f / (right - left);
    mProjectionMatrix[1][1] = 2.f / (bottom - top);
    mProjectionMatrix[2][2] = 1.f / (far - near);
    mProjectionMatrix[3][0] = -(right + left) / (right - left);
    mProjectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    mProjectionMatrix[3][2] = -near / (far - near);

  }


  void FcCamera::setPerspectiveProjection(float fovY, float aspect, float near, float far)
  {
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon() > 0.0f));

    float g = 1.0f / tan(fovY * 0.5);
    float k = far / (far - near);

    mProjectionMatrix = glm::mat4(g / aspect,  0.0f,   0.0f,   0.0f,
                                  0.0f,  g,      0.0f,   0.0f,
                                  0.0f,  0.0f,   k,     1.0f,
                                  0.0f,  0.0f,    -near * k,   0.0f);
  }


// TODO set and return camera matrices in once function call
  void FcCamera::setViewDirection(glm::vec3 position, glm::vec3 target, glm::vec3 up)
  {
     // NOTE1: this originally had--const glm::vec3 w{glm::normalize(target)};
    const glm::vec3 w{glm::normalize(target - position)};
    const glm::vec3 u{glm::normalize(glm::cross(w, up))};
    const glm::vec3 v{glm::cross(w, u)};

     // TODO initialize view matrix in single call
    mViewMatrix = glm::mat4{1.f};
    mViewMatrix[0][0] = u.x;
    mViewMatrix[1][0] = u.y;
    mViewMatrix[2][0] = u.z;
    mViewMatrix[0][1] = v.x;
    mViewMatrix[1][1] = v.y;
    mViewMatrix[2][1] = v.z;
    mViewMatrix[0][2] = w.x;
    mViewMatrix[1][2] = w.y;
    mViewMatrix[2][2] = w.z;
    mViewMatrix[3][0] = -glm::dot(u, position);
    mViewMatrix[3][1] = -glm::dot(v, position);
    mViewMatrix[3][2] = -glm::dot(w, position);

    // ?? Not sure if I'll need but would require a member inverseView matrix
    // mInverseViewMatrix = glm::mat4{1.f};
    // mInverseViewMatrix[0][0] = u.x;
    // mInverseViewMatrix[0][1] = u.y;
    // mInverseViewMatrix[0][2] = u.z;
    // mInverseViewMatrix[1][0] = v.x;
    // mInverseViewMatrix[1][1] = v.y;
    // mInverseViewMatrix[1][2] = v.z;
    // mInverseViewMatrix[2][0] = w.x;
    // mInverseViewMatrix[2][1] = w.y;
    // mInverseViewMatrix[2][2] = w.z;
    // mInverseViewMatrix[3][0] = position.x;
    // mInverseViewMatrix[3][1] = position.y;
    // mInverseViewMatrix[3][2] = position.z;
  }


   // note that this is originally different than the above because of NOTE1
   // TODO determine if we want two distinct functions and what they would accomplish differently
  void FcCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up)
  {
    setViewDirection(position, target - position, up);
  }


  void FcCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation)
  {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);

    const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};

     // TODO set view matrix in w/ one call!
    mViewMatrix = glm::mat4{1.f};
    mViewMatrix[0][0] = u.x;
    mViewMatrix[1][0] = u.y;
    mViewMatrix[2][0] = u.z;
    mViewMatrix[0][1] = v.x;
    mViewMatrix[1][1] = v.y;
    mViewMatrix[2][1] = v.z;
    mViewMatrix[0][2] = w.x;
    mViewMatrix[1][2] = w.y;
    mViewMatrix[2][2] = w.z;
    mViewMatrix[3][0] = -glm::dot(u, position);
    mViewMatrix[3][1] = -glm::dot(v, position);
    mViewMatrix[3][2] = -glm::dot(w, position);

    // ?? Not sure if I'll need but would require a member inverseView matrix
    // mInverseViewMatrix = glm::mat4{1.f};
    // mInverseViewMatrix[0][0] = u.x;
    // mInverseViewMatrix[0][1] = u.y;
    // mInverseViewMatrix[0][2] = u.z;
    // mInverseViewMatrix[1][0] = v.x;
    // mInverseViewMatrix[1][1] = v.y;
    // mInverseViewMatrix[1][2] = v.z;
    // mInverseViewMatrix[2][0] = w.x;
    // mInverseViewMatrix[2][1] = w.y;
    // mInverseViewMatrix[2][2] = w.z;
    // mInverseViewMatrix[3][0] = position.x;
    // mInverseViewMatrix[3][1] = position.y;
    // mInverseViewMatrix[3][2] = position.z;
  }

}
