//>--- fc_gpu.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_window.hpp"
#include "fc_swapChain.hpp"
#include "fc_types.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //



namespace fc
{
  //
  struct PerformanceProperties
  {
     float maxSamplerAnisotropy;
     VkSampleCountFlagBits maxMsaaSamples;
     bool isBindlessSupported;
  };

   // TODO seems carying around a screenSize would be ideal->instead of calling functions in window
   // then refactor all code to use that
  class FcGpu
  {
   private:
     VkPhysicalDevice mPhysicalGPU = VK_NULL_HANDLE;
      // TODO consider renaming
     VkDevice mLogicalGPU = VK_NULL_HANDLE;
     VmaAllocator mAllocator = VK_NULL_HANDLE;
      // handles to Queues (graphics is often the same as presentation but could make dedicated)
     VkQueue mGraphicsQueue {VK_NULL_HANDLE};
     VkQueue mPresentationQueue {VK_NULL_HANDLE};
     FcWindow* pWindow;
      // TODO delete VkCommandPool mCommandPool = VK_NULL_HANDLE;
      // used to setup graphics capabilities, sampler attributes, etc.
     PerformanceProperties mGpuPerformanceProperties;
      //VkSurfaceKHR mSurface;
      // - support functions


     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   HELPERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void pickPhysicalDevice(const VkInstance& instance, std::vector<const char*>& deviceExtensions);

   public:
     FcGpu() = default;
     ~FcGpu() = default;
      // - INITIALIZATION -
     bool init(const VkInstance& instance, FcWindow& window);
     bool createLogicalDevice(std::vector<const char*>& deviceExtensions);

      // TODO DELETE 2 below
     // VkCommandBuffer beginCommandBuffer() const;
     // void submitCommandBuffer(VkCommandBuffer commandBuffer) const;
      // TODO establish convention that all getter functions are capitalized (since they "are" a type)
      // - GETTER FUNCTIONS -
      // DEL QueueFamilyIndices getQueueFamilies(const VkPhysicalDevice& device) const;
     const QueueFamilyIndices getQueueFamilies();
     const VmaAllocator& getAllocator() const { return mAllocator; }
     const VkDevice& getVkDevice() const { return mLogicalGPU; }
     const VkPhysicalDevice physicalDevice() const { return mPhysicalGPU; }
     const VkSurfaceKHR& surface() const { return pWindow->surface(); }
     VkExtent2D SurfaceExtent() const { return pWindow->ScreenSize(); }

      // DELETE
//     QueueFamilyIndices GpuQueueFamilies() const { return getQueueFamilies(mPhysicalGPU); }
     const VkQueue& graphicsQueue() const { return mGraphicsQueue; }
     const VkQueue& presentQueue() const { return mPresentationQueue; }
     SwapChainDetails getSwapChainDetails(const VkPhysicalDevice& device) const;
      // const VkCommandPool commandPool() const { return mCommandPool; }
     const PerformanceProperties Properties() const { return mGpuPerformanceProperties; }
      // Cleanup
     void release(VkInstance& instance);
  };
} // namespace fc END
