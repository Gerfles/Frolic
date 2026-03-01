//>--- fc_normal_renderer.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_pipeline.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FrameAssets; class FcDrawCollection;}
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
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
