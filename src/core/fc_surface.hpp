// fc_surface.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_bounding_box.hpp"
#include "fc_buffer.hpp"
#include "fc_mesh.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


// *-*-*-*-*-*-*-*-*-*-*-*-*-   FC SURFACE & SUBMESH   *-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLR   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FcMeshNode;
  struct FcSubmesh;
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

  //
  //
  // TODO Can incrementally update push constants according to:
  // https://docs.vulkan.org/guide/latest/push_constants.html
  // May provide some benefits if done correctly
  // TODO this is too much data for some GPU push constant limits
  // ScenePCs are not created directly but instead are the values that the shader will recieve from
  // pushing a FcSurface object with vkCmdPushConstants and using this struct as the size of the
  // data sent. Then FcSurface must have the first members exactly match this structure. This will
  // save us from having to create a ScenePushConstant to
  // This structure should not be used directly but instead serves as a size indicator
  struct ScenePushConstants
  {
     glm::mat4 worldMatrix;
     glm::mat4 normalTransform;
     VkDeviceAddress vertexBuffer;

     ScenePushConstants() = delete;
  };


  //
  //
  class FcSurface
  {
   private:
     // *-*-*-*-*-*-*-*-*-   KEEP ALIGNED WITH SCENEPUSHCONSTANTS   *-*-*-*-*-*-*-*-*- //
     glm::mat4 mTransform;
     //
     glm::mat4 mInvModelMatrix;
     //
     VkDeviceAddress mVertexBufferAddress;
     // -*-*-*-*-*-*-*-*-*-*-*-   END ALIGNMENT REQUIREMENTS   -*-*-*-*-*-*-*-*-*-*-*- //

     // ?? ?? for some reason we cant call the following class with fastgltf::mesh.name
     // since it uses an std::pmr::string that only seems to be able to bind to a public
     // class member?? TODO researce PMR
     /* std::pmr::string mName; */
     FcBuffer mIndexBuffer;
     FcBuffer mVertexBuffer;
     // TODO think about storing separate surface subMeshes based on material type / pipeline
     // pair data structure so we can just iterate through the whole thing
     std::vector<FcSubmesh> mSubMeshes;
   public:

     // -*-*-*-*-*-*-*-*-*-*-*-*-   CONSTRUCTORS / CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*- //
     FcSurface() = default;
     /* ~FcSurface() { destroy(); } */
     void destroy();
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //
     inline bool operator==(const FcSurface& other) const
      {
        return (mVertexBufferAddress == other.mVertexBufferAddress);
      };
     //
     inline const void bindIndexBuffer(VkCommandBuffer cmd) const
      {
        vkCmdBindIndexBuffer(cmd, mIndexBuffer.getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);
      }
     //
     const bool isInBounds(const glm::vec4& position) const;
     //
     template <typename T> void uploadMesh(std::span<T> vertices, std::span<uint32_t> indices);
     //
     inline void setTransform(const glm::mat4& mat) {
       mTransform = mat;
       mInvModelMatrix = glm::inverse(glm::transpose(mat));
     }
     //
     void addSubMesh(FcSubmesh& subMesh);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //
     inline const VkBuffer IndexBuffer() const { return mIndexBuffer.getVkBuffer(); }
     //
     inline const VkDeviceAddress VertexBufferAddress() const { return mVertexBufferAddress; }
     //
     inline const glm::mat4& ModelMatrix() const { return mTransform; }
     //
     inline glm::mat4 InvModelMatrix() const { return mInvModelMatrix; }
     //
     inline const std::vector<FcSubmesh>& SubMeshes() const { return mSubMeshes; }
     //
  };


  //
  //
  struct FcSubmesh
  {
     u32 startIndex{0};
     u32 indexCount{0};
     // TODO determine or elaborate why we need both bounds and boundary box
     FcBounds bounds;
     // TODO make boundary box member conditional based on debug build
     FcBoundaryBox boundaryBox;
     // TEST should this be shared??
     std::shared_ptr<FcMaterial> material;
     // ?? could perhaps be made more efficient as standard pointer or reference wrapper??
     std::weak_ptr<FcSurface> parent;
     // TODO implement all the getter function that get members from parent then eliminate x.parent->trait()
     // within code and replace with x.trait();
     inline const glm::mat4& ModelMatrix() const { return parent.lock()->ModelMatrix(); }
     const bool isVisible(const glm::mat4& viewProjection) const;
     const bool isInBounds(const glm::vec4& position) const;
     // Note that bounds must first be set before calling initBoundaryBox()
     inline void initBoundaryBox() { boundaryBox.init(bounds); }
  };



}// --- namespace fc --- (END)
