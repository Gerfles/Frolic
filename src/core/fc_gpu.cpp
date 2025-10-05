#include "fc_gpu.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_locator.hpp"
#include "utilities.hpp"
// #include "log.hpp"
#include "assert.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <SDL_log.h>
// must resort to something like this or get thousands of warnings with vk_mem_alloc
//#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#define VMA_IMPLEMENTATION // must declare only once before including vk_mem_alloc.h in CPP file
#include "vk_mem_alloc.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <stdexcept>
#include <vector>
#include <iostream>
//TODO see if we can just use one or the other type of set
#include <unordered_set>
#include <set>


namespace fc
{
// TODO relocate to debug/error check header and maybe define with class, etc.
  static void check_result(VkResult result);
#define check(result) FCASSERTM( result == VK_SUCCESS, "Vulkan assert code %d", (int)result)

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   INIT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  bool FcGpu::init(const VkInstance& instance, FcWindow& window)
  {
    // first couple the window instance to the GPU (needed for surface stuff)
    pWindow = &window;

    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME};
    // TODO Make sure the extensions and layers are added to specific OSs
    //I believe the following is needed by MacOS
    //, "VK_KHR_portability_subset"};

    // First pick the best GPU
    pickPhysicalDevice(instance, deviceExtensions);

    // now that we have the GPU chosen, interface the logical device to that GPU
    if (mPhysicalGPU != VK_NULL_HANDLE)
    {
      // now create the logical device that vulkan actually uses to interface with the GPU
      if (createLogicalDevice())
      {
        // Create and initialize the VMA allocator
        VmaAllocatorCreateInfo vmaAllocatorInfo = {};
        vmaAllocatorInfo.physicalDevice = mPhysicalGPU;
        vmaAllocatorInfo.device = mLogicalGPU;
        vmaAllocatorInfo.instance = instance;
        vmaAllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        // this will let us use GPU pointers
        vmaAllocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        // ?? check thoroughly if we need to add different flags
        //vmaAllocatorInfo.flags  = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

        VkResult result = vmaCreateAllocator(&vmaAllocatorInfo, &mAllocator);
        check(result);

        return true;
      }
    }

