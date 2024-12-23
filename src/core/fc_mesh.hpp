#pragma once
//TODO rename file fc_mesh.h

// - FROLIC ENGINE -
//#include "fc_pipeline.hpp"
#include "fc_buffer.hpp"
// - EXTERNAL LIBRARIES -
#include "glm/mat4x4.hpp"
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <iostream>
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

   // TODO get rid of this and keep in FcModel
   // must be in a struct for the Uniform buffer or push constant to use
  // struct ModelMatrix
  // {
  //    glm::mat4 model;
  // };

  class FcMesh
  {
   private:
     uint32_t mVertexCount;
     uint32_t mIndexCount;
      // TODO As it appears that both VkDevice and vkphysicaldevice are both just "opaque handles"
      // we should be able to avoid using pointers to them (since they are already pointers...)
      //  ModelMatrix mUboModel;
     uint32_t mTextureID;
     FcBuffer mVertexBuffer;
     FcBuffer mIndexBuffer;
     void createVertexBuffer(std::vector<Vertex>& vertices);
     void createIndexBuffer(std::vector<uint32_t>& indices);
   public:
      //FcMesh() = default;
     FcMesh() = delete;
     FcMesh(std::vector<Vertex>& vertices
                     , std::vector<uint32_t>& indices, uint32_t textureID);
     void createMesh(std::vector<Vertex>& vertices
                     , std::vector<uint32_t>& indices, uint32_t textureID);
     void setModel(glm::mat4 newModel);
      // GETTERS
//     const ModelMatrix& getModel() { return mUboModel; }
     uint32_t VertexCount() const { return mVertexCount; }
     uint32_t IndexCount() const { return mIndexCount; }
      // TODO should both be const and probably not referenct since VkBuffer is just pointer
     VkBuffer& VertexBuffer() { return mVertexBuffer.getVkBuffer(); }
     const VkBuffer IndexBuffer() { return mIndexBuffer.getVkBuffer(); }
     uint32_t DescriptorId() const { return mTextureID; }
      // CLEANUP
     void destroy();
     ~FcMesh() = default;

  };

} // namespace fc _END_
