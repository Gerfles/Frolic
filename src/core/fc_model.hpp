#pragma once

// - FROLIC ENGINE -
//#include "core/fc_gpu.hpp"
#include "fc_image.hpp"
#include "fc_pipeline.hpp"
#include "fc_mesh.hpp"
// - EXTERNAL LIBRARIES -
#include <_types/_uint32_t.h>
#include <glm/glm.hpp>
#define ASSIMP_USE_HUNTER
#include <assimp/scene.h>
// - STD LIBRARIES -
#include <string>
#include <vector>

namespace fc {

  class FcMesh;
  
  class FcModel
  {
   private:
      //static FcGpu* pGpu;
     std::vector<FcMesh> mMeshList; 
     std::vector<FcImage> mTextures;
     glm::mat4 mModelMatrix;
   
   public:
      // INITIALIZATION
     FcModel() = delete;
     FcModel(std::string modelFile);

     FcModel& operator=(FcModel&&) = default;
     FcModel(FcModel&&) = default;
      //void init(FcGpu& gpu);
      // TODO don't initialize like this... have FcModel import meshes
     FcModel(std::vector<FcMesh> newMesh) : mMeshList{newMesh} { mModelMatrix = glm::mat4(1.0f); }
      // MUTATORS
      //void createModel(std::string modelFile, FcPipeline& pipeline, FcGpu& gpu);
      // ?? why were the following three function static in Ben's implementation ??
     std::vector<std::string> LoadMaterials(const aiScene* scene);
     void loadNodes(const FcGpu& gpu, aiNode* node, const aiScene* scene, std::vector<int>& matToTex);
     void loadMesh(const FcGpu& gpu, aiMesh* mesh, const aiScene* scene, uint32_t textureID);
     uint32_t loadTexture(std::string filename);
     
     void setModelMatrix(glm::mat4 modelMatrix) { mModelMatrix = modelMatrix; }
      // GETTERS
     glm::mat4& ModelMatrix() { return mModelMatrix; }
     size_t MeshCount() { return mMeshList.size(); }
      // TODO should think about checking bounds of the index passed 
     FcMesh& Mesh(size_t index) { return mMeshList[index]; }
      // CLEANUP
     void destroy();
      // Think we 
     ~FcModel() = default;
  };
  
  
  
}
