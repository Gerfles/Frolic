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
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
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
    // clearSwapChain();

    // // create new swap chain stuff
    // createSwapChain(windowSize, true);
    // createDepthBufferImage();
    // createFrameBuffers();
  }


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

    // Store for later reference
    /* mSwapchainFormat = surfaceFormat.format; */

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
      FcImage swapChainImage{image};

      // Create image view
      swapChainImage.createImageView(surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
      //
      mSwapchainImages.emplace_back(std::move(swapChainImage));
    }


    //
    // Create aquire semaphores and fences
    for (sizeT i = 0; i < mSwapchainImages.size(); ++i)
    {
      char debugName[256];
      snprintf(debugName, sizeof(debugName) - 1, "Semaphore: mAcquireSemaphore[%u]", i);
      mAcquireSemaphore[i] = createSemaphore(pGpu->getVkDevice(), debugName);

      //  TODO look into strlen instead of sizeof etc.
      snprintf(debugName, sizeof(debugName) - 1, "Fence: mAcquireFence[%u]            ", i);
      mAcquireFence[i] = createFence(pGpu->getVkDevice(), true, debugName);
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


// TODO older method move or delete
  // void FcSwapChain::createDepthBufferImage()
  // {
  //    // create an ordered list of formats with higher prioritization at the front of list
  //   std::vector<VkFormat> formats{VK_FORMAT_D32_SFLOAT
  //                               , VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

  //   VkFormat depthFormat = chooseSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL
  //                                                , VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  //    // create depth buffer image
  //   mDepthBufferImage.create(mSurfaceExtent.width, mSurfaceExtent.height, depthFormat, ImageTypes::Custom
  //                            , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
  //                            , VK_IMAGE_ASPECT_DEPTH_BIT, pGpu->Properties().maxMsaaSamples);
  // }



  // No longer needed in Vulkan 1.3 but but left in for devices that don't support dynamic rendering
  void FcSwapChain::createRenderPass(FcConfig& config)
  {
    VkSurfaceFormatKHR swapchainFormat = chooseSurfaceFormat(config);

    // color attachment of the render pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat.format;                              // Format to use for attachment
    colorAttachment.samples = pGpu->Properties().maxMsaaSamples;            // Number of samples to write for multi
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                   // Describes what to do with attachment before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;                 // Describes what to do with attachment after rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;        // Desribes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;      // Desribes what to do with stencil before rendering
    // Framebuffer data will be stored as an image but images can be given different data layouts to give optimal use for certain operations
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              // Image data layout before render pass starts
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Image data layout after render pass (to change to)

    // attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Image format to convert to for the subpass

    // since we are using a multisampled image as the color attachment, and this cannot be
    // presented directly, we must add a resolve attachment that will instruct the render pass to
    // resolve the multisampled color image into a regular attachment
    VkAttachmentDescription colorAttchmentResolve{};
    colorAttchmentResolve.format = swapchainFormat.format;
    colorAttchmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
//    colorAttchmentResolve.samples = FcLocator::Gpu().Properties().maxMsaaSamples;
    colorAttchmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttchmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttchmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttchmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttchmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttchmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveReference{};
    colorAttachmentResolveReference.attachment = 1;
    colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // create an ordered list of formats with higher prioritization at the front of list
    //TODO since format has alread been found in the above createdepthbufferimage() we could just
    // pass or store it so we don't have to repeat the following code...
    std::vector<VkFormat> formats{VK_FORMAT_D32_SFLOAT
                                , VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

    VkFormat depthFormat = chooseSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL
                                                 , VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // depth attachment of render pass (not necessarily how it will be used in a subpass)
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = pGpu->Properties().maxMsaaSamples;
    //depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentsReference{};
    depthAttachmentsReference.attachment = 2;
    // TODO if we follow vulkan tutorial, we could optimize this slightly but note that Ben Cook says this is
    // pretty uneccessary since this layout will be transitions on the first pass
    depthAttachmentsReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // information about a particular subpass the render pass is using
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;
    subpass.pResolveAttachments = &colorAttachmentResolveReference;
    subpass.pDepthStencilAttachment = &depthAttachmentsReference;


    // need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies;
    // conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // special value meaning outside of renderpass
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; //stage access mask (memory access)
    // but must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;

    // conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // pipeline stage
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // but must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;


    // -- CREATE RENDERPASS -- //
    std::array<VkAttachmentDescription, 3> renderPassAttachments = {colorAttachment, colorAttchmentResolve, depthAttachment};
    //
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassInfo.pAttachments = renderPassAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassInfo.pDependencies = subpassDependencies.data();

    VK_ASSERT(vkCreateRenderPass(pGpu->getVkDevice(), &renderPassInfo, nullptr, &mRenderPass));

  }


// TODO MOVE to renderpass class (older method/tile-based gpu fallback)
// A framebuffer accepts the output of the renderpass
  // void FcSwapChain::createFrameBuffers()
  // {
  //    // resize framebuffer count to equal swap chain image count
  //   mSwapChainFramebuffers.resize(mSwapchainImages.size());

  //    // create a frame buffer for each swap chain image
  //   for (size_t i = 0; i < mSwapChainFramebuffers.size(); i++)
  //   {
  //     std::array<VkImageView, 3> attachments = { mMultiSampledImage.ImageView()
  //                                              , mSwapchainImages[i].ImageView()
  //                                              , mDepthBufferImage.ImageView() };

  //     VkFramebufferCreateInfo frameBufferInfo{};
  //     frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  //     // layout pass the framebuffer will be used with
  //     frameBufferInfo.renderPass = mRenderPass;
  //     frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  //     // list of attachments (1:1 with render pass)
  //     frameBufferInfo.pAttachments = attachments.data();
  //     frameBufferInfo.width = mSurfaceExtent.width;
  //     frameBufferInfo.height = mSurfaceExtent.height;
  //     frameBufferInfo.layers = 1;

  //     if (vkCreateFramebuffer(pGpu->getVkDevice(), &frameBufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
  //     {
  //       throw std::runtime_error("Failed to create Vulkan Frame Buffer!");
  //     }
  //   }
  // }


  void FcSwapChain::present(FcImage& drawImage, VkCommandBuffer cmd)
  {
    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   TRANSITION IMAGES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // First transition draw image into transfer source layout so we can copy to the swapchain image
    drawImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Next transiton the swapchain so it can best accept an image being copied to it
    transitionImage(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // execute a copy from the draw image into the swapchain
    getFrameTexture().copyFromImage(cmd, &drawImage);

    // now transition swapchain image layout to attachment optimal so we can directly draw into it
    transitionImage(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    //
    FcLocator::Renderer().drawImGui(cmd, getFrameTexture().ImageView());

    // finally transition the swapchain image into presentable layout so we can present to surface
    transitionImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    const u64 signalValue = mCurrentFrame + imageCount();

    // DELETE
    u32 deleteThisNum = getCurrentBufferIndex();
    VkSemaphore waitSemaphore = FcLocator::Renderer().mImmediateCommands.acquireLastSubmitSemaphore();


    // 3. present image to screen when it has signalled finished rendering
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;                                       	 // Number of semaphores to wait on
    /* presentInfo.pWaitSemaphores = waitSemaphore; // semaphore to wait on */
    // // FIXME probaly not the semaphore we want
    // VkSemaphore waitSemapore = mImmediateCommands.acquireLastSubmitSemaphore();  // semaphore to wait on
    /* VkSemaphore waitSemapore = IcmdBuffer.semaphore; */
    presentInfo.pWaitSemaphores = &waitSemaphore;  // semaphore to wait on
    presentInfo.swapchainCount = 1;                                           // number of swapchains to present to
    presentInfo.pSwapchains = &mSwapchain;                      // swapchain to present images to
    presentInfo.pImageIndices = &deleteThisNum;                         //index of images in swapchains to present

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
  void FcSwapChain::getCurrentFrame()
  {
        // TODO try to remove this check
    if (mGetNextImage)
    {
      VkDevice pDevice = FcLocator::Device();

      const VkSemaphoreWaitInfo waitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO
      , .semaphoreCount = 1
      , .pSemaphores = &FcLocator::Renderer().mTimelineSemaphore
      , .pValues = &FcLocator::Renderer().mTimelineWaitValues[mCurrentBufferIndex]
      };

      VK_ASSERT(vkWaitSemaphores(pDevice, &waitInfo, U64_MAX));

      VkFence acquireFence {VK_NULL_HANDLE};

      // FIXME - try to use the other method from lvk
      // TODO add check here for maintenance_1_KHR see lvk and remove once maintenance1 becomes mandatory
      // Without VK_KHR_swapchain_maintenance1, use aquireFences to synchronize semaphore reuse
      VK_ASSERT(vkWaitForFences(pDevice, 1, &mAcquireFence[mCurrentBufferIndex], VK_TRUE, U64_MAX));
      VK_ASSERT(vkResetFences(pDevice, 1, &mAcquireFence[mCurrentBufferIndex]));

      acquireFence = mAcquireFence[mCurrentBufferIndex];

      VkSemaphore acquireSemaphore = mAcquireSemaphore[mCurrentBufferIndex];

      // FIXME this may not be the semaphore we're seeking
      /* VkSemaphore imageAvailableSemaphore = mImmediateCommandsf.acquireLastSubmitSemaphore(); */
      /* mCurrentCommandBuffer = mImmediateCommandsf.acquire(); */
      /* const CommandBufferWrapper& wrapper = mImmediateCommandsf.acquire(); */

      /* curWrap = &wrapper; */
      // don't keep adding images to the queue or commands to the buffer until last draw has finished

      // FIXME
      /* vkWaitForFences(pDevice, 1, &getCurrentFrame().renderFence, VK_TRUE, U64_MAX); */
      /* vkWaitForFences(pDevice, 1, &wrapper.fence, VK_TRUE, U64_MAX); */

      // delete any per frame resources no longer needed now the that frame has finished rendering
      // ?? this seems to be the wrong location for this, just by observation: test
      // getCurrentFrame().janitor.flush();

      // 1. get the next available image to draw to and set to signal the semaphore when we're finished with it

      /* VkSemaphore swapSemaphore = wrapper.semaphore; */

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
    }


    // TODO do we need to update the current buffer index


    // manully un-signal (close) the fence ONLY when we are sure we're submitting work (result == VK_SUCESS)
    /* vkResetFences(pDevice, 1, &getCurrentFrame().renderFence); */

    // ?? don't think we need this assert since we use semaphores and fences
    // assert(!mIsFrameStarted && "Can't call recordCommands() while frame is already in progress!");

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROM OLD METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    // TODO would this make more sense to relocate to window resize?
    // ?? also, what are the costs associated with having dynamic states
    // make sure our dynamic viewport and scissors are set properly (if resizing the window etc.)
    // mDynamicViewport.width = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().width);
// mDynamicViewport.height = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().height);
    // mDynamicScissors.extent = mSwapchain.getSurfaceExtent();
    //  //
    // vkCmdSetViewport(commandBuffer, 0, 1, &mDynamicViewport);
    // vkCmdSetScissor(commandBuffer, 0, 1, &mDynamicScissors);
  }




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

  // full destruction of swapchain, note: includes call to partial destruction of swapchain
  void FcSwapChain::destroy()
  {
    fcPrintEndl("calling: FcSwapChain::destroy");

    // destroy the render pass
    vkDestroyRenderPass(pGpu->getVkDevice(), mRenderPass, nullptr);

    clearSwapChain();

    // finally destroy the swapchain itself
    vkDestroySwapchainKHR(pGpu->getVkDevice(), mSwapchain, nullptr);
  }



} //END - namespace fc - END
