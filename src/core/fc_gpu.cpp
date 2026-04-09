//>--- fc_gpu.cpp ---<//
#include "fc_gpu.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_assert.hpp"
/* #include "utilities.hpp" */
#include "fc_config.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <SDL_log.h>
// must resort to something like this or get thousands of warnings with vk_mem_alloc
/* #pragma clang diagnostic ignored "-Wnullability-completeness" */
#define VMA_IMPLEMENTATION // must declare only once before including vk_mem_alloc.h in CPP file
#include "vk_mem_alloc.h"

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <set>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  bool FcGpu::init(const VkInstance instance, FcConfig& configOptions)
  {
    // First pick the best GPU
    pickPhysicalDevice(instance, configOptions);

    // now that we have the GPU chosen, interface the logical device to that GPU
    if (mPhysicalGPU != VK_NULL_HANDLE)
    {
      // now create the logical device that vulkan actually uses to interface with the GPU
      if (createLogicalDevice(configOptions))
      {
        // Create and initialize the VMA allocator
        VmaAllocatorCreateInfo vmaAllocatorInfo = {};
        vmaAllocatorInfo.physicalDevice = mPhysicalGPU;
        vmaAllocatorInfo.device = mLogicalDevice;
        vmaAllocatorInfo.instance = instance;
        vmaAllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        // this will let us use GPU pointers
        vmaAllocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        // ?? check thoroughly if we need to add different flags
        //vmaAllocatorInfo.flags  = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

        VK_ASSERT(vmaCreateAllocator(&vmaAllocatorInfo, &mAllocator));

        return true;
      }
    }

    // we either failed to select a GPU or we were unable to create a logical device to interface with it
    return false;
  }


  // TODO pass device extensions via FcConfig
  void FcGpu::pickPhysicalDevice(const VkInstance& instance, FcConfig& configOptions)
  {
    u32 deviceCount = 0;

    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // don't bother going further if no gfx devices suppport vulkan
    if (deviceCount == 0)
    {
      throw std::runtime_error("failed to find Graphics Device with Vulkan support!");
    }

    // allocate an array to hold all of the VkPhysicalDevice options available to us
    std::vector<VkPhysicalDevice> deviceOptions(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, deviceOptions.data());

    // Store device properties for later property printing of device choice
    VkPhysicalDeviceProperties2 deviceProperties = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };

    // check if any of the graphics cards meet the requirements we need
    for (VkPhysicalDevice potentialDevice : deviceOptions)
    {
      // Make sure all our device extensions are supported
      if ( !configOptions.areDeviceExtensionsSupported(potentialDevice))
      {
        continue;
      }

      VkPhysicalDeviceFeatures2 deviceFeatures_1_0 = {};
      VkPhysicalDeviceVulkan11Features deviceFeatures_1_1 = {};
      VkPhysicalDeviceVulkan12Features deviceFeatures_1_2 = {};
      VkPhysicalDeviceVulkan13Features deviceFeatures_1_3 = {};
      //
      deviceFeatures_1_0.pNext = &deviceFeatures_1_1;
      deviceFeatures_1_1.pNext = &deviceFeatures_1_2;
      deviceFeatures_1_2.pNext = &deviceFeatures_1_3;

      configOptions.populateVulkanPhysicalDeviceFeatures(deviceFeatures_1_0);

      // Check the features we need from from our selected device
      if ( !configOptions.areDeviceFeaturesSupported(potentialDevice, deviceFeatures_1_0))
      {
        continue;
      }

      // FIXME could add this feature if we really want to make sure the device support our swapchain needs
      // Make sure that our swapchain has the capabilities we need
      // SwapChainDetails swapChain = getSwapChainDetails(potentialDevice, configOptions);
      // if (swapChain.presentModes.empty() || swapChain.formats.empty())
      // {
      //   continue;
      // }

      // Get the properties of the potential device to check to prefer GPU and set properties
      vkGetPhysicalDeviceProperties2(potentialDevice, &deviceProperties);

      // TEST (alternate OSs)
      // Prefer a dedicated grapchics card ( TODO use a priority queue with score for GPU, etc.)
      if (deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      {
        mPhysicalGPU = potentialDevice;
        break;
      }
      else if (deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
      {
        mPhysicalGPU = potentialDevice;
        continue;
      }

    } // ---  (_) --- (END)

    // if no available device was selected, terminate
    if (mPhysicalGPU == VK_NULL_HANDLE)
    {
      throw std::runtime_error("failed to find a suitable GPU!");
    }

    // *-*-*-*-*-*-*-*-*-*-*-*-   SET OUR FC_GPU PROPERTIES   *-*-*-*-*-*-*-*-*-*-*-*- //
    // determine the max sampler anisotropic filter when undersampling (having more texels than fragments)
    mGpuPerformanceProperties.maxSamplerAnisotropy = deviceProperties
                                                     .properties.limits.maxSamplerAnisotropy;

    // determine the max Mult-Sample Anti-Aliasing that our graphics card is capable of
    VkSampleCountFlags counts = deviceProperties.properties.limits.framebufferColorSampleCounts
                                & deviceProperties.properties.limits.framebufferDepthSampleCounts;

    // find the highest order MSAA samples bit and set maxMsaaSamples to that
    for (int i = 0, max_sampling_enum = 0x00000040; i < 8; i++)
    {
      if ((counts & max_sampling_enum) == max_sampling_enum)
      {
        mGpuPerformanceProperties.maxMsaaSamples = static_cast<VkSampleCountFlagBits>
                                                   (max_sampling_enum >> i);
        // Its set to the highest possible so exit loop
        break;
      }
      counts <<= 1;
    }

    // BUG MSAA more than one fails, probably because of post processing image
    // Should be able to enable eventually or do our own msaa sampling
    mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;

    // TODO print more specs when in debug
    fcPrintEndl("GPU: %s\n     -Push Constant max size:%u(Bytes)",
                deviceProperties.properties.deviceName,
                deviceProperties.properties.limits.maxPushConstantsSize);
  }


  //
  bool FcGpu::createLogicalDevice(FcConfig& configOptions)
  {
    // Determine queue families that logical device needs to be created
    getQueueFamilyIndicies(configOptions);

    // use a set to prevent the same queue from being added more than once
    // in the case where graphics and present queue are the same (usually the case)
    std::set<u32> queueFamilyIndices = { mQueues.graphicsFamily
                                       , mQueues.presentationFamily
                                       , mQueues.transferFamily
                                       , mQueues.computeFamily };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // Let vulkan know how to handle multiple queues.
    for (int queueFamilyIndex : queueFamilyIndices)
    {
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = queueFamilyIndex;
      queueInfo.queueCount = 1;

      float priority = 1.0f;
      queueInfo.pQueuePriorities = &priority;

      queueCreateInfos.push_back(queueInfo);
    }

    //
    VkPhysicalDeviceFeatures2 deviceFeatures_1_0 = {};
    VkPhysicalDeviceVulkan11Features deviceFeatures_1_1 = {};
    VkPhysicalDeviceVulkan12Features deviceFeatures_1_2 = {};
    VkPhysicalDeviceVulkan13Features deviceFeatures_1_3 = {};
    //
    deviceFeatures_1_0.pNext = &deviceFeatures_1_1;
    deviceFeatures_1_1.pNext = &deviceFeatures_1_2;
    deviceFeatures_1_2.pNext = &deviceFeatures_1_3;

    configOptions.populateVulkanPhysicalDeviceFeatures(deviceFeatures_1_0);

    //FIXME
    mGpuPerformanceProperties.isBindlessSupported = true;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueFamilyIndices.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount = configOptions.getDeviceExtensionCount();
    deviceInfo.ppEnabledExtensionNames = configOptions.getDeviceExtensions();
    // deprecated as of Vulkan 1.1 since instance layers cover everything now
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = nullptr;
    // Because enabled feaures uses a VkPhysicalDeviceFeatures2, we must set .pEnabled
    // features to null and pass via pNext instead
    deviceInfo.pEnabledFeatures = nullptr;
    deviceInfo.pNext = &deviceFeatures_1_0;

    // createt the "logical" device
    VK_ASSERT(vkCreateDevice(mPhysicalGPU, &deviceInfo, nullptr, &mLogicalDevice));

    // Queues are created at the same time as the device, so get hadles to them
    //TRY using a pointer to VkQueue in the GPU class declaration
    vkGetDeviceQueue(mLogicalDevice, mQueues.graphicsFamily, 0, &mQueues.graphicsQueue);
    vkGetDeviceQueue(mLogicalDevice, mQueues.presentationFamily, 0, &mQueues.presentQueue);

    return true;
  }


  //
  void FcGpu::getQueueFamilyIndicies(FcConfig& config)
  {
    // Populate the list of queues
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalGPU, &queueFamilyCount, nullptr);

    // new
    std::vector<VkQueueFamilyProperties2> properties(queueFamilyCount);
    for (VkQueueFamilyProperties2& property : properties)
    {
      property.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    }

    vkGetPhysicalDeviceQueueFamilyProperties2(mPhysicalGPU, &queueFamilyCount, properties.data());

    // Lambda function to try and acquire dedicated queue by passing the queue that you don't want
    // to double up on (usually the graphics queue) as the second argument
    auto findDedicatedQueueFamilyIndex = [&properties](VkQueueFlags require, VkQueueFlags avoid) -> u32
     {
       for (u32 i = 0; i != properties.size(); i++)
       {
         const VkQueueFamilyProperties& property = properties[i].queueFamilyProperties;
         const bool isSuitable = (property.queueFlags & require) == require;
         const bool isDedicated = (property.queueFlags & avoid) == 0;

         if (property.queueCount && isSuitable && isDedicated)
           return i;
       }
       return DeviceQueues::INVALID;
     };

    // Get dedicated queue for compute
    mQueues.computeFamily = findDedicatedQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);

    // if we couldn't find a dedicated compute queue above, settle for any valid queue
    if (mQueues.computeFamily == DeviceQueues::INVALID)
    {
      mQueues.computeFamily = findDedicatedQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, 0);
    }

    // Get dedicated queue for transfer
    mQueues.transferFamily = findDedicatedQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT);

    // if we couldn't find a dedicated compute queue above, settle for any valid queue
    if (mQueues.transferFamily == DeviceQueues::INVALID)
    {
      mQueues.transferFamily = findDedicatedQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, 0);
    }

    // For graphics queue, we just need any valid graphics queue since it won't be dedicated
    mQueues.graphicsFamily = findDedicatedQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, 0);

    // check if queue family supports presentation (usually graphics queue also supports presentation)
    VkBool32 presentationSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalGPU, mQueues.graphicsFamily,
                                         config.getWindowPtr()->surface(),
                                         &presentationSupport);
    if (presentationSupport)
    {
      mQueues.presentationFamily = mQueues.graphicsFamily;
    }

    // TODO set configuration parameters based on whether queues are discreet or not
    // ?? See if there is a benefit to having graphics and present queue be different
    fcPrint("\n -Queue Layout-");
    fcPrint("\nCOMPUTE: index = %u", mQueues.computeFamily);
    fcPrint("\nTRANSFER: index = %u", mQueues.transferFamily);
    fcPrint("\nGRAPHICS: index = %u", mQueues.graphicsFamily);
    fcPrintEndl("\nPRESENTATION: index = %u", mQueues.presentationFamily);
  }


  //
  void FcGpu::release(VkInstance instance)
  {
    vmaDestroyAllocator(mAllocator);

    if (mLogicalDevice != VK_NULL_HANDLE)
    {
      vkDestroyDevice(mLogicalDevice, nullptr);
    }
  }


} // namespace fc END
