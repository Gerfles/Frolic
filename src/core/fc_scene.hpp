// fc_scene.hpp
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_node.hpp"
#include "core/fc_scene_renderer.hpp"
#include "fc_mesh.hpp"
#include "fc_defaults.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#define ASSIMP_USE_HUNTER
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <fastgltf/types.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <unordered_map>
// #include <vulkan/vulkan_core.h>
// #include "SDL2/SDL_stdinc.h"

namespace fc
{

class FcScene
{
 private:
   // ?? remove
   std::string mName;
   // TODO initialize or delete
   uint32_t mNumMaterials;
   // storage for all the data on a given glTF file
   std::vector<FcImage> mTextures;
   // Used for faster loading of materials
   VkDescriptorSetLayout mMaterialDescriptorLayout;
   // TODO May be better to have simple vectors instead and convert names to IDs
   // Collection of all the nodes in scene, including FcMeshNodes
   std::unordered_map<std::string, std::shared_ptr<FcNode>> mNodes;
   // just the nodes that don't have a parent, for iterating through the file in tree order
   std::vector<std::shared_ptr<FcNode>> mTopNodes;
   std::vector<VkSampler*> pSamplers;
   glm::mat4 mTransformMat{1.0f};
 public:
   // TODO change to private
   FcBuffer mMaterialDataBuffer;
   // TODO try and just create the materials within model and keep a pointer via VkDescriptorSet
   /* std::vector<std::shared_ptr<FcMaterial>> mMaterials; */
   // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
   FcScene(std::string fileName, VkDescriptorSetLayout descriptorLayout);
   // TODO don't initialize like this... have FcScene import meshes
   //FcScene(std::vector<FcMesh> newMesh) : mMeshList{newMesh} { mModelMatrix = glm::mat4(1.0f); }
   FcScene() = default;
   FcScene& operator=(FcScene&&) = delete;
   FcScene(FcScene&&) = delete;
   // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
   void addToDrawCollection(FcDrawCollection& ctx);

   void loadGltf(FcSceneRenderer& sceneRenderer, std::string_view filepath);
   VkFilter extractFilter(fastgltf::Filter filter);
   VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter);
   // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TRANSFORMS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
   void rotate(float angleDegrees, glm::vec3 axis);
   void translate(glm::vec3 offset);
   void scale(const glm::vec3 axisFactors);
   // NOTE: must call update after any transform in order to change the scene in DrawCollection
   void update(FcDrawCollection& collection);


   // TODO separate FcModel from LoadedGLTF classes
   std::vector<std::string> LoadMaterials(const aiScene* scene);
   void loadGltfMeshes(std::filesystem::path filePath);
   void loadAssimpNodes(aiNode* node, const aiScene* scene, std::vector<int>& matToTex);
   void loadAssimpMesh(aiMesh* mesh, const aiScene* scene, uint32_t textureID);
   uint32_t loadTexture(std::string filename, VkDescriptorSetLayout layout);
   const uint32_t NumMaterials() { return mNumMaterials; }
   /* void setModelMatrix(glm::mat4 modelMatrix) { mModelMatrix = modelMatrix; } */
   // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
   // VkDeviceAddress vertexBufferAddress() const { return mVertexBufferAddress; }
   /* const glm::mat4& modelMatrix() const { return mModelMatrix; } */
   // size_t MeshCount() { return mMeshList.size(); }
   // FcMesh& Mesh(size_t index) { return mMeshList[index]; } // No bounds checking
   // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
   void destroy();
   void clearAll();
   ~FcScene() = default;
};



}// --- namespace fc --- (END)
