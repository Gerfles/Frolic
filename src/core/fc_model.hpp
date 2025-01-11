#pragma once

 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include "core/fc_gpu.hpp"
#include "fc_descriptors.hpp"
#include "fc_image.hpp"
#include "fc_materials.hpp"
#include "fc_pipeline.hpp"
#include "fc_mesh.hpp"
 // -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include <assimp/mesh.h>
// #include <assimp/scene.h>
// #include <assimp/types.h>
#include <vulkan/vulkan_core.h>
 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/ext/matrix_float4x4.hpp>
#include <string>
 // #include <vector>
 // #include <unordered_map>
#include <filesystem>
 // #include <cstdint>

class aiScene;
class aiMesh;
class aiNode;

namespace fc {
   //
  //?? Replaced by FcModel
  // struct RenderObject
  // {
  //    uint32_t indexCount;
  //    uint32_t firstIndex;
  //    VkBuffer indexBuffer;

  //    MaterialInstance* material;
  //    glm::mat4 transform; ->?? mModelMatrix
  //    VkDeviceAddress vertexBufferAddress;
  // };

//class FcMesh;

  // FIXME implement a better system of extrapolating/expressing the relationship between RenderObject
  // and FcModel and FcMesh. For instance, you could still use a larger class like this as long as
  // everything is being done by pointers, etc. FcModel could contain a RenderObject for instance
  // NOTE: Model may no longer be viable since we are mostly trying to create a scene graph where
  // each node can have multiple children nodes but only one parent node. This design allows us to have
  // meshes that will get reused in separate instances but within the same scene. Given that, FcModel
  // should probably be transformed into something like FcScene?
  class FcModel
  {
   private:
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW MODEL PARADIGM   -*-*-*-*-*-*-*-*-*-*-*-*-*- //


     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   OLD MODEL PARADIGM   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
     std::vector<FcMesh> mMeshList;
     std::vector<FcImage> mTextures;
     std::vector<VkDescriptorSet> mDescriptorSets;
     glm::mat4 mModelMatrix;
     std::string name;
     // uint32_t mIndexCount;
     // uint32_t mFirstIndex;
     // FcBuffer mIndexBuffer;
     // VkDeviceAddress mVertexBufferAddress;
     // MaterialInstance* pMaterial;

   public:
     std::vector<std::shared_ptr<FcMesh>> meshes;
     // void bindPipeline(VkCommandBuffer cmd) const { pMaterial->pPipeline->bind(cmd); }
     // void bindDescriptorSets(VkCommandBuffer cmd
     //                         , VkDescriptorSet descriptorSet, uint32_t firstSet) const;
     // // TODO think about renaming
     // void bindMaterialDescriptorSet(VkCommandBuffer cmd, uint32_t firstSet) const;
     // void bindIndexBuffer(VkCommandBuffer cmd) const;


      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     FcModel(std::string fileName, VkDescriptorSetLayout descriptorLayout);
    // TODO don't initialize like this... have FcModel import meshes
     FcModel(std::vector<FcMesh> newMesh) : mMeshList{newMesh} { mModelMatrix = glm::mat4(1.0f); }
     FcModel() = default;
     FcModel& operator=(FcModel&&) = delete;
     FcModel(FcModel&&) = delete;

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      //void createModel(std::string modelFile, FcPipeline& pipeline, FcGpu& gpu);
      // ?? why were the following three function static in Ben's implementation ??
      // ?? was it to make the functions globally available
     std::vector<std::string> LoadMaterials(const aiScene* scene);

     //std::optional<std::vector<std::shared_ptr<FcModel>>> loadGltfMeshes(std::filesystem::path filePath);
     void loadGltfMeshes(std::filesystem::path filePath);

     // FIXME these need to be renamed to specify assimp methods -> or just remove ASSIMP
     void loadNodes(aiNode* node, const aiScene* scene, std::vector<int>& matToTex);
     void loadMesh(aiMesh* mesh, const aiScene* scene, uint32_t textureID);

     uint32_t loadTexture(std::string filename, VkDescriptorSetLayout layout);
     void setModelMatrix(glm::mat4 modelMatrix) { mModelMatrix = modelMatrix; }
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // uint32_t indexCount() const { return mIndexCount; }
     // uint32_t firstIndex() const { return mFirstIndex; }
     // VkDeviceAddress vertexBufferAddress() const { return mVertexBufferAddress; }
     VkDescriptorSet getDescriptorSet(uint32_t index) { return mDescriptorSets[index]; }
     const glm::mat4& modelMatrix() const { return mModelMatrix; }
     size_t MeshCount() { return mMeshList.size(); }
      // TODO should think about checking bounds of the index passed
     FcMesh& Mesh(size_t index) { return mMeshList[index]; }
      // CLEANUP
     void destroy();
      // Think we
     ~FcModel() = default;
  };



  struct DrawContext
  {
     std::vector<RenderObject> opaqueSurfaces;
  };

}
