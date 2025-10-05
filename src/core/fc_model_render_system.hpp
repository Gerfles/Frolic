#pragma once

// CORE
#include "fc_pipeline.hpp"
#include <glm/ext/matrix_float4x4.hpp>
// EXTERNAL
// STD

namespace fc
{

  struct ModelPushConstantData
  {
     glm::mat4 modelMatrix{1.0f};
     glm::mat4 normalMatrix{1.0f};
  };

  class FcModelRenderSystem
  {
   private:
      //
      //
   public:
     void createPipeline(FcPipeline& pipeline);
     FcModelRenderSystem() = default;
     ~FcModelRenderSystem();
     FcModelRenderSystem(const FcModelRenderSystem&) = delete;
     FcModelRenderSystem &operator=(const FcModelRenderSystem&) = delete;
      //
     void renderGameObjects();
  };

} // namespace fc _END_
