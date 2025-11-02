// fc_surface.cpp
#include "fc_surface.hpp"
#include "core/platform.hpp"

//

namespace fc
{

  // TODO should probably hash the FcSurface for quicker lookup so we can update the surface within the draw collection by using a simple hash lookup
  FcSurface::FcSurface(const FcSubMesh& surface, FcMeshNode* meshNode)
  {
    mFirstIndex = surface.startIndex;
    mIndexCount = surface.indexCount;


    // TODO this seems like a bad idea to Copy this data when it really should only be used in one class
    // This should be done when the gltf file is initially loaded.
    mBounds = surface.bounds;
    mBoundaryBox.init(mBounds);



    mIndexBuffer = meshNode->mMesh->IndexBuffer();
    mTransform = meshNode->worldTransform;
    // BUG this won't get properly updated when model is transformed,
    // need to use reference or otherwise update
    mInvModelMatrix = glm::inverse(glm::transpose(meshNode->worldTransform));
    mVertexBufferAddress = meshNode->mMesh->VertexBufferAddress();
  }

  // TODO inline functions like this
  void FcSurface::bindIndexBuffer(VkCommandBuffer cmd) const
  {
    vkCmdBindIndexBuffer(cmd, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
  }


  bool FcSurface::isVisible(const glm::mat4& viewProjection)
  {
    glm::mat4 matrix = viewProjection * mTransform;

    glm::vec3 min = {1.5f, 1.5f, 1.5f};
    glm::vec3 max = {-1.5f, -1.5f, -1.5f};

    for (int corner = 0; corner < 8; corner++)
    {
      // project each corner into clip space
      glm::vec4 vector = matrix * mBoundaryBox[corner];

      // perspective correction
      vector.x = vector.x / vector.w;
      vector.y = vector.y / vector.w;
      vector.z = vector.z / vector.w;

      min = glm::min(glm::vec3 {vector.x, vector.y, vector.z}, min);
      max = glm::max(glm::vec3 {vector.x, vector.y, vector.z}, max);
    }

    // check the clip space box is within the view
    // BUG this falsly rejects some surfaces that are actually visible
    if (max.x > -1.0f && min.x < 1.0f && max.y > -1.f && min.y < 1.f && max.z > 0.f && min.z < 1.f)
    {
      return false;
      // TODO remove the following BUG this algorithm is inefficient and inprecise
      // if ((max.x > -1.0f && min.x < 1.0f) && (max.y > -1.f && min.y < 1.f))
      // {
      //   return false;
      // }
    }

    // TEST Alternative to profile but would think the previous is faster
    // if (max.x < -1.0f || min.x > 1.0f || max.y < -1.f || min.y > 1.f || max.z < 0.f || min.z > 1.f)
    // {
    //   return false;
    // }

    return true;
  }

  //
  //
  const bool FcSurface::isInBounds(const glm::vec4& position) const
  {
    glm::vec3 min = {FLOAT_MAX, FLOAT_MAX, FLOAT_MAX};
    glm::vec3 max = {FLOAT_MIN, FLOAT_MIN, FLOAT_MIN};

    for (size_t cornerIndex = 0; cornerIndex < 8; cornerIndex++)
    {
      // project each corner into clip space
      glm::vec4 corner = mTransform * mBoundaryBox[cornerIndex];

      min = glm::min(glm::vec3 {corner.x, corner.y, corner.z}, min);
      max = glm::max(glm::vec3 {corner.x, corner.y, corner.z}, max);
    }

    if (position.x < max.x && position.x > min.x &&
        position.y < max.y && position.y > min.y &&
        position.z < max.z && position.z > min.z)
    {
      return false;
    }

    return true;
  }


}// --- namespace fc --- (END)
