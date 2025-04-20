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

  struct FcSurface
  {
     uint32_t indexCount;
     uint32_t firstIndex;
     VkBuffer indexBuffer;
     // TODO make reference members
     glm::mat4 transform;
     glm::mat4 invModelMatrix;
     Bounds bounds;
     VkDeviceAddress vertexBufferAddress;
     // Constructor used when adding to draw collection
     FcSurface(const FcSubMesh& surface, FcMeshNode* meshNode);
     void bindIndexBuffer(VkCommandBuffer cmd) const;
     bool isVisible(const glm::mat4& viewProjection);
  };

}// --- namespace fc --- (END)
