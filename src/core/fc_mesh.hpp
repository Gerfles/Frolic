#pragma once
//TODO rename file fc_mesh.h

// - FROLIC ENGINE -
//#include "fc_pipeline.hpp"
#include "core/fc_materials.hpp"
//#include "core/fc_model.hpp"
#include "fc_buffer.hpp"
// - EXTERNAL LIBRARIES -
#include "glm/mat4x4.hpp"
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FcMesh;
  class FcModel;
  struct DrawContext;

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

  struct DrawPushConstants
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


  // base class for a renderable dynamic object
  class IRenderable
  {
     virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
  };

  // implementation of a drawable scene node. The scene node can hold Children and will also keep
  // a transform to propagate to them
  struct Node : public IRenderable
  {
     // TODO find out what is meant by the following
     // parent pointer must be a weak pointer to avoid circular dependencies
     std::weak_ptr<Node> parent;
     std::vector<std::shared_ptr<Node>> children;

     glm::mat4 localTransform;
     glm::mat4 worldTransform;

     void refreshTransform(const glm::mat4& parentMatrix);
     virtual void draw(const glm::mat4& topMatrix, DrawContext& context);
  };


  struct MeshNode : public Node
  {
     std::shared_ptr<FcMesh> mesh;

     virtual void draw(const glm::mat4& topMatrix, DrawContext& context) override;
  };


  //?? could this be Replaced by FcModel
  struct RenderObject
  {
     uint32_t indexCount;
     uint32_t firstIndex;
     VkBuffer indexBuffer;

     MaterialInstance* material;
     glm::mat4 transform;
     VkDeviceAddress vertexBufferAddress;
     void bindPipeline(VkCommandBuffer cmd) const { material->pPipeline->bind(cmd); }
     void bindDescriptorSet(VkCommandBuffer cmd
                             , VkDescriptorSet descriptorSet, uint32_t firstSet) const;
     void bindIndexBuffer(VkCommandBuffer cmd) const;
  };

  struct GLTFMaterial
  {
     MaterialInstance data;
  };


    struct Surface
    {
       uint32_t startIndex{0};
       uint32_t count{0};
       std::shared_ptr<GLTFMaterial> material;
    };

  // FIXME Should think about lightening this class or making it a wrapper class for a struct with quickly
  // accessible variables, etc.
  class FcMesh// : public Node
  {
   private:
     // TODO As it appears that both VkDevice and vkphysicaldevice are both just "opaque handles"
     // we should be able to avoid using pointers to them (since they are already pointers...)
     //  ModelMatrix mUboModel;
     // TODO DELETE as we are no longer accessing a desriptor set but may need for texture atlas
     uint32_t mDescriptorID{0};
     FcBuffer mVertexBuffer;
     FcBuffer mIndexBuffer;
     VkDeviceAddress mVertexBufferAddress{};
     //

     //

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


     void createVertexBuffer(std::vector<Vertex>& vertices);
     void createIndexBuffer(std::vector<uint32_t>& indices);
     // ?? not sure if we will eventually require this name at the mesh level, may want to delete
     // in favor of the one at the model level

     // TODO try and implement with shared pointers to meshes
     // std::vector<std::shared_ptr<FcMesh>> meshes;
   public:
          std::string mName;
     std::vector<Surface> mSurfaces;
     //     const std::vector<Surface>& getSurfaces() { return mSurfaces; }

     // TODO pass by referenced and verify operation
     // ?? ?? for some reason we cant call the following class with fastgltf::mesh.name
     // since it uses an std::pmr::string that only seems to be able to bind to a public
     // class member?? TODO researce PMR
     void uploadMesh2(std::span<Vertex2> vertices, std::span<uint32_t> indices);

     //void setIndexCounts(uint32_t start, uint32_t count);
     //uint32_t getStartIndex(int ) { return mSurfaces; }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcMesh() = default;
     FcMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, uint32_t descriptorID);
     // void createMesh(std::vector<Vertex>& vertices
     //                 , std::vector<uint32_t>& indices, uint32_t textureID);

     void setModel(glm::mat4 newModel);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     std::string name() { return mName; }
     // const ModelMatrix& getModel() { return mUboModel; }
     //uint32_t VertexCount() const { return mVertexCount; }
     //uint32_t IndexCount() const { return mIndexCount; }
     // TODO should both be const and probably not referenct since VkBuffer is just pointer
     const VkBuffer& VertexBuffer() { return mVertexBuffer.getVkBuffer(); }
     const VkBuffer IndexBuffer() { return mIndexBuffer.getVkBuffer(); }
     uint32_t DescriptorId() const { return mDescriptorID; }
     VkDeviceAddress VertexBufferAddress() { return mVertexBufferAddress; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
     ~FcMesh() = default;

  };

} // namespace fc _END_
