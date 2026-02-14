// fc_scene.hpp
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_node.hpp"
#include "core/fc_scene_renderer.hpp"
#include "core/fc_buffer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#define ASSIMP_USE_HUNTER
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <fastgltf/types.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <unordered_map>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  class FcScene
  {
   private:
     FcRenderer* pRenderer;
     // TODO utilize name or remove
     std::string mName;
     uint32_t mNumMaterials {0};
     VkDescriptorSetLayout mMaterialDescriptorLayout;
     // just the nodes that don't have a parent, for iterating through the file in tree order
     std::vector<std::shared_ptr<FcNode>> mTopNodes;
     //
     glm::mat4 mTransformMat{1.f};
     glm::mat4 mRotationMat{1.f};
     glm::mat4 mTranslationMat{1.f};
     //
     FcBuffer mMaterialDataBuffer;
     //
     // Keep the collection of all surfaces/meshes within the scene
     std::vector<std::shared_ptr<FcMesh> > mMeshes;

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   HELPERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void printNode(std::shared_ptr<FcNode>& node, std::string& nodeID);

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcScene(std::string fileName, VkDescriptorSetLayout descriptorLayout);
     FcScene() = default;
     FcScene& operator=(FcScene&&) = delete;
     FcScene(FcScene&&) = delete;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void addToDrawCollection(FcDrawCollection& ctx);
     //
     void loadGltf(FcRenderer& renderer, std::string_view filepath);
     //
     VkFilter extractFilter(fastgltf::Filter filter) noexcept;
     //
     VkSampler extractSampler(fastgltf::Sampler& sampler);
     //
     VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter) noexcept;
     //
     void loadMeshes(fastgltf::Asset& gltf, std::vector<std::shared_ptr<FcMaterial>>& materials);
     //
     void bindlessLoadAllMaterials(FcDrawCollection& drawCollection,
                                   fastgltf::Asset& gltf,
                                   std::vector<std::shared_ptr<FcMaterial>>& materials,
                                   std::filesystem::path& parentPath);
     //
     void loadAllMaterials(FcDrawCollection& drawCollection,
                           fastgltf::Asset& gltf,
                           std::vector<std::shared_ptr<FcMaterial>>& materials,
                           std::filesystem::path& parentPath);
     //
     void toggleTextureUse(MaterialFeatures texture, std::array<std::vector<u32>, 5>& currentIndices);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TRANSFORMS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void rotate(float angleDegrees, glm::vec3& axis);
     void rotateInPlace(float angleDegrees, glm::vec3& axis);
     void translate(glm::vec3& offset);
     void scale(const glm::vec3& axisFactors);
     // NOTE: must call update after any transform in order to change the scene in DrawCollection
     void update();
     // update only using the supplied matrix (not the member matrix)
     void update(glm::mat4& mat, FcDrawCollection& collection);
     //
     std::vector<std::string> LoadMaterials(const aiScene* scene);
     void loadAssimpNodes(aiNode* node, const aiScene* scene, std::vector<int>& matToTex);
     void loadAssimpMesh(aiMesh* mesh, const aiScene* scene, uint32_t textureID);
     uint32_t loadTexture(std::string filename, VkDescriptorSetLayout layout);
     const uint32_t NumMaterials() { return mNumMaterials; }

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TOOLS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void drawSceneGraph();

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
     void clearAll();
     ~FcScene() = default;
  };

}// --- namespace fc --- (END)
