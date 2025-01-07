#pragma once
//TODO rename file fc_mesh.h

// - FROLIC ENGINE -
//#include "fc_pipeline.hpp"
#include "fc_buffer.hpp"
// - EXTERNAL LIBRARIES -
#include "glm/mat4x4.hpp"
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

// forward declarations
//class FcGpu;

namespace fc
{

   // Vertex data representation
  struct Vertex
  {
     glm::vec3 position; // vertex position (x, y, z)
     glm::vec3 color; // vertex color (r, g, b)
     glm::vec3 normal;
     glm::vec2 texCoord; // texel coordinates(u, v)

      // void print()
      //  {
      //    std::cout << "Vertex: Position(" << pos.x << ", " << pos.y << ", " << pos.z << ")"
      //              << "        Color(" << col.r << ", " << col.g << ", " << col.b << ")"
      //              << "        TexCoord(" << tex.x << ", " << tex.y << ")" << std::endl;
      //  }
     bool operator==(const Vertex& rhs) const
      {
        return position == rhs.position && color == rhs.color && normal == rhs.normal && texCoord == rhs.texCoord;
      }
  };


  struct Vertex2
  {
     glm::vec3 position;
     float uv_x;
     glm::vec3 normal;
     float uv_y;
     glm::vec4 color;
  };

  struct drawPushConstants
  {
     glm::mat4 worldMatrix;
     VkDeviceAddress vertexBuffer;
  };

   // TODO get rid of this and keep in FcModel
   // must be in a struct for the Uniform buffer or push constant to use
   // struct ModelMatrix
   // {
   //    glm::mat4 model;
   // };

  // struct Surface
  // {
  //    uint32_t startIndex{0};
  //    uint32_t count{0};
  // };


  class FcMesh
  {
   private:
      // TODO find out if using both vertex and index counts may need to delete in favor of below
     uint32_t mVertexCount{0};
     uint32_t mIndexCount{0};
      // TODO As it appears that both VkDevice and vkphysicaldevice are both just "opaque handles"
      // we should be able to avoid using pointers to them (since they are already pointers...)
      //  ModelMatrix mUboModel;
      // TODO DELETE as we are no longer accessing a desriptor set but may need for texture atlas
     uint32_t mDescriptorID{0};
     FcBuffer mVertexBuffer;
     FcBuffer mIndexBuffer;
     VkDeviceAddress mVertexBufferAddress{};
      //
     void createVertexBuffer(std::vector<Vertex>& vertices);
     void createIndexBuffer(std::vector<uint32_t>& indices);
      //
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      //std::vector<Surface> surfaces;
     uint32_t startIndex{0};
      // ?? not sure if we will eventually require this name at the mesh level, may want to delete
      // in favor of the one at the model level
     std::string mName;
      // TODO try and implement with shared pointers to meshes
      // std::vector<std::shared_ptr<FcMesh>> meshes;
   public:
      // TODO pass by referenced and verify operation
     void uploadMesh2(std::string name, std::span<Vertex2> vertices, std::span<uint32_t> indices);
     void setIndexCounts(uint32_t start, uint32_t count);
     uint32_t getStartIndex() { return startIndex; }
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcMesh() = default;
     FcMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, uint32_t descriptorID);
     // void createMesh(std::vector<Vertex>& vertices
     //                 , std::vector<uint32_t>& indices, uint32_t textureID);

     void setModel(glm::mat4 newModel);
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // const ModelMatrix& getModel() { return mUboModel; }
     uint32_t VertexCount() const { return mVertexCount; }
     uint32_t IndexCount() const { return mIndexCount; }
      // TODO should both be const and probably not referenct since VkBuffer is just pointer
     VkBuffer& VertexBuffer() { return mVertexBuffer.getVkBuffer(); }
     const VkBuffer IndexBuffer() { return mIndexBuffer.getVkBuffer(); }
     uint32_t DescriptorId() const { return mDescriptorID; }
     VkDeviceAddress VertexBufferAddress() { return mVertexBufferAddress; }
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
     ~FcMesh() = default;

  };

} // namespace fc _END_
