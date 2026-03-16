//>--- fc_swapChain.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcGpu; class FcConfig; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcSwapChain
  {
   private:
     // BUG is there any issues with declaring the following pointer const?
     FcGpu* pGpu;
     VkSwapchainKHR mSwapchain;
     VkRenderPass mRenderPass {nullptr};
     // FRAMEBUFFER ATTACHMENTS
     std::vector<FcImage> mSwapchainImages;
     std::vector<VkFramebuffer> mSwapChainFramebuffers;
     //
     VkFormat createSwapChain(FcConfig& config, bool shouldReUseOldSwapchain = false);

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   HELPER FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkSurfaceFormatKHR chooseSurfaceFormat(FcConfig& config);
     //
     VkPresentModeKHR choosePresentMode(VkSurfaceKHR surface);
     //
     VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR& surfaceCapabilities, FcConfig& config);
     //
     VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats
                                    , VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
     //
     VkImageUsageFlags chooseUsageFlags(VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkFormat format);
     //
     void createRenderPass(FcConfig& config);
     //
     void createDepthBufferImage();
     //
     void createFrameBuffers();

   public:
     // -*-*-*-*-*-*-*-*-*-*-   PREVENT MOVE, COPY, ASSIGNMENT   -*-*-*-*-*-*-*-*-*-*- //
     FcSwapChain(const FcSwapChain&) = delete;
     FcSwapChain(FcSwapChain&&) = delete;
     FcSwapChain& operator=(const FcSwapChain&) = delete;
     FcSwapChain& operator=(FcSwapChain&&) = delete;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   INITIALIZATION   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcSwapChain() = default;
     //
     FcSwapChain(FcSwapChain* oldSwapChain);
     //
     VkFormat init(FcGpu& gpu, FcConfig& config);
     // TODO see if we can just make this part of create swapChain??
     // *-*-*-*-*-*-   MAKE CURRENT SWAPCHAIN FRAME INTO WRITEABLE IMAGE   *-*-*-*-*-*- //
     inline void transitionImage(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                                 VkImageLayout currentLayout, VkImageLayout newLayout) noexcept
      { mSwapchainImages[currentFrame].transitionLayout(commandBuffer, currentLayout, newLayout, 1); }
     //
     void reCreateSwapChain(VkExtent2D windowSize);
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTER FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkFramebuffer& getFrameBuffer(int index) { return mSwapChainFramebuffers[index]; }
     //
     FcImage& getFcImage(uint32_t index) { return mSwapchainImages[index]; }
     //
     const size_t imageCount() const { return mSwapChainFramebuffers.size(); }
     //
     VkRenderPass& getRenderPass()  { return mRenderPass; }
     //
     const VkSwapchainKHR& vkSwapchain() const { return mSwapchain; }
     //
     VkImage vkImage(uint32_t index)  { return mSwapchainImages.at(index).Image();  }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //     ~FcSwapChain();
     //
     void clearSwapChain();
     //
     void destroy();
  };

} //END - namespace fc - END
