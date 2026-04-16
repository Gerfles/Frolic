//>--- fc_commands.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <utility>
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  struct SubmitHandle
  {
     u32 cmdBufferIndex {0};
     u32 submitId {0};
     //
     SubmitHandle() = default;
     //
     explicit SubmitHandle(u64 handle) : cmdBufferIndex(u32(handle & 0xffffffff)),
                                         submitId(u32(handle >> 32)) {}
     //
     inline const bool isEmpty() const { return submitId == 0; }
     //
     inline u64 handle() const { return (u64(submitId) << 32) + cmdBufferIndex; }
  };


  //
  class FcCommandBuffer
  {
     friend class VulkanImmediateCommands;

   private:
     VkCommandBuffer mCmdBuffer {VK_NULL_HANDLE};
     VkCommandBuffer mCmdBufferAllocated {VK_NULL_HANDLE};
     SubmitHandle mHandle {};
     VkFence mFence {VK_NULL_HANDLE};
     VkSemaphore mSemaphore {VK_NULL_HANDLE};
     bool mIsEncoding {false};

   public:
     FcCommandBuffer() = default;
     //
     /* ~CommandBuffer() { FC_ASSERT(!mIsRendering); }; */
     FcCommandBuffer& operator=(FcCommandBuffer&& other) = default;
     inline const VkCommandBuffer getVkCmdBuffer() const noexcept {return mCmdBuffer; }
     inline const VkFence getFence() const noexcept { return mFence; }
     inline const SubmitHandle getHandle() const noexcept { return mHandle; }
     inline const VkSemaphore getSemaphore() const noexcept { return mSemaphore; }
     inline void setIsEncoding(bool setting) noexcept { mIsEncoding = setting; }
     inline const bool IsEncoding() noexcept { return mIsEncoding; }
  };


  // TODO rename to something more intuitive
  class VulkanImmediateCommands
  {
   private:
     static constexpr u32  kMaxCommandBuffers = 128;
     // TODO make static since unless we only have one instance
     VkDevice mDevice {VK_NULL_HANDLE};
     // TODO have multiple queues that we are working with
     VkQueue mQueue {VK_NULL_HANDLE};
     VkCommandPool mCmdPool {VK_NULL_HANDLE};
     u32 mQueueFamilyIdx {0};
     u32 mNumAvailableCmdBuffers {kMaxCommandBuffers};
     u32 mSubmitCounter {1};
     const char* mDebugName {""};
     SubmitHandle mLastSubmitHandle;
     SubmitHandle mNextSubmitHandle;
     FcCommandBuffer mCmdBuffers[kMaxCommandBuffers];
     VkFence mImmediateFence;
     //
     VkSemaphoreSubmitInfo mLastSemaphoreSubmit {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO
                                               , .semaphore = VK_NULL_HANDLE
                                               , .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
     // TODO dedicate and rename to mImgAcquireSemaphoreInfo as well as below
     VkSemaphoreSubmitInfo mWaitSemaphore {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO
                                         , .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
     // mSignalSemaphore is injected at the end of the frame (by calling signalSemaphore() ), before
     // presenting the final image to the screen and is used to orchestrate the swapchain presentation
     VkSemaphoreSubmitInfo mSignalSemaphore {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO
                                           , .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
     //
     void purgeCmdBuffers();
     //
   public:

     // TODO removed for now may add back in later
     /* VulkanImmediateCommands(VkDevice device, u32 queueFamilyIdx, const char* debugName); */
     ~VulkanImmediateCommands();
     //
     FcCommandBuffer& acquire();
     //
     void init(VkDevice device, u32 queueFamilyIdx, const char* debugName);
     //
     SubmitHandle submit(FcCommandBuffer& wrapper);
     //
     SubmitHandle submitSingleUseCmdBuffer(FcCommandBuffer& cmdBuffer);
     //
     void waitSemaphore(VkSemaphore semaphore);
     //
     void signalSemaphore(VkSemaphore semaphore, u64 signalValue);
     //
     inline VkSemaphore acquireLastSemaphoreSubmit() noexcept
     // TODO extrapolate std::exchange to FcUtilities
      { return std::exchange(mLastSemaphoreSubmit.semaphore, VK_NULL_HANDLE); }
     //
     inline SubmitHandle getLastSubmitHandle() const { return mLastSubmitHandle; }
     //
     inline SubmitHandle getNextSubmitHandle() const { return mNextSubmitHandle; }
     //
     bool isReady(const SubmitHandle handle) const ;
     //
     void wait(const SubmitHandle handle);
     //
     void waitAll();
     //
     void destroy();
  };






} // --- namespace fc --- (END)
