//>--- fc_janitor.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_commands.hpp"
#include "vk_mem_alloc.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <deque>
#include <functional>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


// TODO at the moment, this is implemented with callback functors but would
// better scale with arrays of vulkan handles to systematically delete the
// various types from a loop.
namespace fc
{
  //
  class FcJanitor
  {
   private:
     //
     struct BufferCleanup
     {
        VkBuffer buffer;
        VmaAllocation allocation;
        SubmitHandle handle;
     };
     //
     std::vector<BufferCleanup> mBuffersToDelete;
     std::deque<std::function<void()> > deletors;
     std::vector<VkDescriptorSetLayout> mDescLayouts;

   public:
     FcJanitor() = default;
     FcJanitor(const FcJanitor&) = delete;
     FcJanitor(FcJanitor&&) = delete;
     FcJanitor& operator=(const FcJanitor&) = delete;
     FcJanitor& operator=(FcJanitor&&) = delete;
     //
     inline void deleteAfterDone(VkBuffer buffer, VmaAllocation allocation, SubmitHandle handle)
      				{ mBuffersToDelete.emplace_back(BufferCleanup{buffer, allocation, handle}); }
     inline void deleteAfterDone(VkDescriptorSetLayout descLayout) {mDescLayouts.emplace_back(descLayout); }
     //
     void push_function(std::function<void()>&& function) { deletors.push_back(function); }
     //
     void flushAll();
     //
     void flushViaFunctors();
     //
     void flushBuffers();
     //
     void flushDescLayouts();
  };
}
