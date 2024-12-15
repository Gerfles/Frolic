#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_renderer.hpp"
#include "fc_window.hpp"
#include "fc_swapChain.hpp"
#include "utilities.hpp"

//#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>


namespace fc
{

  struct FrameData
  {
     VkCommandPool commandPool = VK_NULL_HANDLE;
     VkCommandBuffer commandBuffer;
  };

  struct PerformanceProperties
  {
     float maxSamplerAnisotropy;
     VkSampleCountFlagBits maxMsaaSamples;
  };

   // TODO seems carying around a screenSize would be ideal->instead of calling functions in window
   // then refactor all code to use that
  class FcGpu
  {
   private:
     int mFrameNumber {0};
     FrameData mFrames[MAX_FRAME_DRAWS];
     VkPhysicalDevice mPhysicalGPU = VK_NULL_HANDLE;
      // TODO consider renaming
     VkDevice mLogicalGPU = VK_NULL_HANDLE;
     VmaAllocator mAllocator = VK_NULL_HANDLE;
      // handles to Queues (graphics is often the same as presentation)
     VkQueue mGraphicsQueue;
     VkQueue mPresentationQueue;
     FcWindow* pWindow;
      // TODO delete VkCommandPool mCommandPool = VK_NULL_HANDLE;
      // used to setup graphics capabilities, sampler attributes, etc.
     PerformanceProperties mGpuPerformanceProperties;
      //VkSurfaceKHR mSurface;
      // - support functions
     void createCommandPool();

      // -- checker functions
     bool isDeviceSuitable(const VkPhysicalDevice& device);
     bool isDeviceExtensionSupported(const VkPhysicalDevice& device) const;
     QueueFamilyIndices getQueueFamilies(const VkPhysicalDevice& device) const;
     void pickPhysicalDevice(const VkInstance& instance, const std::vector<const char*> deviceExtensions);

   public:
     FcGpu() = default;
     ~FcGpu() = default;
      // - INITIALIZATION -
     bool init(const VkInstance& instance, FcWindow& window);
     bool createLogicalDevice();
      //
     VkCommandBuffer beginCommandBuffer() const;
     void submitCommandBuffer(VkCommandBuffer commandBuffer) const;
      // TODO establish convention that all getter functions are capitalized (since they "are" a type)
      // - GETTER FUNCTIONS -
     const VmaAllocator& getAllocator() const { return mAllocator; }
     const VkDevice& getVkDevice() const { return mLogicalGPU; }
     const VkPhysicalDevice& physicalDevice() const { return mPhysicalGPU; }
     const VkSurfaceKHR& surface() const { return pWindow->surface(); }
     VkExtent2D SurfaceExtent() const { return pWindow->ScreenSize(); }
     const FrameData& getCurrentFrame() const { return mFrames[mFrameNumber % MAX_FRAME_DRAWS]; }
      // DELETE
//     QueueFamilyIndices GpuQueueFamilies() const { return getQueueFamilies(mPhysicalGPU); }
     const VkQueue& graphicsQueue() const { return mGraphicsQueue; }
     const VkQueue& presentQueue() const { return mPresentationQueue; }
     SwapChainDetails swapChainDetails(const VkPhysicalDevice& device) const;
      // const VkCommandPool commandPool() const { return mCommandPool; }
     const PerformanceProperties Properties() const { return mGpuPerformanceProperties; }
      // Cleanup
     void release(VkInstance& instance);
  };
} // namespace fc END
