//>--- fc_mesh.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_bounding_box.hpp"
#include "fc_buffer.hpp"
#include "fc_node.hpp"
#include "fc_types.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <span>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcMeshNode; struct FcSubmesh; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcMesh
  {
   private:
     VkDeviceAddress mVertexBufferAddress;
     //
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
     FcMesh() = default;
     void destroy();
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //
     inline bool operator==(const FcMesh& other) const
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
     void addSubMesh(FcSubmesh& subMesh);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //
     inline const VkBuffer IndexBuffer() const { return mIndexBuffer.getVkBuffer(); }
     //
     inline const VkDeviceAddress VertexBufferAddress() const { return mVertexBufferAddress; }
     //
     inline std::vector<FcSubmesh>& SubMeshes() { return mSubMeshes; }
  };


  //
  //
  struct FcSubmesh
  {
     u32 startIndex{0};
     //
     u32 indexCount{0};
     //
     FcMeshNode* node;
     // TODO determine or elaborate why we need both bounds and boundary box
     FcBounds bounds;
     // TODO make boundary box member conditional based on debug build
     FcBoundaryBox boundaryBox;
     // TEST should this be shared??
     std::shared_ptr<FcMaterial> material;
     // ?? could perhaps be made more efficient as standard pointer or reference wrapper??
     std::weak_ptr<FcMesh> parent;
     //
     const bool isVisible(const glm::mat4& viewProjection) const;
     //
     const bool isInBounds(const glm::vec4& position) const;
     //
     // Note that bounds must first be set before calling initBoundaryBox()
     inline void initBoundaryBox() { boundaryBox.init(bounds); }
     // Instead of creating a ScenePushConstants variable and then setting its matrices etc. equal to
     // those within a meshNode, FcNode and its derived class FcMeshNode have been formated so that
     // we can use a pointer to the world transform and with an FcMeshNode, the worldTransform,
     // inverse world transform and vertex buffer address are all contiguous and can be passed to
     // the vkCmdPushConstants function after casting the pointer (which isn't strictly necessary)
     inline const ScenePushConstants* getSceneConstantsPtr() const
      { return reinterpret_cast<ScenePushConstants*>(&node->worldTransform);}
  };



}// --- namespace fc --- (END)
