#pragma once

// - FROLIC ENGINE -
//#include "fc_gpu.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include "vk_mem_alloc.h"
// - STD LIBRARIES -

// TODO could think about having a static FcBuffer commandBuffer for transfers,
// etc. that all FcBuffers would use (for things like staging transfers) but
// probably better implemented within the renderer instead...

// TODO It should be noted that in a real world application, you're not supposed to actually call
//vkAllocateMemory for every individual buffer. The maximum number of simultaneous memory
//allocations is limited by the maxMemoryAllocationCount physical device limit, which may be as low
//as 4096 even on high end hardware like an NVIDIA GTX 1080. The right way to allocate memory for a
//large number of objects at the same time is to create a custom allocator that splits up a single
//allocation among many different objects by using the offset parameters that we've seen in many
//functions.  You can either implement such an allocator yourself, or use the VulkanMemoryAllocator
//library provided by the GPUOpen initiative.


// TODO restructure with using mBufferSize instead of passing in for each
// function
// TODO should all FcBuffers contain binding information? or maybe create a
// inhereted class FcBoundBuffer that contains that stuff?
namespace fc
{
  class FcBuffer
  {
   private:
     VkBuffer mBuffer{nullptr};
     VmaAllocation mAllocation;
//     VmaAllocationInfo info;
     VkDeviceSize mBufferSize;

   public:
     FcBuffer() = default;
     ~FcBuffer() = default;
     FcBuffer& operator=(const FcBuffer&) = delete;
      // ?? This must be included to allow vector.pushBack(Fcbuffer) ?? not sure if there's a better way... maybe unique_ptr
      // FcBuffer(const FcBuffer&) = delete;
     void allocateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags useFlags);
     void storeData(void* sourceData, VkDeviceSize bufferSize, VkBufferUsageFlags useFlags);
     void overwriteData(void * srcData, size_t dataSize);
     void copyBuffer(const FcBuffer& srcBuffer, VkDeviceSize bufferSize);
     VkBuffer& getVkBuffer() { return mBuffer; }
      // BUG this doesn't always have size storred
     VkDeviceSize size() { return mBufferSize; }
     void* getAddres();
     void destroy();
  };

} // namespace fc _END_
