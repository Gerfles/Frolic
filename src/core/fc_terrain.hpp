// Generate terrain + utilities

#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_mesh.hpp"
#include "fc_pipeline.hpp"
#include "fc_image.hpp"


namespace fc
{
  class FcFrustum;


  class FcTerrain
  {
   private:
     // TODO enum with control points
     struct UBO
     {
        glm::mat4 projection;
        glm::mat4 modelView; // TODO model is currently sent via push constants
        glm::mat4 modelViewProj;
        glm::vec4 lightPos;
        glm::vec4 frustumPlanes[6];
        float displacementFactor;
        float tessellationFactor;
        glm::vec2 viewportDim;
        float tessellatedEdgeSize;
     } ubo;
     //
     FcBuffer mUboBuffer;
     FcImage mHeightMap;
     FcImage mTerrainTexture;
     VkSampler mTerrainTextureSampler;
     glm::mat4 mModelTransform;
     // TODO DELETE after creating sampler atlas
     VkSampler mHeightMapSampler;
     VkDescriptorSetLayout mHeightMapDescriptorLayout;
     VkDescriptorSet mHeightMapDescriptor;
     FcMesh mMesh;
     FcPipeline mPipeline;
     // wire frame meshes
     FcPipeline mWireframePipeline;
     FcRenderer* pRenderer;
     uint32_t mNumIndices;
     uint32_t mPixelDensity;
     uint32_t mNumPatches;

     void initPipelines();
     void createSampler();
   public:
     void init(FcRenderer* renderer, std::filesystem::path filename);
     void update(FcFrustum& frustum);
     void loadHeightmap(std::filesystem::path filename, uint32_t numPatches);
     void generateTerrain();
     void draw(VkCommandBuffer cmdBuffer, SceneData* pSceneData, bool drawWireFrame);
  };
}// --- namespace fc --- (END)
