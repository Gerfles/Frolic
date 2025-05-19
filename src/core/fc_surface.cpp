// fc_surface.cpp
#include "fc_surface.hpp"
#include "core/utilities.hpp"
//

namespace fc
{

  // TODO should probably hash the FcSurface for quicker lookup so we can update the surface within the draw collection by using a simple hash lookup
  FcSurface::FcSurface(const FcSubMesh& surface, FcMeshNode* meshNode)
  {
    firstIndex = surface.startIndex;
    indexCount = surface.indexCount;
    bounds = surface.bounds;
    indexBuffer = meshNode->mMesh->IndexBuffer();
    transform = meshNode->worldTransform;
    // BUG this won't get properly updated when model is transformed,
    // need to use reference or otherwise update
    invModelMatrix = glm::inverse(glm::transpose(meshNode->worldTransform));
    vertexBufferAddress = meshNode->mMesh->VertexBufferAddress();
  }

  // TODO inline functions like this
  void FcSurface::bindIndexBuffer(VkCommandBuffer cmd) const
  {
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  }


  bool FcSurface::isVisible(const glm::mat4& viewProjection)
  {
    std::array<glm::vec3, 8> corners { glm::vec3{1.0f, 1.0f, 1.0f}
                                     , glm::vec3{1.0f, 1.0f, -1.0f}
                                     , glm::vec3{1.0f, -1.0f, 1.0f}
                                     , glm::vec3{1.0f, -1.0f, -1.0f}
                                     , glm::vec3{-1.0f, 1.0f, 1.0f}
                                     , glm::vec3{-1.0f, 1.0f, -1.0f}
                                     , glm::vec3{-1.0f, -1.0f, 1.0f}
                                     , glm::vec3{-1.0f, -1.0f, -1.0f} };

    glm::mat4 matrix = viewProjection * transform;

    glm::vec3 min = {1.5f, 1.5f, 1.5f};
    glm::vec3 max = {-1.5f, -1.5f, -1.5f};

    for (int corner = 0; corner < 8; corner++)
    {
      // project each corner into clip space
      glm::vec4 vector = matrix * glm::vec4(bounds.origin
                                            + corners[corner] * bounds.extents, 1.f);
      // perspective correction
      vector.x = vector.x / vector.w;
      vector.y = vector.y / vector.w;
      vector.z = vector.z / vector.w;

      min = glm::min(glm::vec3 {vector.x, vector.y, vector.z}, min);
      max = glm::max(glm::vec3 {vector.x, vector.y, vector.z}, max);
    }

    // check the clip space box is within the view
    if (max.x < -1.0f || min.x > 1.0f || max.y < -1.f || min.y > 1.f || max.z < 0.f || min.z > 1.f)
    {
      return false;
    }

    return true;
  }


}// --- namespace fc --- (END)
