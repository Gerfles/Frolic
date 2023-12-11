#include "fc_gpu.hpp"


// Frolic Engine
#include "utilities.hpp"
// external libraries
#include "vulkan/vulkan_core.h"
// std libraries
#include <stdexcept>
#include <vector>
#include <iostream>
//TODO see if we can just use one or the other type of set
#include <unordered_set>
#include <set>



namespace fc
{
  
  bool FcGpu::init(const VkInstance& instance, FcWindow& window)
  {
     // first couple the window instance to the GPU (needed for surface stuff)
    pWindow = &window;

    

    //  // enumerate all the physical devices the vkInstance can access
    // uint32_t deviceCount = 0;
    // vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    // std::vector<VkPhysicalDevice> deviceList(deviceCount);
    // vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

    //  // right now, just pick the first available device
    //  // TODO pick the best device 
    // for (const auto& device : deviceList)
    // {
    //   if (isDeviceSuitable(device))
    //   {
    //     mPhysicalGPU = device;
    //     break;
    //   }
    // }

    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME
                                                     , "VK_KHR_portability_subset"};
    pickPhysicalDevice(instance, deviceExtensions);

     // now that we have the GPU chosen, interface the logical device to that GPU
    if (mPhysicalGPU != VK_NULL_HANDLE)
    {
       // now create the logical device that vulkan actually uses to interface with the GPU
      if (createLogicalDevice())
      {
         // create the command pool for later allocating command from
        createCommandPool();
        return true;
      }
    }

     // we either failed to select a GPU or we were unable to create a logical device to interface with it 
    return false;
  }




  void FcGpu::pickPhysicalDevice(const VkInstance& instance, const std::vector<const char*> deviceExtensions)
// PICK THE PHYSICAL DEVICE [GRAPHICS CARD]
{
   // query the number of graphics cards
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

   // don't bother going further if no gfx cards
  if (deviceCount == 0)
  {
    throw std::runtime_error("failded to find Graphics cards with Vulkan support!");
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

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(potentialDevice, &deviceProperties);

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(potentialDevice, &supportedFeatures);

     // make sure we're using a dedicated graphics card
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
      if (isDeviceSuitable(potentialDevice))
      {
        std::cout << "GPU: " << deviceProperties.deviceName
                  << "\nPush Constant Max: " << deviceProperties.limits.maxPushConstantsSize << " (Bytes)"
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
    QueueFamilyIndices indices = getQueueFamilies(mPhysicalGPU);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
     // use a set to prevent the same queue from being added more than once
    std::set<int> queueFamilyIndices = {indices.graphicsFamily, indices.presentationFamily};

     // vector for queue creation information, and set for family indices
    for (int queueFamilyIndex : queueFamilyIndices)
    {
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = indices.graphicsFamily;
      queueInfo.queueCount = 1;
       // vulkan needs to know how to handle multiple queues
       // ufortunately we still haven't addressed the possibility of multiple queues
       // this is because it's often the case that the graphics and presentat queues are the the same
       // but not always.
      float priority = 1.0f; // 1 is highest, 0 is the lowest 
      queueInfo.pQueuePriorities = &priority;

      queueCreateInfos.push_back(queueInfo); 
    }

    
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueFamilyIndices.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data(); 
     // deprecated as of Vulkan 1.1 since instance layers cover everything now
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = nullptr;

     // features the logical device will be using 
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceFeatures.samplerAnisotropy = VK_TRUE; // enable Anisotropy
    
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
      return false;
    }

     // TODO refactor so widow is has the swapchain details etc. maybe could use a friend class
     // or some other coupling method. Or just init the GPU by passing a fcWindow...
     // next make sure that our swapchain has the capabilities we need
    SwapChainDetails swapChain = swapChainDetails(device);
    bool swapChainValid;
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
    if (counts & VK_SAMPLE_COUNT_64_BIT) {
      mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_64_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_32_BIT) {
      mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_32_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_16_BIT) {
      mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_16_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_8_BIT) {
      mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_4_BIT) {
      mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_2_BIT) {
      mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;
    }
    else {
      mGpuPerformanceProperties.maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    }
    
     // info about what features the gpu supports
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
     // get the graphics and presentation queues 
    QueueFamilyIndices indices = getQueueFamilies(device);


     // make sure that both queues are present
    return indices.isValid() && deviceFeatures.samplerAnisotropy;
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

  

  QueueFamilyIndices FcGpu::getQueueFamilies(const VkPhysicalDevice& device) const
  {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

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
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, pWindow->surface(), &presentationSupport);

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


   // TODO could consider relocatting this to fcrenderer
  void FcGpu::createCommandPool()
  {
     // get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = GpuQueueFamilies();
    
    VkCommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(mLogicalGPU, &commandPoolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Command Pool!");
    }
  }


  
  VkCommandBuffer FcGpu::beginCommandBuffer() const 
  {
     // command buffer to hold transfer commands
    VkCommandBuffer commandBuffer;

     // command buffer details
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

     // allocate command buffer from pool
    vkAllocateCommandBuffers(mLogicalGPU, &allocInfo, &commandBuffer);
 
     // information to be the command buffer record
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
     // TODO ?? could maybe create commmand Buffers that will be reused for very consistent
     // items, such as image transfer etc. and then just reuse those anytime we want to do the Op
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // becomes invalid after submit

     // begin recording transfer commands
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
  }


  
  void FcGpu::submitCommandBuffer(VkCommandBuffer commandBuffer) const
  {
     // End commands 
    vkEndCommandBuffer(commandBuffer);
    
     // Queue submission information
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

     // submit transfer command to transfer queue and wait until it finishes
    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

     // TODO NOT THE MOST EFFICIENT WAY TO DO THIS SINCE IT LOADS MESHES ONE AT A TIME
     // BETTER TO CREATE SOME KIND OF SYNCHRONIZATION
    vkQueueWaitIdle(mGraphicsQueue);

     // free temporary command buffer back to the pool
    vkFreeCommandBuffers(mLogicalGPU, mCommandPool, 1, &commandBuffer);
  }

  
  
  
  void FcGpu::release(VkInstance& instance)
  {
     // TODO test that these conditionals are working as intended 
    if (mCommandPool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(mLogicalGPU, mCommandPool, nullptr);
    }


    if (mLogicalGPU != VK_NULL_HANDLE)
    {
      vkDestroyDevice(mLogicalGPU, nullptr); 
    }
  }

  
} // namespace fc END
