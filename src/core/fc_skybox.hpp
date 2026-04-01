//>--- fc_skybox.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
#include "fc_pipeline.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcDescriptorCollection; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcSkybox
  {
   private:
     FcImage mCubeImage;
     FcPipeline mPipeline;
     VkSampler mCubeMapSampler {VK_NULL_HANDLE};
     VkDescriptorSet mDescriptorSet {VK_NULL_HANDLE};
     static constexpr int NUM_SIDES_CUBE = 6;

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline FcSkybox() = default;
     FcSkybox& operator=(const FcSkybox&) = delete;
     FcSkybox& operator=(FcSkybox&&) = delete;
     FcSkybox(const FcSkybox&) = delete;
     FcSkybox(FcSkybox&&) = delete;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void loadTextures(std::string_view parentPath, std::string_view extension);
     //
     void loadTextures(std::vector<std::string>& filenames);
     //
     // TODO Store scene data on GPU and ref with frame push constant
     void init(const FcBuffer& sceneDataBuffer);
     //
     void draw(VkCommandBuffer cmd, FcDescriptorCollection& currentFrame);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline const FcImage& Image() { return mCubeImage; }
  };

}// --- namespace --- (END)
