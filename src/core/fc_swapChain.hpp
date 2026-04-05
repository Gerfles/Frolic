//>--- fc_swapChain.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_types.hpp"
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
     // TRY is there any issues with declaring the following pointer const?
     FcGpu* pGpu;
     VkSwapchainKHR mSwapchain;
     // TODO extrapolate into separate class
     VkRenderPass mRenderPass {nullptr};
     // FRAMEBUFFER ATTACHMENTS
     std::vector<FcImage> mSwapchainImages;
     /* std::vector<VkFramebuffer> mSwapChainFramebuffers; */

     // *-*-*-*-*-*-*-*-*-   CACHED TO AVOID RECREATION EACH FRAME   *-*-*-*-*-*-*-*-*- //
     VkRenderingAttachmentInfo mColorAttachment {};
     VkRenderingInfo mRenderInfo {};
     VkPresentInfoKHR presentInfo = {};

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     /* bool mGetNextImage {true}; */
     // FIXME act on window resize
     bool mShouldResizeWindow {false};
     u32 mCurrentBufferIndex {0}; // [0...Number of Swapchain Images)
     // TODO change to u64
     u64  mCurrentFrame {0}; // [0...+inf)
     VkFence mAcquireFences[MAX_FRAME_DRAWS] {};
     VkSemaphore mAcquireSemaphore[MAX_FRAME_DRAWS] {};
     u64 mTimelineWaitValues[MAX_FRAME_DRAWS] {};
     //



     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


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
     // DELETE
     u32 getCurrentBufferIndex() {return mCurrentBufferIndex; }


     const u64 syncTimelineSignalValue() noexcept;



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
     inline void transitionSwapchainLayout(VkCommandBuffer commandBuffer,
                                           VkImageLayout currentLayout,
                                           VkImageLayout newLayout) noexcept
      { mSwapchainImages[mCurrentBufferIndex].transitionLayout(commandBuffer, currentLayout, newLayout); }
     //
     void reCreateSwapChain(VkExtent2D windowSize);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     void present(FcImage& drawImage, VkCommandBuffer cmd);

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTER FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //
     void acquireCurrentFrame();
     //
     FcImage& getFrameTexture() { return mSwapchainImages[mCurrentBufferIndex]; }
     //
     /* const size_t imageCount() const { return mSwapChainFramebuffers.size(); } */
     // TODO delete??
     const size_t imageCount() const { return mSwapchainImages.size(); }
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
