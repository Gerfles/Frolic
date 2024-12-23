#pragma once


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include "SDL2/SDL_video.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <vector>


namespace fc
{
   // FORWARD DECLARATIONS
   // ?? better to place here or with member variable
  class FcGpu;

// ?? could maybe get rid of this
  struct SwapChainDetails
  {
     VkSurfaceCapabilitiesKHR surfaceCapabilities; // surface properties, eg image extent
     std::vector<VkSurfaceFormatKHR> formats;      // surface image formats, eg R8G8B8A8_UNORM
     std::vector<VkPresentModeKHR> presentModes;   // presentaion capabilities, eg Mailbox, fifo
  };

  class FcSwapChain
  {

   private:
      // BUG is there any issues with declaring the following pointer const?
     FcGpu* pGpu;
     VkSwapchainKHR mSwapchain;
     VkExtent2D mSurfaceExtent;
     VkFormat mSwapchainFormat;
     VkRenderPass mRenderPass;
      //std::vector<SwapchainImage> mSwapchainImages;
      // FRAMEBUFFER ATTACHMENTS
     std::vector<FcImage> mSwapchainImages;
     FcImage mMultiSampledImage;
     FcImage mDepthBufferImage;

     std::vector<VkFramebuffer> mSwapChainFramebuffers;
     uint32_t createSwapChain(const VkExtent2D& windowSize, bool shouldReUseOldSwapchain = false);

// -*-*-*-*-*-*-*-*-*-*-   PREVENT MOVE, COPY, ASSIGNMENT   -*-*-*-*-*-*-*-*-*-*- //
     FcSwapChain(const FcSwapChain&) = delete;
     FcSwapChain(FcSwapChain&&) = delete;
     FcSwapChain& operator=(const FcSwapChain&) = delete;
     FcSwapChain& operator=(FcSwapChain&&) = delete;

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-   HELPER FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     SwapChainDetails getSwapChainDetails();
     VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
     VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes);
     VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities, const VkExtent2D& windowSize);
     VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats
                                    , VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
     void createRenderPass();
     void createDepthBufferImage();
     void createFrameBuffers();
   public:
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   INITIALIZATION   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcSwapChain() = default;
     FcSwapChain(FcSwapChain* oldSwapChain);
     uint32_t init(FcGpu& gpu, const VkExtent2D& windowSize);
      // TODO see if we can just make this part of create swapChain??
     void transitionImage(VkCommandBuffer commandBuffer, uint32_t currentFrame
                          , VkImageLayout oldLayout, VkImageLayout newLayout);
     void reCreateSwapChain();
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTER FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkFramebuffer& getFrameBuffer(int index) { return mSwapChainFramebuffers[index]; }
     FcImage& getFcImage(uint32_t index) { return mSwapchainImages[index]; }
     const size_t imageCount() const { return mSwapChainFramebuffers.size(); }
     const VkExtent2D& getSurfaceExtent() const { return mSurfaceExtent; }
     const VkFormat& getFormat() const { return mSwapchainFormat; }
     VkRenderPass& getRenderPass()  { return mRenderPass; }
     const VkSwapchainKHR& vkSwapchain() const { return mSwapchain; }
    VkImage vkImage(uint32_t index)  { return mSwapchainImages.at(index).Image();  }
      // CLEANUP
//     ~FcSwapChain();
     void clearSwapChain();
     void destroy();
  };







} //END - namespace fc - END
