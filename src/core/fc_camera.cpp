#include "fc_camera.hpp"

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"


namespace fc
{


  glm::mat4 FcCamera::getViewMatrix()
  {
    // to create a correct model view, we need to move the world in the opposite direction
    // to the camera. So first create camera model matrix, then invert
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), mPosition);

    //glm::mat4 cameraRotation = getRotationMatrix();
    //glm::mat4 inverseViewMatrix = (mCameraRotation * cameraTranslation);

    //glm::vec3
    //mInverseView = inverseViewMatrix[3];
    //mInverseView = glm::vec3(inverseViewMatrix[3].x, inverseViewMatrix[3].y, inverseViewMatrix[3].z);
    //glm::vec3 mInverseView = glm::vec3(0,0,0);

    //return glm::inverse(inverseViewMatrix);
    //glm::mat4 viewMatrix =

    return glm::inverse(cameraTranslation * mCameraRotation);
    // TRY
    //return glm::inverse(mCameraRotation * cameraTranslation);
    //return inverseViewMatrix;
  }


  glm::mat4 FcCamera::getUvnViewMatrix()
  {
    return mViewMatrix;
  }



  [[deprecated("use FcPlayer to update camera")]]
  glm::mat4 FcCamera::getRotationMatrix()
  {
    // fairly typical FPS style camera. We join the pitch and yaw rotations into
    // the final rotation matrix
    // glm::quat pitchRotation = glm::angleAxis(mPitch, glm::vec3{1.f, 0.f, 0.f});
    // glm::quat yawRotation = glm::angleAxis(mYaw, glm::vec3{0.f, -1.f, 0.f});

    // return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
    return glm::mat4{1.0f};
  }


  // TODO consolidate
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


    glm::mat4 orthographicProjectionMatrix = {
      // Column 1
      2.0f / (right - left),
      0.0f,
      0.0f,
      0.0f,
      // Column 2
      0.0f,
      -2.0f / (bottom - top),
      0.0f,
      0.0f,
	// Column 3
      0.0f,
      0.0f,
      1.0f / (near - far),
      0.0f,
	// Column 4
      -(right + left) / (right - left),
      -(bottom + top) / (bottom - top),
      near / (near - far),
      1.0f
    };

    mProjectionMatrix = orthographicProjectionMatrix;

    // TEST
    //mProjectionMatrix = glm::ortho(left, right, bottom, top, near, far);
  }


  void FcCamera::setPerspectiveProjection(float fovDegrees, float width, float height, float near, float far)
  {
    // assert(glm::abs(aspect - std::numeric_limits<float>::epsilon() > 0.0f));

    // OpenGL or...
    // float g = 1.0f / tan(fovY * 0.5);
    // float k = far / (far - near);

    // mProjectionMatrix = glm::mat4(g / aspect,  0.0f,   0.0f,   0.0f,
    //                               0.0f,  g,      0.0f,   0.0f,
    //                               0.0f,  0.0f,   k,     1.0f,
    //                               0.0f,  0.0f,    -near * k,   0.0f);

    // Vulkan Specific
    float oneOverTanX = 1.0f / tan(fovDegrees * DEG_TO_RAD_OVER_TWO_FACTOR);
    float negFarOverDepth = -far / (far - near);

    // TODO disable auto formatting
    mProjectionMatrix =  glm::mat4{
      height * oneOverTanX / width, 0, 0, 0
             , 0, -oneOverTanX, 0, 0
                      , 0, 0, negFarOverDepth, -1
                               , 0, 0, near * negFarOverDepth, 0 };
  }


// TODO set and return camera matrices in once function call
  void FcCamera::setViewDirection(glm::vec3 position, glm::vec3 target, glm::vec3 up)
  {
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


// Matrix corresponds to translate * Roty * Rotx * Rotz * scale transformation
   // Rotation convention uses tait-bryan (euler) angles with axis order Y(1), X(2), Z(3)
   // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   ALTERNATIVE ?? -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // return glm::mat4{
    //   {
    //     scale.x * (c1 * c3 + s1 * s2 * s3),
    //         scale.x * (c2 * s3),
    //         scale.x * (c1 * s2 * s3 - c3 * s1),
    //         0.0f,
    //     },
    //     {
    //         scale.y * (c3 * s1 * s2 - c1 * s3),
    //         scale.y * (c2 * c3),
    //         scale.y * (c1 * c3 * s2 + s1 * s3),
    //         0.0f,
    //     },
    //     {
    //         scale.z * (c2 * s1),
    //         scale.z * (-s2),
    //         scale.z * (c1 * c2),
    //         0.0f,
    //     },
    //     {translation.x, translation.y, translation.z, 1.0f}};




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
