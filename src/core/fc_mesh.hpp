#pragma once
//TODO rename file fc_mesh.h

// - FROLIC ENGINE -
#include "core/fc_node.hpp"
#include "fc_pipeline.hpp"
#include "fc_bounding_box.hpp"
//#include "core/utilities.hpp"
//#include "fc_materials.hpp"
//#include "core/fc_model.hpp"
#include "fc_buffer.hpp"
// - EXTERNAL LIBRARIES -
#include "glm/mat4x4.hpp"
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <cstdint>
/* #include <glm/ext/matrix_float4x4.hpp> */
#include <iostream>
#include <memory>
#include <span>
#include <vector>

namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
//  class FcMesh;
  class FcModel;


  // TEST see if initialization is necessary since usually we know they must be set
  // I've seen recommendations to have a binding for anything needed for calculating
  // gl_Position, and a second binding for everything else. So hardware that does a pre-pass
  // to bin by position only has to touch the relevant half of the data.
  struct Vertex
  {
     glm::vec3 position;
     float uv_x;
     glm::vec3 normal;
     float uv_y;
     //glm::vec4 color; // not really needed with pbr
     glm::vec4 tangent;
     // TODO could add some features like a print function, etc.
  };


  // TODO this could be more intuitively named
  struct FcMaterial
  {
     // TODO merge with MaterialFeatures
     enum class Type : uint8_t
     {
       Opaque,
      Transparent,
      Other,
     };
     VkDescriptorSet materialSet;
     Type materialType;
  };

  // TODO merge with FcMaterial
  struct FcSubMesh
  {
     uint32_t startIndex{0};
     uint32_t indexCount{0};
     Bounds bounds;
     std::shared_ptr<FcMaterial> material;
  };




  struct VertexBufferPushes
  {
     VkDeviceAddress address;
     /* VkDeviceAddress padding; */
  };


  // Represents a raw mesh asset loaded from file.
class FcMesh
  {
   private:
     // ModelMatrix mUboModel;
     FcBuffer mVertexBuffer;
     FcBuffer mIndexBuffer;
     VkDeviceAddress mVertexBufferAddress{};
     uint32_t mIndexCount{0};
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void createVertexBuffer(std::vector<Vertex>& vertices);
     void createIndexBuffer(std::vector<uint32_t>& indices);
     // TODO try and implement with shared pointers to meshes
     // std::vector<std::shared_ptr<FcMesh>> meshes;

   public:
     // TODO make the following private members
     // ?? not sure if we will eventually require this name at the mesh level, may want to
     // delete in favor of the one at the model level
     std::string mName;
     // TODO would be better to store separate surface vectors based on material type / pipeline
     // but this would be rather involved and probably required to be build when loading gltf
     std::vector<FcSubMesh> mSurfaces;
     /* std::vector<FcSurface*> visibleSurfaces; */
     /* void sortVisibleSurfaces(const glm::mat4& viewProj); */
     // TODO should think about adding destructors on objects that call destroy() if needed.
     ~FcMesh() = default;
     // TODO pass by referenced and verify operation
     // ?? ?? for some reason we cant call the following class with fastgltf::mesh.name
     // since it uses an std::pmr::string that only seems to be able to bind to a public
     // class member?? TODO researce PMR
     template <typename T> void uploadMesh(std::span<T> vertices, std::span<uint32_t> indices);
     //void setIndexCounts(uint32_t start, uint32_t count);
     //uint32_t getStartIndex(int ) { return mSurfaces; }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcMesh() = default;
     FcMesh(const FcMesh&) = delete;

     FcMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, uint32_t descriptorID);
     // void createMesh(std::vector<Vertex>& vertices
     //                 , std::vector<uint32_t>& indices, uint32_t textureID);

     void setModel(glm::mat4 newModel);
     void bindIndexBuffer(VkCommandBuffer cmd) const;

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     std::string& name() { return mName; }
     // const ModelMatrix& getModel() { return mUboModel; }
     //uint32_t VertexCount() const { return mVertexCount; }
     //uint32_t IndexCount() const { return mIndexCount; }
     //     const std::vector<Surface>& getSurfaces() { return mSurfaces; }
     // TODO should both be const and probably not referenct since VkBuffer is just pointer
     const VkBuffer& VertexBuffer() { return mVertexBuffer.getVkBuffer(); }
     const VkBuffer IndexBuffer() { return mIndexBuffer.getVkBuffer(); }
     const uint32_t IndexCount() { return mIndexCount; }
     VkDeviceAddress VertexBufferAddress() { return mVertexBufferAddress; }
     VkDeviceAddress* VertexBufferAddress2() { return &mVertexBufferAddress; }
     const std::vector<FcSubMesh>& Surfaces() { return mSurfaces; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
};


} // namespace fc _END_
