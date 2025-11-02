//>_ fc_skybox.hpp _<//
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
#include "fc_buffer.hpp"
#include "fc_pipeline.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <filesystem>


namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECLS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FrameAssets;

  //
  //
  class FcSkybox
  {
   private:
     FcImage mCubeImage;
     FcPipeline mPipeline;
     VkSampler mCubeMapSampler;
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
     void init(VkDescriptorSetLayout sceneDescriptorLayout, std::vector<FrameAssets>& frames);
     //
     void draw(VkCommandBuffer cmd, FrameAssets& currentFrame);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline const FcImage& Image() { return mCubeImage; }
};

}// --- namespace --- (END)