    // we either failed to select a GPU or we were unable to create a logical device to interface with it
    return false;
  }




  void FcGpu::pickPhysicalDevice(const VkInstance& instance
                                 , const std::vector<const char*> deviceExtensions)

  // PICK THE PHYSICAL DEVICE [GRAPHICS CARD]
  {
    // query the number of graphics cardshttps://vkguide.dev/docs/new_chapter_1/vulkan_init_code/
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // don't bother going further if no gfx cards
    if (deviceCount == 0)
    {
      throw std::runtime_error("failed to find Graphics cards with Vulkan support!");
    }
    // allocate an array to hold all of the VkPhysicalDevice handles
    std::vector<VkPhysicalDevice> deviceOptions(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, deviceOptions.data());

    // check if any of the graphics cards meet the requirements we need
    for (VkPhysicalDevice potentialDevice : deviceOptions)
    {
      // check that device extension is suppported
      uint32_t extensionCount;
      vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr, &extensionCount, nullptr);
      std::vector<VkExtensionProperties> availableExtensions(extensionCount);
      vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr
                                           , &extensionCount, availableExtensions.data());

      // cool method (creation of a std::set introduces overhead, not that it matters here)
      std::set<std::string> requiredExtensions(deviceExtensions.begin() ,deviceExtensions.end());

      for (const auto& extension : availableExtensions)
      {
        requiredExtensions.erase(extension.extensionName);
      }

      if (!requiredExtensions.empty())
      {  // something in the list wasn't supported so move on to next device
        continue;
      }
      // TODO check for each specific feature in a branch so we can provide
      // alternitives in a release version

      // TODO add features to a builder vector to add and use with device creation
      // First populate the supported features, making sure to chain in the vulkan ext features
      VkPhysicalDeviceVulkan12Features extFeatures_1_2 = {};
      extFeatures_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

      VkPhysicalDeviceVulkan13Features extFeatures_1_3 = {};
      extFeatures_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
      extFeatures_1_3.pNext = &extFeatures_1_2;

      VkPhysicalDeviceFeatures2 supportedFeatures = {};
      supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      supportedFeatures.pNext = &extFeatures_1_3;

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
        // TODO should not throw an error here but might be helpful to print a list of devices and
        // print the choice of device that way the user can verify the proper device is being selected
        // in the case of multiple GPUs
      }

      // TODO think about makeing this VK...Properties2
      VkPhysicalDeviceProperties deviceProperties = {};
      vkGetPhysicalDeviceProperties(potentialDevice, &deviceProperties);

      // make sure we're using a dedicated graphics card TODO write provisions to have a fallback GPU
      // could simply add a stack where we push when a discrete GPU is found
      if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      {
        if (isDeviceSuitable(potentialDevice))
        {
          std::cout << "GPU: " << deviceProperties.deviceName
                    << "\nPush Constant Max: "
                    << deviceProperties.limits.maxPushConstantsSize
                    << " (Bytes)"
                    << std::endl;

          mPhysicalGPU = potentialDevice;
          break;
        }
      }

    } // End for(cycle through potential devices);

    // if no available device was selected, terminate
    if (mPhysicalGPU == VK_NULL_HANDLE)
    {
      throw std::runtime_error("failed to find a suitable GPU!");
    }
  }




  bool FcGpu::createLogicalDevice()
  {
    //TODO Consider changing the queue situation to make a little more logical sense - maybe creating a queue struct that has a queue member and an index member
    // Queue that logical device needs to be created
    QueueFamilyIndices indices = getQueueFamilies();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // use a set to prevent the same queue from being added more than once
    // in the case where graphics and present queue are the same
    std::set<int> queueFamilyIndices = {indices.graphicsFamily, indices.presentationFamily};

    SDL_Log("graphics family queue:%i", indices.graphicsFamily);
    SDL_Log("present family queue:%i", indices.presentationFamily);

    // vector for queue creation information, and set for family indices
    for (int queueFamilyIndex : queueFamilyIndices)
    {
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      //queueInfo.queueFamilyIndex = indices.graphicsFamily;
      queueInfo.queueFamilyIndex = queueFamilyIndex;
      queueInfo.queueCount = 1;
      // vulkan needs to know how to handle multiple queues
      // ufortunately we still haven't addressed the possibility of multiple queues
      // this is because it's often the case that the graphics and presentat queues are the the same
      // but not always.
      float priority = 1.0f; // 1 is highest, 0 is the lowest
      queueInfo.pQueuePriorities = &priority;

      queueCreateInfos.push_back(queueInfo);
    }

    // features the logical device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // enable Anisotropy
    // ?? TEST
    deviceFeatures.shaderStorageImageMultisample = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    // TODO test for this feature first
    deviceFeatures.geometryShader = VK_TRUE;
    deviceFeatures.tessellationShader = VK_TRUE;
    // TODO test for this feature first!!
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    //
    // TODO abstract this out into a builder structure
    // vulkan features to request from version 1.2
    VkPhysicalDeviceVulkan12Features features1_2 = {};
    features1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features1_2.bufferDeviceAddress = VK_TRUE;
    features1_2.descriptorIndexing = VK_TRUE;
    // TODO make this an optional branch path
    features1_2.descriptorBindingPartiallyBound = VK_TRUE;
    mGpuPerformanceProperties.isBindlessSupported = true;
    //
    features1_2.runtimeDescriptorArray = VK_TRUE;

    // vulkan features to request from version 1.3
    VkPhysicalDeviceVulkan13Features features1_3 = {};
    features1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features1_3.dynamicRendering = VK_TRUE;
    features1_3.synchronization2 = VK_TRUE;
    features1_3.pNext = &features1_2;

    VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR;
    dynamicRenderingFeatures.dynamicRenderingLocalRead = VK_TRUE;
    dynamicRenderingFeatures.pNext = &features1_3;

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
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.pNext = &dynamicRenderingFeatures;

    // createt the "logical" device
    if (vkCreateDevice(mPhysicalGPU, &deviceInfo, nullptr, &mLogicalGPU) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create vulkan logical device!");
      return false;
    }

    // Queues are created at the same time as the device, so get hadles to them
    //TRY using a pointer to VkQueue in the GPU class declaration
    vkGetDeviceQueue(mLogicalGPU, indices.graphicsFamily, 0, &mGraphicsQueue);
    vkGetDeviceQueue(mLogicalGPU, indices.presentationFamily, 0, &mPresentationQueue);

    return true;
  }



  bool FcGpu::isDeviceSuitable(const VkPhysicalDevice& device)
  {
    // first make sure device extensions are supported
    if (!isDeviceExtensionSupported(device))
    {
      std::cout << "GPU Extensions Unsupported: " << std::endl;
      return false;
    }

    // TODO refactor so widow has the swapchain details etc. maybe could use a friend class
    // or some other coupling method. Or just init the GPU by passing a fcWindow...
    // next make sure that our swapchain has the capabilities we need
    SwapChainDetails swapChain = swapChainDetails(device);
    if (swapChain.presentModes.empty() || swapChain.formats.empty())
    {
      return false;
    }

    // info about the device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // determine the max sampler anti isotropic filter -- when undersampling (having more texels than fragments)
    mGpuPerformanceProperties.maxSamplerAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;

    // determine the max Mult-Sample Anti-Aliasing that our graphics card is capable of
    // ?? might be better to have two different sample counts instead of ANDing them
    VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts
                                & deviceProperties.limits.framebufferDepthSampleCounts;

    // TODO find out if the following even works--probably a lot clearer to use multiple if-statements instead
    //  // find the highest order MSAA samples bit and set maxMsaaSamples to that
    // for (int i = 0, max32 = 0x80000000; i < 32; i++)
    // {
    //   if ((counts & max32) == max32)
    //   {
    //     mGpuPerformanceProperties.maxMsaaSamples = static_cast<VkSampleCountFlagBits>(max32 >> i);
    //     break;
//   }
    //   counts <<= 1;
    // }

    // find the highest order MSAA samples bit and set maxMsaaSamples to that
    // if (counts & VK_SAMPLE_COUNT_64_BIT) {
    //   mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_64_BIT;
    //   std::cout << "maxMSAASamples: 64_BIT" << std::endl;
    // }
    // else if (counts & VK_SAMPLE_COUNT_32_BIT) {
    //   mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_32_BIT;
    //   std::cout << "maxMSAASamples: 32_BIT" << std::endl;
    // }
    // else if (counts & VK_SAMPLE_COUNT_16_BIT) {
    //   mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_16_BIT;
    //   std::cout << "maxMSAASamples: 16_BIT" << std::endl;
    // }
    // else if (counts & VK_SAMPLE_COUNT_8_BIT) {
    //   mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;
    //   std::cout << "maxMSAASamples: 8_BIT" << std::endl;
    // }
    // else if (counts & VK_SAMPLE_COUNT_4_BIT) {
    //   mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;
    //   std::cout << "maxMSAASamples: 4_BIT" << std::endl;
    // }
    // else if (counts & VK_SAMPLE_COUNT_2_BIT) {
    //   mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;
    //   std::cout << "maxMSAASamples: 2_BIT" << std::endl;
    // }
    // else {
    //   mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    //   std::cout << "maxMSAASamples: 1_BIT" << std::endl;
    // }

    // BUG override sample count
    mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;



    // info about what features the gpu supports
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // ?? used to make sure that both queues are present (graphics and present but could do this check later or
    // potentially omit since it is probably not likely to find a GPU without graphics and present queue...)
    // get the graphics and presentation queues
    //QueueFamilyIndices indices = getQueueFamilies(device);

    return deviceFeatures.samplerAnisotropy;
  }

  // TODO pass list of required extensions to let us get rid of stored global deviceExtensions
  bool FcGpu::isDeviceExtensionSupported(const VkPhysicalDevice& device) const
  {

    // get number of extensions supported
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // if no extensions found, return failure
    if (extensionCount == 0)
    {
      return false;
    }

    // populate list of extensions
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // add all the required extentions to an unordered set for easy search
    std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    // now go through and delete all the extensions that we know are available from the required list
    for (const auto& extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    // return true if all the all the passed device extensions were found in all available extensions
    return requiredExtensions.empty();
  }



  const QueueFamilyIndices FcGpu::getQueueFamilies()
  {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalGPU, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalGPU, &queueFamilyCount, queueFamilyList.data());

    // go through each queue family and check if it has at least 1 of the required types of queues
    int i = 0;
    for (const auto& queueFamily : queueFamilyList)
    {
      // queueFamily can have no queues so make sure it has at least one (and that it's at least graphics)
      if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        indices.graphicsFamily = i;
      }

      // check if queue family supports presentation (usually graphics queue also supports presentation)
      VkBool32 presentationSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalGPU, i, pWindow->surface(), &presentationSupport);

      //
      if (queueFamily.queueCount > 0 && presentationSupport)
      {
        indices.presentationFamily = i;
      }

      //?? this may be necessary later but right now we could just break from above if statement
      if (indices.isValid())
      {
        break;
      }

      i++;
    }

    return indices;
  }



  SwapChainDetails FcGpu::swapChainDetails(const VkPhysicalDevice& device) const
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
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentModes.data());
    }

    return swapChainDetails;
  }


  void FcGpu::release(VkInstance& instance)
  {
    fcLog("Calling FcGpu::release()");


    vmaDestroyAllocator(mAllocator);

    if (mLogicalGPU != VK_NULL_HANDLE)
    {
      vkDestroyDevice(mLogicalGPU, nullptr);
    }
  }


} // namespace fc END
