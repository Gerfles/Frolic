// fc_surface.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_bounding_box.hpp"
#include "fc_buffer.hpp"
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
  class FcSurface
  {
   private:
     // ?? ?? for some reason we cant call the following class with fastgltf::mesh.name
     // since it uses an std::pmr::string that only seems to be able to bind to a public
     // class member?? TODO researce PMR
     // std::string mName;
     glm::mat4 mTransform;
     glm::mat4 mInvModelMatrix;
     VkDeviceAddress mVertexBufferAddress;

     uint32_t mIndexCount;
     uint32_t mFirstIndex;
     FcBuffer mIndexBuffer;
     FcBuffer mVertexBuffer;



   public:


     Bounds mBounds;
     BoundaryBox mBoundaryBox;

     // TODO think about storing separate surface subMeshes based on material type / pipeline
     // pair data structure so we can just iterate through the whole thing
     std::vector<FcSubMesh> mMeshes;
     // Constructor used when adding to draw collection
     // -*-*-*-*-*-*-*-*-*-*-*-*-   CONSTRUCTORS / CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*- //
     FcSurface(const FcSubMesh& surface, FcMeshNode* meshNode);
     //
     FcSurface() = default;
     // TODO should think about adding destructors on objects that call destroy() if needed.
     void destroy();
     // FIXME could help eliminate memory leaks since objects that go out of scope should also
     // be destroyed
     /* ~FcSurface() { destroy(); } */

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline const void bindIndexBuffer(VkCommandBuffer cmd) const
      { vkCmdBindIndexBuffer(cmd, mIndexBuffer.getVkBuffer(), 0, VK_INDEX_TYPE_UINT32); }
     //
     bool isVisible(const glm::mat4& viewProjection);
     //
     const bool isInBounds(const glm::vec4& position) const;
     //
     template <typename T> void uploadMesh(std::span<T> vertices, std::span<uint32_t> indices);
     //
     inline void setTransform(glm::mat4& mat)
      { mTransform = mat; }
     //

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     /* inline const VkBuffer VertexBuffer() const { return mVertexBuffer.getVkBuffer(); } */
     //
     inline const VkBuffer IndexBuffer() const { return mIndexBuffer.getVkBuffer(); }
     //
     inline const uint32_t IndexCount() const { return mIndexCount; }
     //
     inline const VkDeviceAddress VertexBufferAddress() const { return mVertexBufferAddress; }
     //
     inline const uint32_t FirstIndex() const { return mFirstIndex; }
     //
     inline glm::mat4 ModelMatrix() const { return mTransform; }
     //
     inline glm::mat4 InvModelMatrix() const { return mInvModelMatrix; }
     // TODO improve usefulness or eliminate
     inline const glm::mat4* Transform() const { return &mTransform;}
  };

}// --- namespace fc --- (END)
