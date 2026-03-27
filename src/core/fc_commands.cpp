//>--- fc_commands.cpp ---<//
#include "fc_commands.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/log.hpp"
#include "fc_assert.hpp"
#include "utilities.hpp"
#include "fc_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdio>
#include <utility>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // VulkanImmediateCommands::VulkanImmediateCommands(VkDevice device, u32 queueFamilyIdx, const char* debugName)
  //   : mDevice(device), mQueueFamilyIdx(queueFamilyIdx), mDebugName(debugName)
  void VulkanImmediateCommands::init(VkDevice device, u32 queueFamilyIdx, const char* debugName)
  {
    fcPrintEndl("Initializing Immediate Command Buffers...");
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
      CommandBufferWrapper& buffer = mCmdBuffers[i];
      char fenceName[256] = {0};
      char semaphoreName[256] = {0};

      if (debugName)
      {
        std::snprintf(fenceName, sizeof(fenceName) - 1, "Fence: %s (commandBuffer %u)", debugName, i);
        std::snprintf(semaphoreName, sizeof(semaphoreName) - 1, "Semaphore: %s (commandBuffer %u)", debugName, i);
      }

      buffer.semaphore = createSemaphore(device, semaphoreName);
      buffer.fence = createFence(device, false, fenceName);

      VK_ASSERT(vkAllocateCommandBuffers(mDevice, &allocInfo, &buffer.cmdBufferAllocated));
      mCmdBuffers[i].handle.cmdBufferIndex = i;
     }
  }


  //
  const CommandBufferWrapper& VulkanImmediateCommands::acquire()
  {
    while(!mNumAvailableCmdBuffers)
    {
      // This loop should not run very often
      /* fcPrintEndl("Warning: not enough command buffers forcing stall"); */
      purgeCmdBuffers();
    }

    // Find the first available command buffer
    CommandBufferWrapper* current {nullptr};
    for (CommandBufferWrapper& cmdBuffer : mCmdBuffers)
    {
      if (cmdBuffer.cmdBuffer == VK_NULL_HANDLE)
      {
        current = &cmdBuffer;
        break;
      }
    }

    // TODO place  in if()
    current->handle.submitId = mSubmitCounter;
    --mNumAvailableCmdBuffers;
    current->cmdBuffer = current->cmdBufferAllocated;
    current->isEncoding = true;

    // Start recording to the current command buffer
    const VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    , .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    mNextSubmitHandle = current->handle;

    VK_ASSERT(vkBeginCommandBuffer(current->cmdBuffer, &beginInfo));

    return *current;
  }


  //
  void VulkanImmediateCommands::purgeCmdBuffers()
  {
    // TODO probably better to use kMax... instead of ARRAY_SIZE
    const u32 numBuffers = ARRAY_SIZE(mCmdBuffers);

    for (size_t i = 0; i < numBuffers; ++i)
    {
      // Start with the oldest submited buffer first
      const u32 index = i + mLastSubmitHandle.cmdBufferIndex + 1;

      // Since we start with the oldest buffer, we must wrap around after checking that one
      CommandBufferWrapper& cmdBuffer = mCmdBuffers[index % numBuffers];

      if (cmdBuffer.cmdBuffer == VK_NULL_HANDLE || cmdBuffer.isEncoding)
        continue;

      // Use a fence timeout value of 0 signals vkWaitForFences to just return current status of fence
      const VkResult result = vkWaitForFences(mDevice, 1, &cmdBuffer.fence, VK_TRUE, 0);

      // // vkWaitForFences will return VK_TIMEOUT if the fence is unsignaled
      // if (result == VK_TIMEOUT)
      //   continue;

      // // Check if the fence is signaled, is so we can reset the command buffer
      // if (result == VK_SUCCESS)
      // {
      //   vkResetFences(mDevice, 1, &cmdBuffer.fence);

      //   vkResetCommandBuffer(cmdBuffer.cmdBuffer, VkCommandBufferResetFlags{0});
      //   cmdBuffer.cmdBuffer = VK_NULL_HANDLE;
      //   ++mNumAvailableCmdBuffers;
      // }
      // else
      // {
      //   // Error if we got here since result was not either VK_SUCCESS or VK_TIMEOUT
      //   fcPrintEndl("SHOULD NOT be here: VkResult = %u", result);
      //   VK_ASSERT(result);
      // }
      // vkWaitForFences will return VK_TIMEOUT if the fence is unsignaled

      // BOOK
      // Check if the fence is signaled, is so we can reset the command buffer
      if (result == VK_SUCCESS)
      {
        vkResetFences(mDevice, 1, &cmdBuffer.fence);

        vkResetCommandBuffer(cmdBuffer.cmdBuffer, VkCommandBufferResetFlags{0});
        cmdBuffer.cmdBuffer = VK_NULL_HANDLE;
        ++mNumAvailableCmdBuffers;
      }
      else
      {
        if (result != VK_TIMEOUT)
        {
          // Error if we got here since result was not either VK_SUCCESS or VK_TIMEOUT
          fcPrintEndl("SHOULD NOT be here: VkResult = %u", result);
          VK_ASSERT(result);
        }

      }
    }
  }


  //
  SubmitHandle VulkanImmediateCommands::submit(const CommandBufferWrapper& wrapper)
  {
    FC_ASSERT(wrapper.isEncoding);
    // stop recording to the command buffer
    VK_ASSERT(vkEndCommandBuffer(wrapper.cmdBuffer));

    // Prepare the 2 optional semaphores that we can set to be waited on before GPU processses cmdBuffer
    VkSemaphoreSubmitInfo waitSemaphores[] = { {}, {} };
    u32 numWaitSemaphores = 0;
    if (mWaitSemaphore.semaphore)
    {
      waitSemaphores[numWaitSemaphores++] = mWaitSemaphore;
    }
    if (mLastSubmitSemaphore.semaphore)
    {
      waitSemaphores[numWaitSemaphores++] = mLastSubmitSemaphore;
    }

    // Prepare the 2 semaphores that are signaled after the command buffer finishes execution
    VkSemaphoreSubmitInfo signalSemaphores[] = {
      VkSemaphoreSubmitInfo{ .sType 	{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO}
                           , .stageMask {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}
                           , .semaphore {wrapper.semaphore} }
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
    , .commandBuffer {wrapper.cmdBuffer}
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

    VK_ASSERT(vkQueueSubmit2(mQueue, 1u, &submitInfo, wrapper.fence));

    mLastSubmitSemaphore.semaphore = wrapper.semaphore;
    mLastSubmitHandle = wrapper.handle;
    // Discard the wait and signal semaphores since they should only be used with one cmd buffer
    mWaitSemaphore.semaphore = VK_NULL_HANDLE;
    mSignalSemaphore.semaphore = VK_NULL_HANDLE;
    const_cast<CommandBufferWrapper&>(wrapper).isEncoding = false;

    ++mSubmitCounter;

    // Since a SubmitHandle is considered empty when its command buffer and submitId are both zero,
    // so just need to skip the zero value of submit coutnter
    // TRY to accomplish this trick in a different way
    if (mSubmitCounter == 0)
      ++mSubmitCounter;

    return mLastSubmitHandle;
  }


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
    const CommandBufferWrapper& cmdBuffer = mCmdBuffers[handle.cmdBufferIndex];
    if (cmdBuffer.cmdBuffer == VK_NULL_HANDLE)
      return true;

    // Lastly, check if the cmd buffer has been recycled and then reused, which can only happen
    // after the cmd buffer has finished execution so the submitId values will be different
    if (cmdBuffer.handle.submitId != handle.submitId)
      return true;

    // If all the above checks are false, then we can just return the status of the VkFence
    return (vkWaitForFences(mDevice, 1, &cmdBuffer.fence, VK_TRUE, 0) == VK_SUCCESS);
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
    if (!mCmdBuffers[handle.cmdBufferIndex].isEncoding)
    {
      return;
    }

    VK_ASSERT(vkWaitForFences(mDevice, 1, &mCmdBuffers[handle.cmdBufferIndex].fence, VK_TRUE, U64_MAX));

    // Since we are sure there is now at least one available command buffer we can reclaim,
    purgeCmdBuffers();
  }


  // When we wish to delete all resources (ie destructor) we can call waitAll()
  void VulkanImmediateCommands::waitAll()
  {
    VkFence fences[kMaxCommandBuffers];
    u32 fenceIndex = 0;

    for (const CommandBufferWrapper& cmdBuffer : mCmdBuffers)
    {
      if (cmdBuffer.cmdBuffer != VK_NULL_HANDLE && !cmdBuffer.isEncoding)
      {
        fences[fenceIndex++] = cmdBuffer.fence;
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
    waitAll();

    for (CommandBufferWrapper& cmdBuffer : mCmdBuffers)
    {
      vkDestroyFence(mDevice, cmdBuffer.fence, nullptr);

      vkDestroySemaphore(mDevice, cmdBuffer.semaphore, nullptr);
    }

    // Then destroy the issuing command pool
    vkDestroyCommandPool(mDevice, mCmdPool, nullptr);
  }


  //
  VkSemaphore VulkanImmediateCommands::acquireLastSubmitSemaphore()
  {
    return std::exchange(mLastSubmitSemaphore.semaphore, VK_NULL_HANDLE);
  }


  CommandBuffer::CommandBuffer(FcRenderer* renderer) : mWrapper(&renderer->mImmediateCommands.acquire())
  {
  }


} // --- namespace fc --- (END)
