#include "fc_swapChain.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_renderer.hpp"
#include "core/fc_window.hpp"
#include "fc_gpu.hpp"
#include "utilities.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_events.h"
#include "SDL2/SDL_video.h"
#include "SDL2/SDL_vulkan.h"
#include "vulkan/vulkan_core.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>


namespace fc
{

  uint32_t FcSwapChain::init(FcGpu& gpu, const VkExtent2D& windowSize)
  {
    pGpu = &gpu;

    uint32_t swapchainImageCount = createSwapChain(windowSize);
    createDepthBufferImage();
    createRenderPass();
    createFrameBuffers();

    return swapchainImageCount;
  }



  void FcSwapChain::reCreateSwapChain()
  {
     // make sure nothing is getting written to or read before we re-create our swap chain
    vkDeviceWaitIdle(pGpu->getVkDevice());

     // toss out old swap chain stuff
    clearSwapChain();

    // create new swap chain stuff
    createSwapChain(mSurfaceExtent, true);
    createDepthBufferImage();
    createFrameBuffers();
  }


  uint32_t FcSwapChain::createSwapChain(const VkExtent2D& windowSize, bool shouldReUseOldSwapchain)
  {
    SwapChainDetails swapChainDetails = getSwapChainDetails();

     // 1. Choose best surface format
    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapChainDetails.formats);
     // 2. Choose best presentation modes
    VkPresentModeKHR presentMode = choosePresentMode(swapChainDetails.presentModes);
     // 3. Choose swap chain image resolution
    VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities, windowSize);

     // std::cout << "extent size: " << extent.width
     //           << " x " << extent.height << std::endl;

     // BUG this would NOT give us tripple buffering but give us double buffering if minImageCount is 1;
     // How many images are in the swap chain? get 1 more than the minimum to allow tripple buffering
     // But notice that VkSwapchainCreateInfoKHR uses minImageCount instead of imageCount
    uint32_t imageCount = MAX_FRAME_DRAWS;
     // BUG should pick one image count and stick with it.
     //  uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
    std::cout << "Swapchain image count (buffers): " << imageCount << std::endl;
     // check to make sure
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
        swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
    {
      imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
    }

     // creation information for swap chain
    VkSwapchainCreateInfoKHR swapChainInfo{};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
     // TODO doesnt really make intuitive sense that a gpu should have a notion of surface
    swapChainInfo.surface = pGpu->surface();
    swapChainInfo.imageFormat = surfaceFormat.format;
    swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.imageExtent =  extent;
    swapChainInfo.minImageCount = imageCount;
    // Number of layers for each image in chain
    swapChainInfo.imageArrayLayers = 1;
     // what attachment images will be used as
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapChainInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
     // how to handle blending images with external grapics (e.g. other windows)
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // whether to clip parts of image not in view (e.g. behind another window, off screen, etc)
    swapChainInfo.clipped = VK_TRUE;

     // Get queue family indices
    QueueFamilyIndices indices = pGpu->getQueueFamilies();

     // if graphics and presentation families are different, then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily)
    {
       // TODO should probably make indices uint32_t
      uint32_t queueFamilyIndices[] = {static_cast<uint32_t>(indices.graphicsFamily)
                                     , static_cast<uint32_t>(indices.presentationFamily) };

      swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapChainInfo.queueFamilyIndexCount = 2;
      swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else // graphics queue and present queue are the same (often the case)
    {
      swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

     // set oldswapchain to the previous swapchain if recreating, otherwise defaults to VK_NULL_HANDLE
     // this MAY help reuse existing resources to allow new swapchain to create faster / smoother resize
    VkSwapchainKHR oldSwapchain = (shouldReUseOldSwapchain) ? mSwapchain : VK_NULL_HANDLE;
    swapChainInfo.oldSwapchain = oldSwapchain;

     // Finally, create the swapchain
    if (vkCreateSwapchainKHR(pGpu->getVkDevice(), &swapChainInfo, nullptr, &mSwapchain) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a swapchain");
    }

     // if we are reusing resources from old swap chain, make sure to destroy them after new swapchain creation
    if (shouldReUseOldSwapchain)
    {
      vkDestroySwapchainKHR(pGpu->getVkDevice(), oldSwapchain, nullptr);
    }
     //

     // Store for later reference
    mSwapchainFormat = surfaceFormat.format;
    mSurfaceExtent = extent;

     //
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(pGpu->getVkDevice(), mSwapchain, &swapchainImageCount, nullptr);
    std::cout << "Actual Swap chain Image count: " << swapchainImageCount << std::endl;
    std::vector<VkImage> images(swapchainImageCount);
    vkGetSwapchainImagesKHR(pGpu->getVkDevice(), mSwapchain, &swapchainImageCount, images.data());

    for (VkImage image : images)
    {
       //TODO new section - clean out
      FcImage swapChainImage{image};

       // TODO BUG try this all in one fell swoop
       //swapChainImage.image = image;

       // Create image view
      swapChainImage.createImageView(mSwapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
       //
      mSwapchainImages.emplace_back(std::move(swapChainImage));
    }

     // TODO DELETE
    VkExtent3D temp = {mSurfaceExtent.width, mSurfaceExtent.height, 1};

     // finally create the color image and image view that will be used as multi-sampled color attachment
    mMultiSampledImage.create(temp, mSwapchainFormat
                              , pGpu->Properties().maxMsaaSamples
                              , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                              |   VK_IMAGE_USAGE_TRANSFER_DST_BIT
                               // ?? Originally set and seems useful according to documentation but maybe
                               // this is the way it was done pre-vulkan 1.3
                               // | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                              , VK_IMAGE_ASPECT_COLOR_BIT);

    return swapchainImageCount;
  }



   // Best format is subjective but ours will be:
   // format : VK_FORMAT_R8G8B8A8_UNORM
   // colorSpace : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
   // BUG This function would in not robust and only really works when desired format is available
   // so better to do would be to return false or something if fails to find our format
  VkSurfaceFormatKHR FcSwapChain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
  {
     // somewhat unintuitively, the following format means that all formats are available to use
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
       // since all are available, just pick what we prefer
      return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& format : formats)
    {
       // BUG This won't work as expected--just returns the first in the list (no prioritization for RGB)
      if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || VK_FORMAT_B8G8R8A8_UNORM)
          && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return format;
      }
    }

     // if the formats we want aren't available, just return the first format and hope it works
    return formats[0];
  }


   // TODO print the current present mode
  VkPresentModeKHR FcSwapChain::choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes)
  {
    for (const auto& presentMode : presentModes)
    {
       // TODO  we prefer mailbox mode as that reduces tearing and helps performance
      if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
         //if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        return VK_PRESENT_MODE_MAILBOX_KHR;
         //return VK_PRESENT_MODE_IMMEDIATE_KHR;
          //return presentMode;
      }
      // if (presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
      // {
      //   return VK_PRESENT_MODE_IMMEDIATE_KHR;
      // }
    }

    // TODO we should try and include a flag for the modes that allow framerate comparison
     // Present mode: Immediate is good to check for performance via FPS
     // for (const auto &availablePresentMode : availablePresentModes) {
     //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
     //     std::cout << "Present mode: Immediate" << std::endl;
     //     return availablePresentMode;
     //   }
     // }

     // as per vulkan spec--this mode must always be available
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  SwapChainDetails FcSwapChain::getSwapChainDetails()
  {
     // create a new struct to hold all the details of the available swapchain
    SwapChainDetails swapChainDetails;

    const VkPhysicalDevice& device = pGpu->physicalDevice();
    const VkSurfaceKHR& surface = pGpu->surface();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

    // std::cout << "surface capabilities size: " << swapChainDetails.surfaceCapabilities.maxImageExtent.width
    //           << " x " << swapChainDetails.surfaceCapabilities.maxImageExtent.height  << std::endl;

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



  VkExtent2D FcSwapChain::chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities, const VkExtent2D& windowSize)
  {
     // if current extent is at numeric limits, then extent can vary. and we will have to determine it ourselves

     // Figure out what the resolution of the surface is in pixel size (account for high-DPI monitors)
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
       // if the value can vary, we need to set it manually so first set it to the actual window size
      VkExtent2D actualExtent  = windowSize;

       // Surface also defines max and min, so make sure within boundaries by clamping values
      actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width
                                      , surfaceCapabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height
                                       , surfaceCapabilities.maxImageExtent.height);

      return actualExtent;
    }

     // Otherwise, it's just the size of window
    std::cout << "swapchain size: " << surfaceCapabilities.currentExtent.width
              << " x " << surfaceCapabilities.currentExtent.height << std::endl;

    return surfaceCapabilities.currentExtent;
  }


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
        std::cout << "Warning: Using Linear Tiling" << std::endl;
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


  // *-*-*-*-*-*-   MAKE CURRENT SWAPCHAIN FRAME INTO WRITEABLE IMAGE   *-*-*-*-*-*- //
  void FcSwapChain::transitionImage(VkCommandBuffer commandBuffer, uint32_t currentFrame
                                    , VkImageLayout oldLayout, VkImageLayout newLayout)
  {
    mSwapchainImages[currentFrame].transitionImage(commandBuffer, oldLayout, newLayout, 1);
  } // --- FcSwapChain::beginRendering (_) --- (END)



  void FcSwapChain::createDepthBufferImage()
  {
     // create an ordered list of formats with higher prioritization at the front of list
    std::vector<VkFormat> formats{VK_FORMAT_D32_SFLOAT_S8_UINT
                                , VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT};

    VkFormat depthFormat = chooseSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL
                                                 , VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

     // create depth buffer image

         // TODO DELETE
    VkExtent3D temp = {mSurfaceExtent.width, mSurfaceExtent.height, 1};

    mDepthBufferImage.create(temp, depthFormat, pGpu->Properties().maxMsaaSamples
                             , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
  }



  void FcSwapChain::createRenderPass()
  {
     // color attachment of the render pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = mSwapchainFormat;                              // Format to use for attachment
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
    colorAttchmentResolve.format = mSwapchainFormat;
    colorAttchmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
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
    std::vector<VkFormat> formats{VK_FORMAT_D32_SFLOAT_S8_UINT
                                , VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT};

    VkFormat depthFormat = chooseSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL
                                                 , VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

     // depth attachment of render pass (not necessarily how it will be used in a subpass)
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = pGpu->Properties().maxMsaaSamples;
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

    if (vkCreateRenderPass(pGpu->getVkDevice(), &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Render Pass!");
    }
  }



// A framebuffer accepts the output of the renderpass
  void FcSwapChain::createFrameBuffers()
  {
     // resize framebuffer count to equal swap chain image count
    mSwapChainFramebuffers.resize(mSwapchainImages.size());

     // create a frame buffer for each swap chain image
    for (size_t i = 0; i < mSwapChainFramebuffers.size(); i++)
    {
      std::array<VkImageView, 3> attachments = { mMultiSampledImage.ImageView()
                                               , mSwapchainImages[i].ImageView(), mDepthBufferImage.ImageView() };

      VkFramebufferCreateInfo frameBufferInfo{};
      frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frameBufferInfo.renderPass = mRenderPass;                                    // layout pass the framebuffer will be used with
      frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); //
      frameBufferInfo.pAttachments = attachments.data();                           // list of attachments (1:1 with render pass)
      frameBufferInfo.width = mSurfaceExtent.width;                                // framebuffer width
      frameBufferInfo.height = mSurfaceExtent.height;                              // framebuffer height
      frameBufferInfo.layers = 1;                                                  // framebuffer layers

      if (vkCreateFramebuffer(pGpu->getVkDevice(), &frameBufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create Vulkan Frame Buffer!");
      }
    }
  }


   // Partially free of swapchain resources -- used when resizing the window and recreating swapchain
  void FcSwapChain::clearSwapChain()
  {
     // free up our depth buffer image
     mDepthBufferImage.destroy();

     // destroy all the frame buffers
    for (auto& frameBuffer : mSwapChainFramebuffers)
    {
      vkDestroyFramebuffer(pGpu->getVkDevice(), frameBuffer, nullptr);
    }

     // destroy all the images views in our swapchain--the actual images and memory are freed by actual swapchain
    for (auto& image : mSwapchainImages)
    {
      image.destroyImageView();
    }
     // make sure to shrink the swapchain images container in case we just need to recreateswapchain for window resize
    mSwapchainImages.clear();

    mMultiSampledImage.destroy();
  }

   // full destruction of swapchain, note: includes call to partial destruction of swapchain
  void FcSwapChain::destroy()
  {
    std::cout << "calling: FcSwapChain::destroy" << std::endl;

    clearSwapChain();

     // destroy the render pass
    vkDestroyRenderPass(pGpu->getVkDevice(), mRenderPass, nullptr);

     // finally destroy the swapchain itself
    vkDestroySwapchainKHR(pGpu->getVkDevice(), mSwapchain, nullptr);
  }



} //END - namespace fc - END
