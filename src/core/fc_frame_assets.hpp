// fc_frame_assets.hpp
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>


namespace fc
{
  struct FrameAssets
  {
     // TODO might be better to make this an entire class with all methods and static ints (numFrame)
     VkCommandPool commandPool = VK_NULL_HANDLE;
     VkCommandBuffer commandBuffer;
     VkSemaphore imageAvailableSemaphore;
     VkSemaphore renderFinishedSemaphore;
     VkFence renderFence;
     // TODO allocate all the descriptorSets for each frame (skybox, terrain, etc.)
     VkDescriptorSet sceneDataDescriptorSet;
     VkDescriptorSet shadowMapDescriptorSet;
     VkDescriptorSet skyBoxDescriptorSet;

  };
}
