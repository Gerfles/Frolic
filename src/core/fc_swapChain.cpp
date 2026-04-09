//>--- fc_swapChain.cpp ---<//
#include "fc_swapChain.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_assert.hpp"
#include "utilities.hpp"
#include "fc_renderer.hpp"
#include "log.hpp"
#include "fc_gpu.hpp"
#include "fc_config.hpp"
#include "fc_locator.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <imgui_impl_vulkan.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <algorithm>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  VkFormat FcSwapChain::init(FcGpu& gpu, FcConfig& config)
  {
    pGpu = &gpu;

    VkFormat swapchainImageFormat = createSwapChain(config);

    // TODO remove old vulkan renderpass methods or move to sub-vulkan initializer
    // createDepthBufferImage();
    // createRenderPass();
    // createFrameBuffers();

    return swapchainImageFormat;
  }


  // FIXME
  void FcSwapChain::reCreateSwapChain(VkExtent2D windowSize)
  {
    fcPrintEndl("Recreating Swapchain");

    // make sure nothing is getting written to or read before we re-create our swap chain
    vkDeviceWaitIdle(pGpu->getVkDevice());

    //  // toss out old swap chain stuff
    /* clearSwapChain(); */

    // // create new swap chain stuff
    // createSwapChain(windowSize, true);
    // createDepthBufferImage();
    // createFrameBuffers();
  }


  //
  VkFormat FcSwapChain::createSwapChain(FcConfig& config, bool shouldReUseOldSwapchain)
  {
    // 1. Choose best surface format
    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(config);

    // 2. Obtain the surface capabilities available
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pGpu->physicalDevice(),
                                              config.getWindowPtr()->surface(),
                                              &surfaceCapabilities);

    // 3. Choose the swapchain image count (buffering)
    // request one additional image to make sure we are not waiting on the GPU
    u32 desiredImageCount = surfaceCapabilities.minImageCount + 1;

    if (surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < desiredImageCount)
    {
      desiredImageCount = surfaceCapabilities.maxImageCount;
    }
    fcPrintEndl("Swapchain: %u buffers (image buffer count)", desiredImageCount);

    // creation information for swap chain
    VkSwapchainCreateInfoKHR swapChainInfo{};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = config.getWindowPtr()->surface();
    swapChainInfo.imageFormat = surfaceFormat.format;
    swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainInfo.presentMode = choosePresentMode(config.getWindowPtr()->surface());
    swapChainInfo.imageExtent = chooseSwapExtent(surfaceCapabilities, config);
    swapChainInfo.minImageCount = desiredImageCount;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = chooseUsageFlags(surfaceCapabilities, surfaceFormat.format);
    swapChainInfo.preTransform = surfaceCapabilities.currentTransform;
    // whether to clip parts of image not in view (e.g. behind another window, off screen, etc)
    swapChainInfo.clipped = VK_TRUE;

    // Determine how to handle blending swapchainImages with external grapics (e.g. other windows)
    if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR != 0)
    {
      swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    else
    {
      swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }

    // Determine how to share swapchain swapchainImages and how many queues we actually have
    const DeviceQueues& queues = pGpu->getQueues();
    uint32_t queueFamilyIndices[] = { queues.graphicsFamily, queues.presentationFamily };

    // check if graphics queue and present queue are the same (often the case)
    if (queues.areGraphicsAndPresentationSame())
    {
      swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      swapChainInfo.queueFamilyIndexCount = 1;
    }
    else
    {
      swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapChainInfo.queueFamilyIndexCount = 2;
    }

    swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;

    // set oldswapchain to the previous swapchain if recreating, otherwise defaults to VK_NULL_HANDLE
    // this MAY help reuse existing resources to allow new swapchain to create faster / smoother resize
    VkSwapchainKHR oldSwapchain = (shouldReUseOldSwapchain) ? mSwapchain : VK_NULL_HANDLE;
    swapChainInfo.oldSwapchain = oldSwapchain;

    // Finally, create the swapchain
    VK_ASSERT(vkCreateSwapchainKHR(pGpu->getVkDevice(), &swapChainInfo, nullptr, &mSwapchain));

    // if we are reusing resources from old swap chain, make sure to destroy them after new swapchain creation
    if (shouldReUseOldSwapchain)
    {
      vkDestroySwapchainKHR(pGpu->getVkDevice(), oldSwapchain, nullptr);
    }


    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;                                       	 // Number of semaphores to wait on
    /* presentInfo.pWaitSemaphores = waitSemaphore; // semaphore to wait on */
    // // FIXME probaly not the semaphore we want
    // VkSemaphore waitSemapore = mImmediateCommands.acquireLastSubmitSemaphore();  // semaphore to wait on
    /* VkSemaphore waitSemapore = IcmdBuffer.semaphore; */
    presentInfo.swapchainCount = 1;                                           // number of swapchains to present to
    presentInfo.pSwapchains = &mSwapchain;                      // swapchain to present images to

    // TODO place in function along with other init detailed stuff
    mColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    mColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    mColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    mColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    mRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    // TODO have swapchain dims available locally (and different from window dims in some cases)
    mRenderInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, FcLocator::ScreenDims() };
    mRenderInfo.layerCount = 1;
    mRenderInfo.colorAttachmentCount = 1;
    mRenderInfo.pColorAttachments = &mColorAttachment;
    mRenderInfo.pDepthAttachment = nullptr;
    mRenderInfo.pStencilAttachment = nullptr;

    // determine the actual amount of buffers we were able to aquire and notify if different than requested
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(pGpu->getVkDevice(), mSwapchain, &swapchainImageCount, nullptr);
    std::vector<VkImage> swapchainImages(swapchainImageCount);
    vkGetSwapchainImagesKHR(pGpu->getVkDevice(), mSwapchain, &swapchainImageCount, swapchainImages.data());

    if (swapchainImageCount != desiredImageCount)
    {
      fcPrint("Warning: Actual Swap chain Image count differs from desired Count:");
      fcPrintEndl("%u(actual) vs. %u(desired)", swapchainImageCount);
    }

    // Finally, create the actual Images that will be used in the swapchain
    for (VkImage image : swapchainImages)
    {
      FcSwapChainImage swapChainImage{image};

      // Create image view
      swapChainImage.createImageView(surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
      //
      mSwapchainImages.emplace_back(std::move(swapChainImage));

      swapChainImage.shouldTrack = false;
    }


    //
    // Create aquire semaphores and fences
    for (sizeT i = 0; i < mSwapchainImages.size(); ++i)
    {
      char debugName[256];
      snprintf(debugName, strlen(debugName), "Semaphore: mAcquireSemaphore[%u]", i);
      mAcquireSemaphore[i] = createSemaphore(pGpu->getVkDevice(), debugName);

      //  TODO look into strlen instead of sizeof etc.
      // create the fence that makes sure the draw commands of a a given frame is finished
      snprintf(debugName, strlen(debugName), "Fence: mAcquireFence[%u]            ", i);
      mAcquireFences[i] = createFence(pGpu->getVkDevice(), true, debugName);
    }

    return surfaceFormat.format;
  }


  //
  VkSurfaceFormatKHR FcSwapChain::chooseSurfaceFormat(FcConfig& config)
  {
    const VkPhysicalDevice& device = pGpu->physicalDevice();

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, config.getWindowPtr()->surface(),
                                         &formatCount, nullptr);

    std::vector<VkSurfaceFormatKHR> availableFormats;

    // if formats available, get list of them
    if (formatCount != 0)
    {
      availableFormats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, config.getWindowPtr()->surface(),
                                           &formatCount, availableFormats.data());
    }

    FC_ASSERT( !availableFormats.empty());

    // FIXME this assumes that the first entries in the formats vector are preferred (native) formats
    // for our surface and so this will set the nativeBGR boolean to true if they appear first
    bool isSurfaceNativelyBGR = false;
    for (const VkSurfaceFormatKHR& format : availableFormats)
    {
      if (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_R8G8B8A8_SRGB
          || format.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32) {
        isSurfaceNativelyBGR = false;
        break;
      }

      if (format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_SRGB
          || format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32) {
        isSurfaceNativelyBGR = true;
        break;
      }
    }

    VkSurfaceFormatKHR preferredFmt;
    using ColorSpace = FcConfig::ColorSpace;

    switch (config.requestedColorSpace)
    {
        case ColorSpace::SRGB_NONLINEAR:
          preferredFmt = {isSurfaceNativelyBGR? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM
                        , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
          break;
        case ColorSpace::SRGB_EXTENDED_LINEAR:
          if (config.isExtendedSwapchainColorSpaceEnabled()) {
            preferredFmt = {VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT};
            break;
          }
          [[fallthrough]];
        case ColorSpace::HDR10:
          if (config.isExtendedSwapchainColorSpaceEnabled()) {
            preferredFmt = {isSurfaceNativelyBGR ?
                          VK_FORMAT_A2B10G10R10_UNORM_PACK32 : VK_FORMAT_A2R10G10B10_UNORM_PACK32
                          , VK_COLOR_SPACE_HDR10_ST2084_EXT};
            break;
          }
          [[fallthrough]];
        default: // Just use normal sRGB non-linear if nothing above works
          preferredFmt = {isSurfaceNativelyBGR? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB
                        , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    // If VK_FORMAT_UNDEFINED is only format returned, it means that all formats are available
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
    {
      // since all are available, just pick what we prefer
      return preferredFmt;
    }

    // initialize to the first available format in case none are found that match our preferred
    VkSurfaceFormatKHR chosenFormat = availableFormats[0];
    bool isPreferredFormatAvailable = false;

    // See if we have the preferred format available, and if so return that
    for (const VkSurfaceFormatKHR& format : availableFormats)
    {
      // if our preferred format is availabe override chosenFormat to this format and if we don't
      // also end up matching color space, we will return this as the next best option
      if (format.format == preferredFmt.format) {
        chosenFormat = format;
        isPreferredFormatAvailable = true;

        // if our color space is also available, simply break so we can return the chosen format
        if (format.colorSpace == preferredFmt.colorSpace) {
          break;
        }
      }
    }

    if ( !isPreferredFormatAvailable)
    {
      // TODO issue this warning only if image storage (compute shaders) is being used
      fcPrintEndl("Warning: no preferred swap chain formats available: defaulting to first supported format!");
    }

    // TODO Log the chosen format
    return chosenFormat;
  }


  //
  VkPresentModeKHR FcSwapChain::choosePresentMode(VkSurfaceKHR surface)
  {
    // Get presentation modes
    const VkPhysicalDevice& device = pGpu->physicalDevice();

    uint32_t presentModesCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, nullptr);

    // if presentation modes available, get list of them
    std::vector<VkPresentModeKHR> presentModes;

    if (presentModesCount != 0)
    {
      presentModes.resize(presentModesCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, presentModes.data());
    }

    // as per vulkan spec--this mode must always be available
    VkPresentModeKHR chosenMode = VK_PRESENT_MODE_FIFO_KHR;
    std::string modeName = "FIFO";

    for (const auto& presentMode : presentModes)
    {
      // Prefer mailbox mode as that reduces tearing and helps performance
      if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        chosenMode = VK_PRESENT_MODE_MAILBOX_KHR;
        modeName = "MAILBOX";
        break;
      }

      // Second choice is immediate mode for decent performance albeit with tearing
      if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        chosenMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        modeName = "IMMEDIATE";
      }
    }

    // TODO log instead of print after release
    fcPrintEndl("Present_Mode: %s", modeName.c_str());
    return chosenMode;
  }


  //
  VkExtent2D FcSwapChain::chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities, FcConfig& config)
  {
    // if current extent is at numeric limits, then extent can vary. and we will have to determine it ourselves
    // Figure out what the resolution of the surface is in pixel size (account for high-DPI monitors)
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
      // if the value can vary, we need to set it manually so first set it to the actual window size
      VkExtent2D actualExtent = {config.windowWidth, config.windowHeight};

      // Surface also defines max and min, so make sure within boundaries by clamping values
      actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width
                                      , surfaceCapabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height
                                       , surfaceCapabilities.maxImageExtent.height);

      return actualExtent;
    }

    return surfaceCapabilities.currentExtent;
  }


  //
  VkImageUsageFlags FcSwapChain::chooseUsageFlags(VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkFormat format)
  {
    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                   | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                   | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    const bool isImageStorageSupported = (surfaceCapabilities.supportedUsageFlags
                                          & VK_IMAGE_USAGE_STORAGE_BIT) > 0;

    VkFormatProperties2 fmtProps {
      .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2
    };

    vkGetPhysicalDeviceFormatProperties2(pGpu->physicalDevice(), format, &fmtProps);

    const bool isOptimalTilingSupported = (fmtProps.formatProperties.optimalTilingFeatures
                                           & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT) > 0;

    if (isImageStorageSupported && isOptimalTilingSupported)
    {
      usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    else
    {
      fcPrintEndl("Warning: Swapchain image not usable for direct storage");
    }

    return usageFlags;
  }


  //
  // TODO move to renderpass builder
  VkFormat  FcSwapChain::chooseSupportedFormat(const std::vector<VkFormat>& formats
                                               , VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
  {
    // loop throught the format options and find a compatible one
    for (const VkFormat& format : formats)
    {
      // get properties for a given format on this device
      VkFormatProperties properties;
      vkGetPhysicalDeviceFormatProperties(pGpu->physicalDevice(), format, &properties);

      // depending on tiling choice, need to check for different bit flag
      if (tiling == VK_IMAGE_TILING_LINEAR
          && (properties.linearTilingFeatures & featureFlags) == featureFlags)
      {
        fcPrintEndl("Warning: Using Linear Tiling");
        return format;
      }
      else if (tiling == VK_IMAGE_TILING_OPTIMAL
               && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
      {
        return format;
      }
    }

    // failed to find an acceptable format
    return VK_FORMAT_UNDEFINED;
  }


  const u64 FcSwapChain::syncTimelineSignalValue() noexcept
  {
    // TODO make sure mSwapchainImages.size() returns the actuall frame buffer count and not just preallocated...
    const u64 signalValue = mCurrentFrame + mSwapchainImages.size();

    mTimelineWaitValues[mCurrentBufferIndex] = signalValue;

    return signalValue;
  }


  //
  void FcSwapChain::present(FcImage& drawImage, VkCommandBuffer cmd)
  {
    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   TRANSITION IMAGES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // First transition draw image into transfer source layout so we can copy to the swapchain image
    drawImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Next transiton the swapchain so it can best accept an image being copied to it
    mSwapchainImages[mCurrentBufferIndex].transitionToWriteMode(cmd);

    // execute a copy from the draw image into the swapchain
    getFrameTexture().copyFromImage(cmd, &drawImage);

    // TODO look into FcImage transitionLayout to see example of memImg barrier...
    // finally transition the swapchain image into presentable layout so we can present to surface
    mSwapchainImages[mCurrentBufferIndex].transitionToPresentMode(cmd);

    // BUG need to add a fence or barrier so our image layouts are correct first
    // TODO should have the ability to submit currently registered cmdBuffer without having access to that buffer
    if (mCurrentFrame <= 3)
    {
      FcCommandBuffer* currentCmd = FcLocator::Renderer().mCurrentCommandBuffer;
      FcLocator::Renderer().mImmediateCommands.submit(*currentCmd);
      FcLocator::Renderer().mCurrentCommandBuffer = &FcLocator::Renderer().mImmediateCommands.acquire();
    }


    // TODO decouple
    // BUG the first time through the draw-cycle, the last semaphore submit is not the one used in acquire
    // but rather the semaphore used with image copying etc...
    VkSemaphore waitSemaphore = FcLocator::Renderer().mImmediateCommands.acquireLastSemaphoreSubmit();

    // 3. present image to screen when it has signalled finished rendering
    presentInfo.pWaitSemaphores = &waitSemaphore; // semaphore to wait on
    presentInfo.pImageIndices = &mCurrentBufferIndex; //index of image in swapchains to present

    VkResult result = vkQueuePresentKHR(pGpu->presentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) // || mWindow.wasWindowResized())
    {
      fcPrintEndl("ERRROR OUT of date submit");
      // TODO  handle resize properly
      mShouldResizeWindow = true;
      /* mWindow.resetWindowResizedFlag(); */
      /* handleWindowResize(); */
    }
    else if (result != VK_SUCCESS)
    {
      throw std::runtime_error("Faled to submit image to Vulkan Present Queue!");
    }

    ++mCurrentFrame;
  }



  //
  void FcSwapChain::acquireCurrentFrame()
  {
    VkDevice pDevice = FcLocator::Device();

      const VkSemaphoreWaitInfo waitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO
      , .semaphoreCount = 1
      // TODO get uncouple
      , .pSemaphores = &FcLocator::Renderer().mTimelineSemaphore
      , .pValues = &mTimelineWaitValues[mCurrentBufferIndex]
      };

      VK_ASSERT(vkWaitSemaphores(pDevice, &waitInfo, U64_MAX));

      VkFence acquireFence {VK_NULL_HANDLE};

      // FIXME - try to use the other method from lvk
      // TODO add check here for maintenance_1_KHR see lvk and remove once maintenance1 becomes mandatory


      // Without VK_KHR_swapchain_maintenance1, use aquireFences to synchronize semaphore reuse
      VK_ASSERT(vkWaitForFences(pDevice, 1, &mAcquireFences[mCurrentBufferIndex], VK_TRUE, U64_MAX));
      VK_ASSERT(vkResetFences(pDevice, 1, &mAcquireFences[mCurrentBufferIndex]));

      acquireFence = mAcquireFences[mCurrentBufferIndex];

      VkSemaphore acquireSemaphore = mAcquireSemaphore[mCurrentBufferIndex];

      // delete any per frame resources no longer needed now the that frame has finished rendering
      // ?? this seems to be the wrong location for this, just by observation: test
      /* getCurrentFrame().janitor.flush(); */

      // 1. get the next available image to draw to and set to signal the semaphore when we're finished with it
      VkResult result = vkAcquireNextImageKHR(pDevice, mSwapchain, U64_MAX
                                              , acquireSemaphore
                                              , acquireFence
                                              , &mCurrentBufferIndex);



      if (result == VK_ERROR_OUT_OF_DATE_KHR)
      {
        fcPrintEndl("ERROR out of date submit1");
        mShouldResizeWindow = true;
        //handleWindowResize();
        // BUG FIXME should flag failure
        /* return mCurrentCommandBuffer; */
      }
      else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
      {
        throw std::runtime_error("Failed to acquire Vulkan Swap Chain image!");
      }

      FcLocator::Renderer().mImmediateCommands.waitSemaphore(acquireSemaphore);

    // manully un-signal (close) the fence ONLY when we are sure we're submitting work (result == VK_SUCESS)
    /* vkResetFences(pDevice, 1, &getCurrentFrame().renderFence); */
  }


  //
  // Partially free of swapchain resources -- used when resizing the window and recreating swapchain
  void FcSwapChain::clearSwapChain()
  {
    // destroy all the image views in our swapchain--the actual images and memory are freed by actual swapchain
    // FIXME
    for (auto& image : mSwapchainImages)
    {
      image.destroyImageView();
    }

    // make sure to shrink the swapchain images container in case we just need to recreateswapchain for window resize
    mSwapchainImages.clear();
  }


  //
  // full destruction of swapchain, note: includes call to partial destruction of swapchain
  void FcSwapChain::destroy()
  {
    fcPrintEndl("calling: FcSwapChain::destroy");

    clearSwapChain();

    // finally destroy the swapchain itself
    vkDestroySwapchainKHR(pGpu->getVkDevice(), mSwapchain, nullptr);
  }



} //END - namespace fc - END
