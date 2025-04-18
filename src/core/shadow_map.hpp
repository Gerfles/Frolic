#pragma once

#include "fc_pipeline.hpp"
#include "fc_image.hpp"
#include "glm/mat4x4.hpp"

// TODO move after relocating drawCollection
/* #include "fc_model.hpp" // DrawCollection */

namespace fc
{
  class FcRenderer;
  class FcDrawCollection;
  class FrameData;

  constexpr uint32_t shadowMapSize{2048};


  struct ShadowPushConstants
  {
     glm::mat4 lightSpaceMatrix;
     glm::mat4 modelMatrix;
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
     FcRenderer* pRenderer;
     glm::mat4 mLightProjection;
     glm::mat4 mLightView;
     glm::mat4 mLightSpaceTransform;
     VkFormat depthFormat;
     //VkFence shadowRenderFence;
     FcImage mShadowMapImage;
     FcPipeline mShadowPipeline;
     FcPipeline mShadowDebugPipeline;
     /* VkDescriptorSet mShadowMapDescriptorSet; */
     void initPipelines(std::vector<FrameData>& frames);
     Box mFrustum;
   public:
     Box& Frustum() { return mFrustum; }
     //void setLightSpaceMatrix(glm::mat4 matrix) { mLightSpaceTransform = matrix; }
     void init(FcRenderer* renderer, std::vector<FrameData>& frames);
     void generateMap(VkCommandBuffer cmd, FcDrawCollection& drawContext);
     void drawDebugMap(VkCommandBuffer cmd, FrameData& currentFrame);
     glm::mat4 LightSpaceMatrix() { return mLightSpaceTransform; }
     /* VkDescriptorSet Descriptor() { return mShadowMapDescriptorSet; } */
     VkImageView ImageView() { return mShadowMapImage.ImageView(); }
     void updateLightSource(glm::vec3 lightPos, glm::vec3 target);
     void updateLightSpaceTransform();

  };// ---   class FcShadowMap --- (END)


}// --- namespace fc --- (END)
