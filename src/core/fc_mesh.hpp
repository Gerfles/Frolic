#pragma once
//TODO rename file fc_mesh.h

// - FROLIC ENGINE -
//#include "fc_pipeline.hpp"
#include "core/fc_materials.hpp"
//#include "core/fc_model.hpp"
#include "core/utilities.hpp"
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

//I've seen recommendations to have a binding for anything needed for calculating gl_Position, and a
//second binding for everything else. So hardware that does a pre-pass to bin by position only has
//to touch the relevant half of the data.
  struct Vertex
  {
     glm::vec3 position {0.f, 0.f, 0.f};
     float uv_x {0.f};
     glm::vec3 normal {1.f, 0.f, 0.f};
     float uv_y {0.f};
     //glm::vec4 color; // not needed with pbr
     glm::vec4 tangent {0.f, 0.f, 0.f, 0.f};
     // TODO could add some features like a print function, etc.
  };

  // TODO Can incrementally update push constants according to:
  // https://docs.vulkan.org/guide/latest/push_constants.html
  // May provide some benefits if done correctly
  struct DrawPushConstants
  {
     glm::mat4 worldMatrix;
     glm::mat4 normalTransform;
     VkDeviceAddress vertexBuffer;
  };

  struct BoundingBoxPushConstants
  {
     glm::mat4 modelMatrix;
     glm::vec4 origin;
     glm::vec4 extents;
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
       virtual void draw(DrawContext& ctx) = 0;
       virtual void update(const glm::mat4& topMatrix) = 0;
    };

  // implementation of a drawable scene node. The scene node can hold Children and will also keep
  // a transform to propagate to them
  struct Node : public IRenderable
  {
     // TODO find out what is meant by the following
     // parent pointer must be a weak pointer to avoid circular dependencies
     std::weak_ptr<Node> parent;
     std::vector<std::shared_ptr<Node>> children;

     glm::mat4 localTransform {1.0f};
     glm::mat4 worldTransform {1.0f};
     void refreshTransform(const glm::mat4& parentMatrix);
     virtual void draw(DrawContext& context);
     virtual void update(const glm::mat4& topMatrix);
  };


  struct MeshNode : public Node
  {
     std::shared_ptr<FcMesh> mesh;
     virtual void draw(DrawContext& context) override;
     virtual void update(const glm::mat4& topMatrix) override;
  };


  struct Bounds
  {
     glm::vec3 origin;
     float sphereRadius;
     glm::vec3 extents;
  };


  //?? could this be Replaced by FcModel
  struct RenderObject
  {
     uint32_t indexCount;
     uint32_t firstIndex;
     VkBuffer indexBuffer;

     MaterialInstance* material;
     glm::mat4 transform {1.0f};
     glm::mat4 invModelMatrix {1.0f};
     Bounds bounds;
     VkDeviceAddress vertexBufferAddress;
     void bindPipeline(VkCommandBuffer cmd) const {  material->pPipeline->bind(cmd); }
     void bindDescriptorSet(VkCommandBuffer cmd
                            , VkDescriptorSet descriptorSet, uint32_t firstSet) const;
     void bindIndexBuffer(VkCommandBuffer cmd) const;
     bool isVisible(const glm::mat4& viewProj);
  };

  struct GLTFMaterial
  {
     MaterialInstance data;
  };


  struct Surface
  {
     uint32_t startIndex{0};
     uint32_t count{0};
     Bounds bounds;
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
     ~FcMesh() { fcLog("Deleting FcMesh");}
     // TODO pass by referenced and verify operation
     // ?? ?? for some reason we cant call the following class with fastgltf::mesh.name
     // since it uses an std::pmr::string that only seems to be able to bind to a public
     // class member?? TODO researce PMR
     void uploadMesh2(std::span<Vertex> vertices, std::span<uint32_t> indices);

     //void setIndexCounts(uint32_t start, uint32_t count);
     //uint32_t getStartIndex(int ) { return mSurfaces; }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcMesh() = default;
     FcMesh(const FcMesh&) = delete;

     FcMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, uint32_t descriptorID);
     // void createMesh(std::vector<Vertex>& vertices
     //                 , std::vector<uint32_t>& indices, uint32_t textureID);

     void setModel(glm::mat4 newModel);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     std::string& name() { return mName; }
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
     //~FcMesh() = default;

  };

} // namespace fc _END_
