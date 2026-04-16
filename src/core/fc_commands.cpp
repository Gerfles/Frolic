//>--- fc_commands.cpp ---<//
#include "fc_commands.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/log.hpp"
#include "fc_assert.hpp"
#include "utilities.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdio>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // VulkanImmediateCommands::VulkanImmediateCommands(VkDevice device, u32 queueFamilyIdx, const char* debugName)
  //   : mDevice(device), mQueueFamilyIdx(queueFamilyIdx), mDebugName(debugName)
  void VulkanImmediateCommands::init(VkDevice device, u32 queueFamilyIdx, const char* debugName)
  {
    fcPrint("Initializing Immediate Command Buffers... ");
    mDevice = device;
    mQueueFamilyIdx = queueFamilyIdx;
    mDebugName = debugName;

    vkGetDeviceQueue(device, queueFamilyIdx, 0, &mQueue);

    // Create the command pool that will issue our command buffers
    const VkCommandPoolCreateInfo poolInfo = {
      .sType {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}
    , .queueFamilyIndex {queueFamilyIdx}
    , .flags {VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT}
    };

    vkCreateCommandPool(device, &poolInfo, nullptr, &mCmdPool);

    // Preallocate command buffers from the created command pool
    const VkCommandBufferAllocateInfo allocInfo = {
      .sType {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}
    , .level {VK_COMMAND_BUFFER_LEVEL_PRIMARY}
    , .commandBufferCount {1}
    , .commandPool {mCmdPool}
    };

    for (sizeT i = 0; i < kMaxCommandBuffers; ++i)
    {
      FcCommandBuffer& buffer = mCmdBuffers[i];
      char fenceName[256] = {0};
      char semaphoreName[256] = {0};

      if (debugName)
      {
        std::snprintf(fenceName, sizeof(fenceName) - 1, "Fence: %s (commandBuffer %u)", debugName, i);
        std::snprintf(semaphoreName, sizeof(semaphoreName) - 1, "Semaphore: %s (commandBuffer %u)", debugName, i);
      }

      buffer.mSemaphore = createSemaphore(device, semaphoreName);
      buffer.mFence = createFence(device, false, fenceName);

      VK_ASSERT(vkAllocateCommandBuffers(mDevice, &allocInfo, &buffer.mCmdBufferAllocated));
      mCmdBuffers[i].mHandle.cmdBufferIndex = i;
     }


    VkFenceCreateInfo fenceInfo {
      .sType {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}
    , .flags {VK_FENCE_CREATE_SIGNALED_BIT}
    };

    VK_ASSERT(vkCreateFence(mDevice, &fenceInfo, nullptr, &mImmediateFence));

    fcPrintEndl("DONE");
  }


  //
  FcCommandBuffer& VulkanImmediateCommands::acquire()
  {
    while(!mNumAvailableCmdBuffers)// == 0)
    {
      // This loop should not run very often
      /* fcPrintEndl("Warning: not enough command buffers forcing stall"); */
      purgeCmdBuffers();
    }

    // Find the first available command buffer
    FcCommandBuffer* current {nullptr};
    for (FcCommandBuffer& cmdBuffer : mCmdBuffers)
    {
      if (cmdBuffer.mCmdBuffer == VK_NULL_HANDLE)
      {
        current = &cmdBuffer;
        break;
      }
    }

    // TODO place  in if()
    current->mHandle.submitId = mSubmitCounter;
    --mNumAvailableCmdBuffers;
    current->mCmdBuffer = current->mCmdBufferAllocated;
    current->mIsEncoding = true;

    // Start recording to the current command buffer
    const VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    , .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    // TODO this may be problematic if command buffers are being used asynchronously
    mNextSubmitHandle = current->mHandle;

    VK_ASSERT(vkBeginCommandBuffer(current->mCmdBuffer, &beginInfo));

    return *current;
  }


  // TODO determine if there is a way to automatically purge cmd buffers a little more cleanly
  void VulkanImmediateCommands::purgeCmdBuffers()
  {
    // TODO probably better to use kMax... instead of ARRAY_SIZE
    const u32 numBuffers = ARRAY_SIZE(mCmdBuffers);

    for (size_t i = 0; i < numBuffers; ++i)
    {
      // Start with the oldest submited buffer first
      const u32 index = i + mLastSubmitHandle.cmdBufferIndex + 1;

      // Since we start with the oldest buffer, we must wrap around after checking that one
      FcCommandBuffer& cmdBuffer = mCmdBuffers[index % numBuffers];

      // Skip reseting this cmd buffer if it is already null or is currently being used
      if (cmdBuffer.mCmdBuffer == VK_NULL_HANDLE || cmdBuffer.mIsEncoding)
        continue;

      // Use a fence timeout value of 0 signals vkWaitForFences to just return current status of fence
      const VkResult result = vkWaitForFences(mDevice, 1, &cmdBuffer.mFence, VK_TRUE, 0);

      // vkWaitForFences will return VK_TIMEOUT if the fence is unsignaled (cmdBuffer is still in use)
      if (result == VK_TIMEOUT)
        continue;

      // The fence is signaled, so we can reset the command buffer and fence
      if (result == VK_SUCCESS)
      {
        vkResetFences(mDevice, 1, &cmdBuffer.mFence);

        vkResetCommandBuffer(cmdBuffer.mCmdBuffer, VkCommandBufferResetFlags{0});
        cmdBuffer.mCmdBuffer = VK_NULL_HANDLE;
        ++mNumAvailableCmdBuffers;
      }
      else
      {
        // Error if we got here since result was not either VK_SUCCESS or VK_TIMEOUT
        VK_ASSERT(result);
      }
    }
  }



  // begin single
  SubmitHandle VulkanImmediateCommands::submitSingleUseCmdBuffer(FcCommandBuffer& cmdBuffer)
  {
    // TODO copy of submit() for now but implement a better submit for single use cmd buffer
    FC_ASSERT(cmdBuffer.IsEncoding());

    // stop recording to the command buffer
    VK_ASSERT(vkEndCommandBuffer(cmdBuffer.getVkCmdBuffer()));

    // Prepare the 2 optional semaphores that we can set to be waited on before GPU processses cmdBuffer
    // VkSemaphoreSubmitInfo waitSemaphores[] = { {}, {} };
    // u32 numWaitSemaphores = 0;
    // if (mWaitSemaphore.semaphore)
    // {
    //   waitSemaphores[numWaitSemaphores++] = mWaitSemaphore;
    // }

    // ?? Related to rendering frames
    // if (mLastSemaphoreSubmit.semaphore)
    // {
    //   waitSemaphores[numWaitSemaphores++] = mLastSemaphoreSubmit;
    // }

    // Prepare the 2 semaphores that are signaled after the command buffer finishes execution
    // VkSemaphoreSubmitInfo signalSemaphores[] = {
    //   VkSemaphoreSubmitInfo{.sType {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO}
    //                       , .stageMask {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}
    //                       , .semaphore {cmdBuffer.getSemaphore()} }
    // , VkSemaphoreSubmitInfo {}
    // };

    // TRY to get rid of num's
    // u32 numSignalSemaphores = 1;
    // // Check to see if the optional timeline semaphore has been activated (by signalSemaphore())
    // if (mSignalSemaphore.semaphore)
    // {
    //   // TODO get rid of this num...++ and just put a 1 here
    //   signalSemaphores[numSignalSemaphores++] = mSignalSemaphore;
    // }

    //
    const VkCommandBufferSubmitInfo cmdSubmitInfo = {
      .sType {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO}
    , .commandBuffer {cmdBuffer.getVkCmdBuffer()}
    };

    // const VkSubmitInfo2 submitInfo = {
    //   .sType {VK_STRUCTURE_TYPE_SUBMIT_INFO_2}
    // , .waitSemaphoreInfoCount {numWaitSemaphores}
    // , .pWaitSemaphoreInfos {waitSemaphores}
    // , .commandBufferInfoCount {1u}
    // , .pCommandBufferInfos {&cmdSubmitInfo}
    // , .signalSemaphoreInfoCount {numSignalSemaphores}
    // , .pSignalSemaphoreInfos {signalSemaphores}
    // };
    const VkSubmitInfo2 submitInfo = {
      .sType {VK_STRUCTURE_TYPE_SUBMIT_INFO_2}
    , .waitSemaphoreInfoCount {0u}
    , .pWaitSemaphoreInfos {VK_NULL_HANDLE}
    , .commandBufferInfoCount {1u}
    , .pCommandBufferInfos {&cmdSubmitInfo}
    , .signalSemaphoreInfoCount {0u}
    , .pSignalSemaphoreInfos {VK_NULL_HANDLE}
    };

    // TODO submit to a different and specific queue...
    VK_ASSERT(vkQueueSubmit2(mQueue, 1u, &submitInfo, cmdBuffer.getFence()));

    /* mLastSemaphoreSubmit.semaphore = cmdBuffer.getSemaphore(); */
    /* mLastSubmitHandle = cmdBuffer.getHandle(); */

    // Discard the wait and signal semaphores since they should only be used with one cmd buffer
    // mWaitSemaphore.semaphore = VK_NULL_HANDLE;
    // mSignalSemaphore.semaphore = VK_NULL_HANDLE;
    // TODO BUG
    /* vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) */

    /* const_cast<CommandBuffer&>(cmdBuffer).isEncoding = false; */
    cmdBuffer.setIsEncoding(false);

    ++mSubmitCounter;

    // Since a SubmitHandle is considered empty when its command buffer and submitId are both zero,
    // so just need to skip the zero value of submit coutnter
    // TRY to accomplish this "trick" in a different way
    if (mSubmitCounter == 0)
      ++mSubmitCounter;


    vkWaitForFences(mDevice, 1, &mImmediateFence, true, U64_MAX);

    return mLastSubmitHandle;
  }


  //
  SubmitHandle VulkanImmediateCommands::submit(FcCommandBuffer& cmdBuffer)
  {
    FC_ASSERT(cmdBuffer.IsEncoding());
    // stop recording to the command buffer
    VK_ASSERT(vkEndCommandBuffer(cmdBuffer.getVkCmdBuffer()));

    // Prepare the 2 optional semaphores that we can set to be waited on before GPU processses cmdBuffer
    VkSemaphoreSubmitInfo waitSemaphores[] = { {}, {} };
    u32 numWaitSemaphores = 0;
    if (mWaitSemaphore.semaphore)
    {
      waitSemaphores[numWaitSemaphores++] = mWaitSemaphore;
    }
    if (mLastSemaphoreSubmit.semaphore)
    {
      waitSemaphores[numWaitSemaphores++] = mLastSemaphoreSubmit;
    }

    // Prepare the 2 semaphores that are signaled after the command buffer finishes execution
    VkSemaphoreSubmitInfo signalSemaphores[] = {
      VkSemaphoreSubmitInfo{ .sType 	{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO}
                           , .stageMask {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}
                           , .semaphore {cmdBuffer.getSemaphore()} }
    , VkSemaphoreSubmitInfo {}
    };

    // TRY to get rid of num's
    u32 numSignalSemaphores = 1;
    // Check to see if the optional timeline semaphore has been activated (by signalSemaphore())
    if (mSignalSemaphore.semaphore)
    {
      // TODO get rid of this num...++ and just put a 1 here
      signalSemaphores[numSignalSemaphores++] = mSignalSemaphore;
    }

    //
    const VkCommandBufferSubmitInfo cmdSubmitInfo = {
      .sType {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO}
    , .commandBuffer {cmdBuffer.getVkCmdBuffer()}
    };

    const VkSubmitInfo2 submitInfo = {
      .sType {VK_STRUCTURE_TYPE_SUBMIT_INFO_2}
    , .waitSemaphoreInfoCount {numWaitSemaphores}
    , .pWaitSemaphoreInfos {waitSemaphores}
    , .commandBufferInfoCount {1u}
    , .pCommandBufferInfos {&cmdSubmitInfo}
    , .signalSemaphoreInfoCount {numSignalSemaphores}
    , .pSignalSemaphoreInfos {signalSemaphores}
    };

    VK_ASSERT(vkQueueSubmit2(mQueue, 1u, &submitInfo, cmdBuffer.getFence()));

    mLastSemaphoreSubmit.semaphore = cmdBuffer.getSemaphore();
    mLastSubmitHandle = cmdBuffer.getHandle();
    // Discard the wait and signal semaphores since they should only be used with one cmd buffer
    mWaitSemaphore.semaphore = VK_NULL_HANDLE;
    mSignalSemaphore.semaphore = VK_NULL_HANDLE;


    /* const_cast<CommandBuffer&>(cmdBuffer).isEncoding = false; */
    cmdBuffer.setIsEncoding(false);

    ++mSubmitCounter;

    // Since a SubmitHandle is considered empty when its command buffer and submitId are both zero,
    // so just need to skip the zero value of submit coutnter
    // TRY to accomplish this trick in a different way
    if (mSubmitCounter == 0)
      ++mSubmitCounter;

    return mLastSubmitHandle;
  }


  //
  void VulkanImmediateCommands::waitSemaphore(VkSemaphore semaphore)
  {
    FC_ASSERT(mWaitSemaphore.semaphore == VK_NULL_HANDLE);

    mWaitSemaphore.semaphore = semaphore;
  }

  void VulkanImmediateCommands::signalSemaphore(VkSemaphore semaphore, u64 signalValue)
  {
    FC_ASSERT(mSignalSemaphore.semaphore == VK_NULL_HANDLE);

    mSignalSemaphore.semaphore = semaphore;
    mSignalSemaphore.value = signalValue;
  }


  // Basically a high level equivalent of vkWaitForFences() with timeout set to 0 to get current fence status
  bool VulkanImmediateCommands::isReady(const SubmitHandle handle) const
  {
    // First make sure we don't just have an empty submit handle
    if (handle.isEmpty())
      return true;

    // Next check if the command buffer has already been recycled by purge()
    const FcCommandBuffer& cmdBuffer = mCmdBuffers[handle.cmdBufferIndex];
    if (cmdBuffer.mCmdBuffer == VK_NULL_HANDLE)
      return true;

    // Lastly, check if the cmd buffer has been recycled and then reused, which can only happen
    // after the cmd buffer has finished execution so the submitId values will be different
    if (cmdBuffer.mHandle.submitId != handle.submitId)
      return true;

    // If all the above checks are false, then we can just return the status of the VkFence
    return (vkWaitForFences(mDevice, 1, &cmdBuffer.mFence, VK_TRUE, 0) == VK_SUCCESS);
  }


  // If we call wait() with an empty submit handle, it will invoke vkDeviceWaitIdle() (useful for debug)
  void VulkanImmediateCommands::wait(const SubmitHandle handle)
  {
    if (handle.isEmpty())
    {
      vkDeviceWaitIdle(mDevice);
      return;
    }

    if (isReady(handle))
      return;

    // FIXME BUG
    /* if (!FC_ASSERT(!mCmdBuffers[handle.bufferIndex].isEncoding)) */
    if (!mCmdBuffers[handle.cmdBufferIndex].mIsEncoding)
    {
      return;
    }

    VK_ASSERT(vkWaitForFences(mDevice, 1, &mCmdBuffers[handle.cmdBufferIndex].mFence, VK_TRUE, U64_MAX));

    // Since we are sure there is now at least one available command buffer we can reclaim,
    purgeCmdBuffers();
  }


  // When we wish to delete all resources (ie destructor) we can call waitAll()
  void VulkanImmediateCommands::waitAll()
  {
    VkFence fences[kMaxCommandBuffers];
    u32 fenceIndex = 0;

    for (const FcCommandBuffer& cmdBuffer : mCmdBuffers)
    {
      if (cmdBuffer.mCmdBuffer != VK_NULL_HANDLE && !cmdBuffer.mIsEncoding)
      {
        fences[fenceIndex++] = cmdBuffer.mFence;
      }
    }

    if (fenceIndex != 0)
    {
      VK_ASSERT(vkWaitForFences(mDevice, fenceIndex, fences, VK_TRUE, U64_MAX));
    }

    // Reclaim all the command buffers we waited for
    purgeCmdBuffers();
  }


  //
  VulkanImmediateCommands::~VulkanImmediateCommands()
  {
    // waitAll();

    // for (FcCommandBuffer& cmdBuffer : mCmdBuffers)
    // {
    //   vkDestroyFence(mDevice, cmdBuffer.mFence, nullptr);

    //   vkDestroySemaphore(mDevice, cmdBuffer.mSemaphore, nullptr);
    // }

    // // Then destroy the issuing command pool
    // vkDestroyCommandPool(mDevice, mCmdPool, nullptr);
  }

  void VulkanImmediateCommands::destroy()
  {
    FC_DEBUG_LOG("Destroying: Immediate conmmands");

    waitAll();

    for (FcCommandBuffer& cmdBuffer : mCmdBuffers)
    {
      vkDestroyFence(mDevice, cmdBuffer.mFence, nullptr);

      vkDestroySemaphore(mDevice, cmdBuffer.mSemaphore, nullptr);
    }

    vkDestroyFence(mDevice, mImmediateFence, nullptr);

    // Then destroy the issuing command pool
    vkDestroyCommandPool(mDevice, mCmdPool, nullptr);
  }


  // CommandBuffer::CommandBuffer(FcRenderer* renderer) : mWrapper(&renderer->mImmediateCommands.acquire())
  // {

  // }


} // --- namespace fc --- (END)
