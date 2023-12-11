#pragma once

// CORE
// #include "core/fc_descriptors.hpp"
#include "core/fc_gpu.hpp"
#include "fc_pipeline.hpp"
// EXTERNAL
#include "vulkan/vulkan_core.h"
// STD
#include <_types/_uint32_t.h>
#include <memory>
#include <sys/_types/_size_t.h>

namespace fc
{
   // TODO combine with billboardcomponent?
  struct UIpushConstants
  {
      // TODO vec4 used to simplify alignment. may want to do differently
     glm::vec4 position{};
     glm::vec4 color{};
     float width;
     float height;
  };

  class FcText;
  
  class FcUIrenderSystem
  {
   private:
      //FcPipeline mUIpipeline;
      //
   public:
     void createPipeline(FcPipeline& pipeline, VkRenderPass& renderPass);
     FcUIrenderSystem() = default;
     ~FcUIrenderSystem() = default;
     FcUIrenderSystem(const FcUIrenderSystem&) = delete;
     FcUIrenderSystem &operator=(const FcUIrenderSystem&) = delete;
      //
     void draw(std::vector<FcText>& UIelements, VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);
     void destroy();
  };

} // namespace fc _END_
