#pragma once


// Frolic Engine
#include "fc_window.hpp"
#include "fc_swapChain.hpp"
#include "utilities.hpp"
//#include "vulkan/vulkan_core.h"
// external libraries
#include <vulkan/vulkan.hpp>
// std libraries
#include <vector>



namespace fc
{

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
       VkPhysicalDevice mPhysicalGPU = VK_NULL_HANDLE;
       VkDevice mLogicalGPU = VK_NULL_HANDLE;
        // handles to Queues (graphics is often the same as presentation)
       VkQueue mGraphicsQueue;
       VkQueue mPresentationQueue;
       FcWindow* pWindow;
       VkCommandPool mCommandPool = VK_NULL_HANDLE;
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
       const VkDevice& VkDevice() const { return mLogicalGPU; }
       const VkPhysicalDevice& physicalDevice() const { return mPhysicalGPU; }
       const VkSurfaceKHR& surface() const { return pWindow->surface(); }
       VkExtent2D SurfaceExtent() const { return pWindow->ScreenSize(); }
       QueueFamilyIndices GpuQueueFamilies() const { return getQueueFamilies(mPhysicalGPU); }
       const VkQueue& graphicsQueue() const { return mGraphicsQueue; }
       const VkQueue& presentQueue() const { return mPresentationQueue; }
       SwapChainDetails swapChainDetails(const VkPhysicalDevice& device) const;
       const VkCommandPool commandPool() const { return mCommandPool; }
       const PerformanceProperties Properties() const { return mGpuPerformanceProperties; }
        // Cleanup
       void release(VkInstance& instance);
    };
} // namespace fc END

