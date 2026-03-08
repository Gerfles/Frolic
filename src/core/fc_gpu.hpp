//>--- fc_gpu.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_window.hpp"
#include "fc_swapChain.hpp"
#include "fc_types.hpp"
#include "fc_config.hpp"
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
     VkPhysicalDevice mPhysicalGPU {VK_NULL_HANDLE};
     VkDevice mLogicalDevice {VK_NULL_HANDLE};
     VmaAllocator mAllocator {VK_NULL_HANDLE};
     DeviceQueues mQueues;
     FcWindow* pWindow;
      // used to setup graphics capabilities, sampler attributes, etc.
     PerformanceProperties mGpuPerformanceProperties;

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   HELPERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void pickPhysicalDevice(const VkInstance& instance,
                             std::vector<const char*>& deviceExtensions,
                             FcConfig& configOptions);
     //
     bool createLogicalDevice(std::vector<const char*>& deviceExtensions, FcConfig& configOptions);

   public:
     FcGpu() = default;
     ~FcGpu() = default;
      // - INITIALIZATION -
     bool init(const VkInstance& instance, FcWindow& window, FcConfig& configOptions);

      // TODO establish convention that all getter functions are capitalized (since they "are" a type)
      // - GETTER FUNCTIONS -
     void getQueueFamilyIndicies();

     inline const VmaAllocator& getAllocator() const { return mAllocator; }
     inline const VkDevice& getVkDevice() const { return mLogicalDevice; }
     inline const VkPhysicalDevice physicalDevice() const { return mPhysicalGPU; }
     inline const VkSurfaceKHR& surface() const { return pWindow->surface(); }
     inline VkExtent2D SurfaceExtent() const { return pWindow->ScreenSize(); }
     inline const DeviceQueues& getQueues() const { return mQueues; }

     inline const VkQueue& graphicsQueue() const { return mQueues.graphicsQueue; }
     inline const VkQueue& presentQueue() const { return mQueues.graphicsQueue; }
     SwapChainDetails getSwapChainDetails(const VkPhysicalDevice& device) const;
      // const VkCommandPool commandPool() const { return mCommandPool; }
     inline const PerformanceProperties Properties() const { return mGpuPerformanceProperties; }
      // Cleanup
     void release(VkInstance& instance);
  };
} // namespace fc END
