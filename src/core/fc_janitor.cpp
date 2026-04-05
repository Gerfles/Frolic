//>--- fc_janitor.cpp ---<//
#include "fc_janitor.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_gpu.hpp"
#include "fc_renderer.hpp"
#include "fc_locator.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  void FcJanitor::flushAll()
  {
    // TODO call all flush functions
  }


  void FcJanitor::flushViaFunctors()
  {
    // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
    {
      (*it)(); // call functors
    }

    deletors.clear();
  }

  void FcJanitor::flushBuffers()
  {
    std::vector<BufferCleanup>::iterator it = mBuffersToDelete.begin();
    // TODO make branchless by having 3 separate vectors and referencing the frame we're on
    while (it != mBuffersToDelete.end() && FcLocator::Renderer().mImmediateCommands.isReady(it->handle))
    {
      // Destroy the VMA buffer allocation which will in turn call vkDestroyBuffer and vkFreeMemory
        vmaDestroyBuffer(FcLocator::Gpu().getAllocator(), it->buffer, it->allocation);
        ++it;
    }

    // Since we exited the while loop at point where commands were not ready, delete up until that point
    mBuffersToDelete.erase(mBuffersToDelete.begin(), it);
  }


  //
  void FcJanitor::flushDescLayouts()
  {
    for (VkDescriptorSetLayout& layout : mDescLayouts)
    {
      vkDestroyDescriptorSetLayout(FcLocator::Device(), layout, nullptr);
    }

    //
    mDescLayouts.erase(mDescLayouts.begin(), mDescLayouts.end());
  }

}// --- namespace fc --- (END)
