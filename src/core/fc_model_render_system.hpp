#pragma once

// CORE
#include "core/fc_descriptors.hpp"
#include "core/fc_gpu.hpp"
#include "fc_pipeline.hpp"
// EXTERNAL
#include "vulkan/vulkan_core.h"
// STD
#include <memory>

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
