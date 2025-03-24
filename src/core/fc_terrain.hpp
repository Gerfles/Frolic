// Generate terrain + utilities

#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_mesh.hpp"
#include "fc_pipeline.hpp"
#include "fc_image.hpp"


namespace fc
{

  class FcTerrain
  {
   private:
     // TODO enum with control points

     FcImage mHeightMap;
     // TODO DELETE after creating sampler atlas
     VkSampler mHeightMapSampler;
     VkDescriptorSetLayout mHeightMapDescriptorLayout;
     VkDescriptorSet mHeightMapDescriptor;
     FcMesh mMesh;
     FcPipeline mPipeline;
     FcRenderer* pRenderer;
     uint32_t mNumIndices;
     uint32_t mPixelDensity;
     uint32_t mNumPatches;

     void initPipelines();
     void createSampler();
   public:
     void init(FcRenderer* renderer, std::filesystem::path filename);
     void loadHeightmap(std::filesystem::path filename, uint32_t numPatches);
     void generateTerrain();
     void draw(VkCommandBuffer cmdBuffer, VkDescriptorSet* sceneDataDescriptors);
  };
}// --- namespace fc --- (END)
