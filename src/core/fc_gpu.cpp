//>--- fc_gpu.cpp ---<//
#include "fc_gpu.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_assert.hpp"
#include "utilities.hpp"
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
  bool FcGpu::init(const VkInstance& instance, FcWindow& window)
  {
    // first couple the window instance to the GPU (needed for surface stuff)
    pWindow = &window;

    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME
                                                , VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
                                                , VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME };
    // TODO Make sure the extensions and layers are added to specific OSs
    //I believe the following is needed by MacOS
    //, "VK_KHR_portability_subset"};

    // First pick the best GPU
    pickPhysicalDevice(instance, deviceExtensions);

    // now that we have the GPU chosen, interface the logical device to that GPU
    if (mPhysicalGPU != VK_NULL_HANDLE)
    {
      // now create the logical device that vulkan actually uses to interface with the GPU
      if (createLogicalDevice(deviceExtensions))
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


  //
  void FcGpu::pickPhysicalDevice(const VkInstance& instance, std::vector<const char*>& deviceExtensions)
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
      if (areFeaturesSupported(deviceExtensions, FeatureType::DeviceExtension, potentialDevice))
      {
        continue;
      }

      // TODO check for each specific feature in a branch so we can provide
      // alternatives in a release version

      // TODO add features to a builder vector to add and use with device creation
      // First populate the supported features, making sure to chain in the vulkan ext features
      // VkPhysicalDeviceVulkan11Features extFeatures_1_1 = {

      // }
      VkPhysicalDeviceVulkan13Features extFeatures_1_3 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
      };

      VkPhysicalDeviceVulkan12Features extFeatures_1_2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
	.pNext = &extFeatures_1_3
      };

      VkPhysicalDeviceFeatures2 supportedFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      	.pNext = &extFeatures_1_2
      };

      vkGetPhysicalDeviceFeatures2(potentialDevice, &supportedFeatures);

      if (extFeatures_1_2.bufferDeviceAddress == VK_FALSE ||
          extFeatures_1_2.descriptorIndexing == VK_FALSE ||
          extFeatures_1_3.dynamicRendering == VK_FALSE ||
          extFeatures_1_3.synchronization2 == VK_FALSE ||
          // Check for support of bindless rendering so that we can automatically bind
          // an array of textures that can be used across multiple shaders and accessed
          // via index (note that this is an avaialable feature of the Nintendo Switch)
          // Gives us the ability to partially bind a descriptor, since some entries in the
          // bindless array will need to be empty
          extFeatures_1_2.descriptorBindingPartiallyBound == VK_FALSE ||
          // Gives us the binless descriptor usage with SpirV
          extFeatures_1_2.runtimeDescriptorArray == VK_FALSE)
      {
        continue;
      }

      // Make sure that our swapchain has the capabilities we need
      SwapChainDetails swapChain = getSwapChainDetails(potentialDevice);
      if (swapChain.presentModes.empty() || swapChain.formats.empty())
      {
        continue;
      }

      // Get the properties of the potential device to check to prefer GPU and set properties
      vkGetPhysicalDeviceProperties2(potentialDevice, &deviceProperties);

      // TEST (alternat OSs)
      // Prefer a dedicated grapchics card ( TODO use a priority queue with score for GPU)
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
        mGpuPerformanceProperties.maxMsaaSamples =
          static_cast<VkSampleCountFlagBits>(max_sampling_enum >> i);
        // Its set to the highest possible so exit loop
        break;
      }
      counts <<= 1;
    }

    // BUG MSAA more than one fails, probably because of post processing image
    // Should be able to enable eventually or do our own msaa sampling
    mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;

    // TODO print more specs when in debug
    fcPrintEndl("GPU: %s\nPush Constant max size:%u(Bytes)",
                deviceProperties.properties.deviceName,
                deviceProperties.properties.limits.maxPushConstantsSize);
  }


  //
  bool FcGpu::createLogicalDevice(std::vector<const char*>& deviceExtensions)
  {
    // Determine queue families that logical device needs to be created
    getQueueFamilyIndicies();

    // use a set to prevent the same queue from being added more than once
    // in the case where graphics and present queue are the same
    std::set<u32> queueFamilyIndices = { mQueues.graphicsFamily
                                       , mQueues.presentationFamily
                                       , mQueues.transferFamily
                                       , mQueues.computeFamily };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // vulkan needs to know how to handle multiple queues unfortunately we still haven't
    // addressed the possibility of multiple queues this is because it's often the case
    // that the graphics and presentat queues are the the same but not always.
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

    // features the logical device will be using
    // VkPhysicalDeviceFeatures deviceFeatures = {};
    // deviceFeatures.samplerAnisotropy = VK_TRUE; // enable Anisotropy
    // // ?? TEST
    // deviceFeatures.shaderStorageImageMultisample = VK_TRUE;
    // deviceFeatures.sampleRateShading = VK_TRUE;
    // // TODO test for this feature first
    // deviceFeatures.geometryShader = VK_TRUE;
    // deviceFeatures.tessellationShader = VK_TRUE;
    // // TODO test for this feature first!!
    // deviceFeatures.fillModeNonSolid = VK_TRUE;


    VkPhysicalDeviceFeatures2 deviceFeatures = {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.features.samplerAnisotropy = VK_TRUE; // enable Anisotropy
    // ?? TEST
    deviceFeatures.features.shaderStorageImageMultisample = VK_TRUE;
    deviceFeatures.features.sampleRateShading = VK_TRUE;
    // TODO test for this feature first
    deviceFeatures.features.geometryShader = VK_TRUE;
    deviceFeatures.features.tessellationShader = VK_TRUE;
    // TODO test for this feature first!!
    deviceFeatures.features.fillModeNonSolid = VK_TRUE;
    deviceFeatures.pNext = nullptr;

    // TODO abstract this out into a builder structure
    // vulkan features to request from version 1.2
    VkPhysicalDeviceVulkan12Features features1_2 = {};
    features1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features1_2.bufferDeviceAddress = VK_TRUE;
    features1_2.descriptorIndexing = VK_TRUE;
    // TODO make this an optional branch path
    // BUG: some of these features are not checked for
    features1_2.descriptorBindingPartiallyBound = VK_TRUE;
    features1_2.runtimeDescriptorArray = VK_TRUE;
    features1_2.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    features1_2.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    features1_2.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features1_2.pNext = &deviceFeatures;
    //
    mGpuPerformanceProperties.isBindlessSupported = true;

    // vulkan features to request from version 1.3
    VkPhysicalDeviceVulkan13Features features1_3 = {};
    features1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features1_3.dynamicRendering = VK_TRUE;
    features1_3.synchronization2 = VK_TRUE;
    features1_3.pNext = &features1_2;

    // VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR dynamicRenderingFeatures{};
    // dynamicRenderingFeatures.sType =
    //   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR;
    // dynamicRenderingFeatures.dynamicRenderingLocalRead = VK_TRUE;
    // dynamicRenderingFeatures.pNext = &features1_3;

    // VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    // indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    // indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
    // indexingFeatures.runtimeDescriptorArray = VK_TRUE;
    // indexingFeatures.pNext = &dynamicRenderingFeatures;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueFamilyIndices.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    // deprecated as of Vulkan 1.1 since instance layers cover everything now
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = nullptr;
    // finally attach our required version features
    /* deviceInfo.pEnabledFeatures = &deviceFeatures; */
    deviceInfo.pEnabledFeatures = nullptr;
    deviceInfo.pNext = &features1_3;

    // createt the "logical" device
    VK_ASSERT(vkCreateDevice(mPhysicalGPU, &deviceInfo, nullptr, &mLogicalDevice));

    // Queues are created at the same time as the device, so get hadles to them
    //TRY using a pointer to VkQueue in the GPU class declaration
    vkGetDeviceQueue(mLogicalDevice, mQueues.graphicsFamily, 0, &mQueues.graphicsQueue);
    vkGetDeviceQueue(mLogicalDevice, mQueues.presentationFamily, 0, &mQueues.presentQueue);

    return true;
  }


  //
  void FcGpu::getQueueFamilyIndicies()
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
    if (mQueues.computeFamily == DeviceQueues::INVALID)
    {
      mQueues.transferFamily = findDedicatedQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, 0);
    }

    // For graphics queue, we just need any valid graphics queue since it won't be dedicated
    mQueues.graphicsFamily = findDedicatedQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, 0);

    // check if queue family supports presentation (usually graphics queue also supports presentation)
    VkBool32 presentationSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalGPU, mQueues.graphicsFamily,
                                         pWindow->surface(), &presentationSupport);

    if (presentationSupport)
    {
      mQueues.presentationFamily = mQueues.graphicsFamily;
    }
  }



  SwapChainDetails FcGpu::getSwapChainDetails(const VkPhysicalDevice& device) const
  {
    // create a new struct to hold all the details of the available swapchain
    SwapChainDetails swapChainDetails;
    const VkSurfaceKHR& surface = pWindow->surface();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // if formats available, get list of them
    if (formatCount != 0)
    {
      swapChainDetails.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
    }

    // Get presentation modes
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

    // if presentation modes available, get list of them
    if (presentationCount != 0)
    {
      swapChainDetails.presentModes.resize(presentationCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount,
                                                swapChainDetails.presentModes.data());
    }
    return swapChainDetails;
  }


  void FcGpu::release(VkInstance& instance)
  {
    vmaDestroyAllocator(mAllocator);

    if (mLogicalDevice != VK_NULL_HANDLE)
    {
      vkDestroyDevice(mLogicalDevice, nullptr);
    }
  }


} // namespace fc END
