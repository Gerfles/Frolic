
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_buffer.hpp"
#include <SDL_log.h>
#include "core/fc_locator.hpp"
#include "core/log.hpp"
#include "fc_gpu.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include "vk_mem_alloc.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstring>


namespace  fc
{

  void FcBuffer::create(VkDeviceSize bufferSize, VkBufferUsageFlags useFlags, VkMemoryPropertyFlags properties)
  {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = useFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
     // TODO performace test different mem type flags
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
     //allocInfo.requiredFlags = properties;
     // TODO chang this to use a simlple "STAGING bool"
    if ((properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
          allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }


     // much slower frame rate if allow access random bit
     //allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

     // ?? does this instantiate a copied instance of FcGpu
    FcGpu gpu = FcLocator::Gpu();

     // TODO Note: This function creates a new VkBuffer. Sub-allocation of parts of one large
     // buffer, although recommended as a good practice, is out of scope of this library and could
     // be implemented by the user as a higher-level logic on top of VMA.
    if (vmaCreateBuffer(gpu.getAllocator(), &bufferInfo, &allocInfo, &mBuffer, &mAllocation, nullptr)
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Buffer!");
    }

     // VkDevice device = FcLocator::Device();
     // if (vkCreateBuffer(device, &bufferInfo, nullptr, &mBuffer) != VK_SUCCESS)
     // {
     //   throw std::runtime_error("Failed to create a Vulkan Buffer!");
     // }

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
//     if (vkAllocateMemory(device, &memoryAllocInfo, nullptr, &mBufferMemory) != VK_SUCCESS)
//     {
//       throw std::runtime_error("Failed to allocate Vulkan Device Memory for FcBuffer!");
//     }

//      // allocate memory to given vertex buffer
//     vkBindBufferMemory(device, mBuffer, mBufferMemory, 0);
  }


  void FcBuffer::storeData(void* sourceData, size_t dataSize)
  {

    void* memAddress;
    VmaAllocator allocator = FcLocator::Gpu().getAllocator();

    if (vmaMapMemory(allocator, mAllocation, &memAddress) != VK_SUCCESS)
    {
            throw std::runtime_error("Failed to map VMA buffer memory!");
    }

    memcpy(memAddress, sourceData, dataSize);
    vmaUnmapMemory(FcLocator::Gpu().getAllocator(), mAllocation);

     // void* memDestination;
     // vkMapMemory(FcLocator::Device(), mBufferMemory, 0, dataSize, 0, &memDestination);
     // memcpy(memDestination, sourceData, dataSize);
     // vkUnmapMemory(FcLocator::Device(), mBufferMemory);

  } // --- FcBuffer::storeData (_) --- (END)




  void FcBuffer::copyBuffer(const FcBuffer& srcBuffer, VkDeviceSize bufferSize)
  {
     // allocate and begin the command buffer to transfer a buffer
    VkCommandBuffer transferCommandBuffer = FcLocator::Gpu().beginCommandBuffer();

     // region of data to copy from and to
    VkBufferCopy bufferCopyRegion{};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = bufferSize;

     // command to copy src buffer to dst buffer
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer.mBuffer, mBuffer, 1, &bufferCopyRegion);

    FcLocator::Gpu().submitCommandBuffer(transferCommandBuffer);
  }


  void FcBuffer::destroy()
  {
    //  // Destroy the VMA buffer allocation first
    // vmaDestroyBuffer(FcLocator::Gpu().getAllocator(), mBuffer, mAllocation);

     // Destroy buffer and deallocate memory
    if (mBuffer != VK_NULL_HANDLE)
    {
       // Destroy the VMA buffer allocation first
      vmaDestroyBuffer(FcLocator::Gpu().getAllocator(), mBuffer, mAllocation);

      // vkDestroyBuffer(FcLocator::Device(), mBuffer, nullptr);
      // vkFreeMemory(FcLocator::Device(), mBufferMemory, nullptr);
    }

  }

} // namespace  fc _END_
