// fc_surface.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_bounding_box.hpp"
#include "fc_mesh.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLR   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{
  class FcMeshNode;
}

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FC SURFACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{

  // TODO probably should make a class
  struct FcSurface
  {
     uint32_t mIndexCount;
     uint32_t mFirstIndex;
     VkBuffer mIndexBuffer;
     // TODO make reference members
     glm::mat4 mTransform;//{1.0f};
     glm::mat4 mInvModelMatrix;
     Bounds mBounds;
     BoundaryBox mBoundaryBox;
     VkDeviceAddress mVertexBufferAddress;
     // Constructor used when adding to draw collection
     FcSurface(const FcSubMesh& surface, FcMeshNode* meshNode);
     void bindIndexBuffer(VkCommandBuffer cmd) const;
     bool isVisible(const glm::mat4& viewProjection);
     const bool isInBounds(const glm::vec4& position) const;
  };

}// --- namespace fc --- (END)
