//>--- shadow_map.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_pipeline.hpp"
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcDrawCollection; class FrameAssets; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  constexpr uint32_t SHADOW_MAP_SIZE{2048};

  struct ShadowPushConsts
  {
     glm::mat4 MVP;
     /* glm::mat4 lightSpaceMatrix; */
     /* glm::mat4 modelMatrix; */
     VkDeviceAddress vertexBuffer;
  };


  // TODO use frustum class instead
  struct Box
  {
     float near;
     float far;
     float left;
     float right;
     float top;
     float bottom;
     void setAll(float near, float far, float left, float right, float top, float bottom)
      {
        this->near = near; this->far = far; this->left = left;
        this->right = right; this->top = top; this->bottom = bottom;
      }
  };


  class FcShadowMap
  {
   private:

     glm::mat4 mLightProjection;
     glm::mat4 mLightView;
     glm::mat4 mLightSpaceTransform;
     //VkFence shadowRenderFence;
     FcImage mShadowMapImage;
     FcPipeline mShadowPipeline;
     FcPipeline mShadowDebugPipeline;
     /* VkDescriptorSet mShadowMapDescriptorSet; */
     void initPipelines(std::vector<FrameAssets>& frames);
     Box mFrustum;
   public:
     Box& Frustum() { return mFrustum; }
     //void setLightSpaceMatrix(glm::mat4 matrix) { mLightSpaceTransform = matrix; }
     void init(std::vector<FrameAssets>& frames);
     void generateMap(VkCommandBuffer cmd, FcDrawCollection& drawContext);
     void drawDebugMap(VkCommandBuffer cmd, FrameAssets& currentFrame);
     glm::mat4 LightSpaceMatrix() { return mLightSpaceTransform; }
     /* VkDescriptorSet Descriptor() { return mShadowMapDescriptorSet; } */
     VkImageView ImageView() { return mShadowMapImage.ImageView(); }
     void updateLightSource(glm::vec3 lightPos, glm::vec3 target);
     void updateLightSpaceTransform();
  };// ---   class FcShadowMap --- (END)


}// --- namespace fc --- (END)
