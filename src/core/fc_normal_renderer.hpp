// fc_normal_renderer.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include "fc_pipeline.hpp"
#include "fc_frame_assets.hpp"
#include "fc_draw_collection.hpp"
//
namespace fc
{
  class FcNormalRenderer
  {
   private:
     FcPipeline mNormalDrawPipeline;
   public:
     void buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout);
     void draw(VkCommandBuffer cmd,
               FcDrawCollection& drawCollection,
               FrameAssets& currentFrame);
     void destroy();
  };
} // --- namespace fc --- (END)
