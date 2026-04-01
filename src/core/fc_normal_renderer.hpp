//>--- fc_normal_renderer.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_pipeline.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcDescriptorCollection; class FcDrawCollection; class FcBuffer; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcNormalRenderer
  {
   private:
     VkDescriptorSet mDescriptorSet;
     FcPipeline mNormalDrawPipeline;
   public:
     void init(const FcBuffer& sceneDataBuffer);
     void draw(VkCommandBuffer cmd,
               FcDrawCollection& drawCollection,
               FcDescriptorCollection& currentFrame);
     void destroy();
  };
} // --- namespace fc --- (END)
