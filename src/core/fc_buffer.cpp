
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_buffer.hpp"
#include "core/fc_locator.hpp"
#include "fc_gpu.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
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

    VkDevice device = FcLocator::Device();

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &mBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Buffer!");
    }

     // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirments;
    vkGetBufferMemoryRequirements(device, mBuffer, &memRequirments);
     // get properties of physical device memory
     // TODO abstract out the memProperties situation and have locator store the (use a map maybe)
     // TODO could also store Physical device in locator if we can't abstract out memproperties
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(FcLocator::Gpu().physicalDevice(), &memProperties);

     // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocInfo{};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memRequirments.size;

     // cycle through all the memory types available and choose the one that has our required properties
    uint32_t memoryTypeIndex = -1;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
      if ((memRequirments.memoryTypeBits & (1 << i)) // first, only pick each allowed type of memory passed in (skip evaluation when that type is not bit-enabled by our allowedType parameter)
          && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) // then make sure that the allowed types also contains the property flags that we request by passing the vkmemorypropertyflags
      {
         // this memory type is an allowed type and it has the properties we want (flags) return
        memoryTypeIndex = i;
        break;
      }
    }

     // Quit if we can't find the proper memory type
     // ?? Tutorial doesn't contain this check so maybe Vulkan is required to return something...
    if (memoryTypeIndex == -1)
    {
      throw std::runtime_error("Failed to find a suitable memory type!");
    }

    memoryAllocInfo.memoryTypeIndex = memoryTypeIndex;

     // allocate memory to VkDeviceMemory
    if (vkAllocateMemory(device, &memoryAllocInfo, nullptr, &mBufferMemory) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to allocate Vulkan Device Memory for FcBuffer!");
    }

     // allocat memory to given vertex buffer
    vkBindBufferMemory(device, mBuffer, mBufferMemory, 0);
  }




  void FcBuffer::storeData(void* sourceData, size_t dataSize)
  {
    void* memDestination;
    vkMapMemory(FcLocator::Device(), mBufferMemory, 0, dataSize, 0, &memDestination);
    memcpy(memDestination, sourceData, dataSize);
    vkUnmapMemory(FcLocator::Device(), mBufferMemory);
  }



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
     // Destroy buffer and deallocate memory
    if (mBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(FcLocator::Device(), mBuffer, nullptr);
      vkFreeMemory(FcLocator::Device(), mBufferMemory, nullptr);
    }

  }

} // namespace  fc _END_
