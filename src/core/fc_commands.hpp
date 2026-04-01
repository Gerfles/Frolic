//>--- fc_commands.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_assert.hpp"
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcRenderer;

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



struct CommandBufferWrapper
     {
        VkCommandBuffer cmdBuffer {VK_NULL_HANDLE};
        VkCommandBuffer cmdBufferAllocated {VK_NULL_HANDLE};
        SubmitHandle handle {};
        VkFence fence {VK_NULL_HANDLE};
        VkSemaphore semaphore {VK_NULL_HANDLE};
        bool isEncoding {false};
     };



  class VulkanImmediateCommands final
  {
   private:
     static constexpr u32  kMaxCommandBuffers = 64;

     // TODO make static since unless we only have one instance
     VkDevice mDevice {VK_NULL_HANDLE};
     VkQueue mQueue {VK_NULL_HANDLE};
     VkCommandPool mCmdPool {VK_NULL_HANDLE};
     u32 mQueueFamilyIdx {0};
     u32 mNumAvailableCmdBuffers {kMaxCommandBuffers};
     u32 mSubmitCounter {1};
     const char* mDebugName {""};
     SubmitHandle mLastSubmitHandle {SubmitHandle()};
     SubmitHandle mNextSubmitHandle {SubmitHandle()};
     CommandBufferWrapper mCmdBuffers[kMaxCommandBuffers];
     //
     VkSemaphoreSubmitInfo mLastSubmitSemaphore {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO
                                               , .semaphore = VK_NULL_HANDLE
                                               , .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};

     VkSemaphoreSubmitInfo mWaitSemaphore {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO
                                         , .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
     // mSignalSemaphore is injected at the end of the frame (by calling signalSemaphore() ), before
     // presenting the final image to the screen and is used to orchestrate the swapchain presentation
     VkSemaphoreSubmitInfo mSignalSemaphore {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO
                                           , .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
     void purgeCmdBuffers();

   public:

     // TODO removed for now may add back in later
     /* VulkanImmediateCommands(VkDevice device, u32 queueFamilyIdx, const char* debugName); */
     ~VulkanImmediateCommands();
     const CommandBufferWrapper& acquire();
     void init(VkDevice device, u32 queueFamilyIdx, const char* debugName);
     SubmitHandle submit(const CommandBufferWrapper& wrapper);
     void waitSemaphore(VkSemaphore semaphore);
     void signalSemaphore(VkSemaphore semaphore, u64 signalValue);
     VkSemaphore acquireLastSubmitSemaphore();
     inline SubmitHandle getLastSubmitHandle() const { return mLastSubmitHandle; }
     inline SubmitHandle getNextSubmitHandle() const { return mNextSubmitHandle; }
     bool isReady(const SubmitHandle handle) const ;
     void wait(const SubmitHandle handle);
     void waitAll();
  };


  //
  class CommandBuffer
  {
   public:
     CommandBuffer() = default;
     //
     explicit CommandBuffer(FcRenderer* renderer);
     ~CommandBuffer() { FC_ASSERT(!mIsRendering); };
     CommandBuffer& operator=(CommandBuffer&& other) = default;
     // TODO eliminate
     /* operator VkCommandBuffer() const { return getVkCommandBuffer(); } */
     VkCommandBuffer getVkCommandBuffer() const {return mWrapper ? mWrapper->cmdBuffer : VK_NULL_HANDLE;}

   private:

     friend class FcRenderer;
     /* FcRenderer* mRenderer = nullptr; */
     const CommandBufferWrapper* mWrapper = nullptr;
     SubmitHandle mLastSubmitHandle {};
     // DELETE
     bool mIsRendering = false;
  };



} // --- namespace fc --- (END)
