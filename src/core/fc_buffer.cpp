//>--- fc_buffer.cpp ---<//
#include "fc_buffer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_assert.hpp"
#include "fc_renderer.hpp"
#include "fc_locator.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstring>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace  fc
{
  //
  void FcBuffer::allocate(VkDeviceSize bufferSize, FcBufferTypes bufferType)
  {
    isDestroyed = false;
    mSize = bufferSize;
    mBufferType = bufferType;

    // TODO better handle the case where we try to create an empty buffer
    // if (mSize == 0)
    // {
    //   return;
    // }
    // -*-*-*-*-*-*-*-*-*-*-   DEFAULT BUFFER CREATION DETAILS   -*-*-*-*-*-*-*-*-*-*- //
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = mSize;
    bufferInfo.usage = 0;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


    VmaAllocationCreateInfo allocInfo = {};
    // TODO performace test different mem type flags & verify local things be local
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // *-*-*-*-*-*-*-*-*-*-*-*-   CUSTOMIZE BUFFER DETAILS   *-*-*-*-*-*-*-*-*-*-*-*- //
    // TODO  implement defaults for prefered memory in a struct that contains bufferInfo.usage
    // and allocInfo.flags, etc. so that we can populate the defaults on vulkan initialization
    // and just keep them handy for less logic when creating buffers/images/etc.
    switch (bufferType)
    {
        case FcBufferTypes::Staging:
        {
          bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT;
          allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
          // ?? This may allow easier mapping for pixel read if needed
          /* |VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT; */
          break;
        }
        case FcBufferTypes::Vertex:
        {
          bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
          allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
          allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
          break;
        }
        case FcBufferTypes::Index:
        {
          bufferInfo.usage =  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
          break;
        }
        case FcBufferTypes::Uniform:
        {
          // TODO implement the situation where BAR memory is not available
          // Using VMA flags like this tries to force use of BAR memory if it's available
          // and if not it will use GPU dedicated memory but then we must check for that and
          // explicitly transfer from a local CPU copy to GPU if that's the case
          allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
          // | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
          // | VMA_ALLOCATION_CREATE_MAPPED_BIT;
          bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                             // This flag is needed if BAR mem is not available->requires transfer
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT;
          break;
        }
        case FcBufferTypes::Gpu:
        {
          // May want to rename eventually but for now this is a buffer we can send to the gpu
          // for things like drawIndirect
          bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                             VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
          allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
          break;
        }
        case FcBufferTypes::Custom:
        {
          // TODO implement
          break;
        }

        default:
          break;
    }

    // TODO VMA creates a new VkBuffer each call. Sub-allocation of parts of one
    // large buffer is recommended for commercial implementation

    // TODO set names to see what isn't deleted;
    /* vmaSetAllocationName(VmaAllocator  _Nonnull allocator, VmaAllocation  _Nonnull allocation, const char * _Nullable pName); */
    VK_ASSERT(vmaCreateBuffer(FcLocator::Gpu().getAllocator(), &bufferInfo
                              , &allocInfo, &mBuffer, &mAllocation, nullptr));
  }



  // TODO check preferred (fastest) method via VMA website
  void* FcBuffer::getAddress()
  {
    if (mSize == 0)
    {
      return nullptr;
    }
    /* void* memAddress; */
    if (mMemoryAddress == nullptr)
    {
      VmaAllocator allocator = FcLocator::Gpu().getAllocator();
      VK_ASSERT(vmaMapMemory(allocator, mAllocation, &mMemoryAddress));
    }

    return mMemoryAddress;
    // ?? The following method may also work with some research...
    /* return mAllocation->GetMappedData(); */
  }


  //
  //
  VkDeviceAddress FcBuffer::getVkDeviceAddress() const
  {
    // find the address for the vertex buffer
    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = mBuffer;

    return vkGetBufferDeviceAddress(FcLocator::Device(), &deviceAddressInfo);
  }


  // TODO TEST all uses of write() etc. to see if it would be more efficient to pass a
  // cmd buffer and allow write to be one of the commands within that buffer (if it's part of the render cycle)
  void FcBuffer::write(bool isTesting, VkCommandBuffer cmd, void* sourceData, size_t dataSize, VkDeviceSize offset)
  {
    // if no dataSize passed in (default = 0), set copy length to whole buffer size
    if (dataSize == 0)
    {
      dataSize = mSize;
    }

    // -*-*-*-*-*-*-*-*-*-   SIMPLE BUFFER STORED IN HOST MEMORY   -*-*-*-*-*-*-*-*-*- //
    if (mBufferType == FcBufferTypes::Staging)
    {
      // Appears to be a simple buffer request so just create buffer and store data
      overwriteData(sourceData, dataSize, offset);
    }
    else // TODO consider taking this one branch out to reduce logic for updating UBOs and just renaming updateUBO()
    {
      // *-*-*-*-*-   MORE COMPLICATED BUFFER REQUIRING TRANSFER TO GPU MEM   *-*-*-*-*- //
      // First create separate staging buffer and store sourceData in RAM
      FcBuffer stagingBuffer(dataSize, FcBufferTypes::Staging);
      stagingBuffer.overwriteData(sourceData, dataSize, offset);
      //
      copyBuffer(cmd, stagingBuffer, dataSize, isTesting);
      //
      stagingBuffer.deferredDestroy();
    }
  }


// TODO Relocate to method that doesn't require VMA, in case it's not available or prefer fine grained
//      // GET BUFFER MEMORY REQUIREMENTS
//     VkMemoryRequirements memRequirments;
//     vkGetBufferMemoryRequirements(gpu.getVkDevice(), mBuffer, &memRequirments);
//      // get properties of physical device memory
//      // TODO abstract out the memProperties situation and have locator store the (use a map maybe)
//      // TODO could also store Physical device in locator if we can't abstract out memproperties
//     VkPhysicalDeviceMemoryProperties memProperties;
//     vkGetPhysicalDeviceMemoryProperties(gpu.physicalDevice(), &memProperties);

//      // ALLOCATE MEMORY TO BUFFER
//     VkMemoryAllocateInfo memoryAllocInfo{};
//     memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//     memoryAllocInfo.allocationSize = memRequirments.size;

//      // cycle through all the memory types available and choose the one that has our required properties
//     uint32_t memoryTypeIndex = -1;
//     for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
//     {
// // first, only pick each allowed type of memory passed in (skip evaluation when that type is not
// // bit-enabled by our allowedType parameter)
//       if ((memRequirments.memoryTypeBits & (1 << i))
//           && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) // then make sure that the allowed types also contains the property flags that we request by passing the vkmemorypropertyflags
//       {
//          // this memory type is an allowed type and it has the properties we want (flags) return
//         memoryTypeIndex = i;
//         break;
//       }
//     }

//      // Quit if we can't find the proper memory type
//      // ?? Tutorial doesn't contain this check so maybe Vulkan is required to return something...
//     if (memoryTypeIndex == -1)
//     {
//       throw std::runtime_error("Failed to find a suitable memory type!");
//     }

//     memoryAllocInfo.memoryTypeIndex = memoryTypeIndex;

//      // allocate memory to VkDeviceMemory
//     VK_ASSERT(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &mBufferMemory));
//     {
//       throw std::runtime_error("Failed to allocate Vulkan Device Memory for FcBuffer!");
//     }

//      // allocate memory to given vertex buffer
//     vkBindBufferMemory(device, mBuffer, mBufferMemory, 0);
//  }


  //
  //
  // TODO
  void FcBuffer::overwriteData(void* sourceData, size_t dataSize, VkDeviceSize offset)
  {
    /* void* memAddress; */

    VmaAllocator allocator = FcLocator::Gpu().getAllocator();
    // VK_ASSERT(vmaMapMemory(allocator, mAllocation, &mMemoryAddress));
    // mMemoryAddress = (char*)mMemoryAddress + offset;

    // memcpy(mMemoryAddress, sourceData, dataSize);

    // vmaUnmapMemory(allocator, mAllocation);


    // ?? Note that the following function will also accomplish the same thing but
    // in a more robust manner -- though it may be a bit slower, this also flushes
    // memory when needed so should really look into.
    vmaCopyMemoryToAllocation(allocator, sourceData, mAllocation, offset, dataSize);

    // TODO also, SHould we just use this ??
    //mAllocation->GetMappedData();
    // mMemoryAddress = (char*)mMemoryAddress + offset;

    // memcpy(mMemoryAddress, sourceData, dataSize);

    //
    /* fcPrintEndl("unmapping:"); */
    //vmaUnmapMemory(allocator, mAllocation);
    /* fcPrintEndl("unmaped:"); */
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    // void* memAddress;

    // VmaAllocator allocator = FcLocator::Gpu().getAllocator();

    // VK_ASSERT(vmaMapMemory(allocator, mAllocation, &memAddress));

    // // ?? Note that the following function will also accomplish the same thing but
    // // in a more robust manner -- though it may be a bit slower, this also flushes
    // // memory when needed so should really look into.
    // //vmaCopyMemoryToAllocation(allocator, sourceData, mAllocation, offset, dataSize);

    // // TODO also, SHould we just use this ??
    // //mAllocation->GetMappedData();
    // memAddress = (char*)memAddress + offset;

    // memcpy(memAddress, sourceData, dataSize);

    // vmaUnmapMemory(allocator, mAllocation);
  } // --- FcBuffer::writeData (_) --- (END)




  /* TODO implement */
  void FcBuffer::fetchData(uint32_t location, size_t dataSize)
  {
    // void* memAddress;
    // VmaAllocator allocator = FcLocator::Gpu().getAllocator();

    // VK_ASSERT(vmaMapMemory(allocator, mAllocation, &memAddress));
    // {
    //   throw std::runtime_error("Failed to map VMA buffer memory!");
    // }

    // return *(memAddress + (x + y * mImageExtent.width))
  }


  //
  void FcBuffer::copyBuffer(VkCommandBuffer cmd, const FcBuffer& srcBuffer, VkDeviceSize bufferSize, bool isTesting)
  {
    // allocate and begin the command buffer to transfer a buffer
    if (isTesting)
    {
      // region of data to copy from and to
      VkBufferCopy bufferCopyRegion{};
      bufferCopyRegion.srcOffset = 0;
      bufferCopyRegion.dstOffset = 0;
      bufferCopyRegion.size = bufferSize;

      // command to copy src buffer to dst buffer
      vkCmdCopyBuffer(cmd, srcBuffer.mBuffer, mBuffer, 1, &bufferCopyRegion);
    }
    else
    {
      FcCommandBuffer& cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

      // region of data to copy from and to
      VkBufferCopy bufferCopyRegion{};
      bufferCopyRegion.srcOffset = 0;
      bufferCopyRegion.dstOffset = 0;
      bufferCopyRegion.size = bufferSize;

      // command to copy src buffer to dst buffer
      vkCmdCopyBuffer(cmdBuffer.getVkCmdBuffer(), srcBuffer.mBuffer, mBuffer, 1, &bufferCopyRegion);

      /* FcLocator::Renderer().submitNonRenderCmdBuffer(cmdBuffer); */
      FcLocator::Renderer().submitCmdBuffer(cmdBuffer);
    }
  }


  //
  void FcBuffer::printBufferStats() const
  {
    std::stringstream msg;
    msg << "--FcBuffer Type: ";

    switch (mBufferType)
    {
        case FcBufferTypes::Staging:
          msg << "STAGING";
          break;
        case FcBufferTypes::Gpu:
          msg << "GPU";
          break;
        case FcBufferTypes::Index:
          msg << "INDEX";
          break;
        case FcBufferTypes::Vertex:
          msg << "VERTEX";
          break;
        case FcBufferTypes::Uniform:
          msg << "UNIFORM";
          break;
        case FcBufferTypes::Custom:
          msg << "CUSTOM";
          break;
        default:
          break;
    }

    msg << "\n--Buffer size: " << mSize;

    fcPrintEndl("%s", msg.str().c_str());
  }


  //
  void FcBuffer::setName(const char* name)
  {
    vmaSetAllocationName(FcLocator::Gpu().getAllocator(), mAllocation, name);
  }


  // TODO note how often this function gets called due to staging buffer and copy to swapchain etc...
  // We should create a semi-permanent buffer specifically for swapchain copies...
  void FcBuffer::deferredDestroy()
  {
    isDestroyed = true;
    // Make sure we don't try and delete a non-allocated buffer
    if (mBuffer == VK_NULL_HANDLE)
      return;

    if(mMemoryAddress != nullptr)
    {
      VmaAllocator allocator = FcLocator::Gpu().getAllocator();
      vmaUnmapMemory(allocator, mAllocation);
      mMemoryAddress = nullptr;
    }

    FcLocator::Janitor().deleteAfterDone(mBuffer, mAllocation, FcLocator::Renderer().getCurrentCommandBuffer());
  }


  //
  void FcBuffer::immediateDestroy()
  {
    // Make sure we don't try and delete a non-allocated buffer
    if (mBuffer == VK_NULL_HANDLE)
      return;

    if(mMemoryAddress != nullptr)
    {
      VmaAllocator allocator = FcLocator::Gpu().getAllocator();
      vmaUnmapMemory(allocator, mAllocation);
      mMemoryAddress = nullptr;
    }

    vmaDestroyBuffer(FcLocator::Gpu().getAllocator(), mBuffer, mAllocation);
  }



  // DELETE after test
  FcBuffer::~FcBuffer()
  {
    if (!isDestroyed)
    {
      fcPrintEndl("Buffer Destructor Called without being destroyed");
      printBufferStats();
    }
  }

} // namespace  fc _END_
