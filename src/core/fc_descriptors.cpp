#include "fc_descriptors.hpp"


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_buffer.hpp"
#include "core/fc_image.hpp"
#include "core/fc_locator.hpp"
//#include "fc_mesh.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_pipeline.hpp"
#include "core/utilities.hpp"
#include "vulkan/vulkan_core.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <span>


namespace fc
{
  void FcDescriptorBindInfo::addBinding(uint32_t bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages)
  {
    VkDescriptorSetLayoutBinding layoutBinding{};
    // newBinding point in shader (designated by newBinding number specified in shader)
    layoutBinding.binding = bindSlot;
    // type of descriptor (uniform, dynamic uniform, image sampler, etc)
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = shaderStages;
    // TODO not necessary to set but look into: for textures, can make sampler immutable (unchangeable)
    layoutBinding.pImmutableSamplers = nullptr;

    layoutBindings.emplace_back(std::move(layoutBinding));
  }

  void FcDescriptorBindInfo::attachBuffer(uint32_t bindSlot, VkDescriptorType type
                                          ,const FcBuffer& buffer, VkDeviceSize size, VkDeviceSize offset)
  {


    VkDescriptorBufferInfo& bufferInfo =
        bufferInfos.emplace_back(VkDescriptorBufferInfo{
            .buffer = buffer.getVkBuffer(),
            .offset = offset,
        // TODO could have size determined from FcBuffer
             .range = size
          });

    // VkDescriptorBufferInfo bufferInfo;
    //   bufferInfo.buffer = buffer.getVkBuffer();
    //   bufferInfo.offset = offset;
    //   bufferInfo.range = size;
    //   bufferInfos.emplace_back(std::move(bufferInfo));

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = type;
    descriptorWrite.dstBinding = bindSlot;
    descriptorWrite.descriptorCount = 1;
    // leave blank for now until it's time to write descriptor set
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;

    //
    descriptorWrites.emplace_back(std::move(descriptorWrite));
  }


  void FcDescriptorBindInfo::attachImage(uint32_t bindSlot, VkDescriptorType type
                      ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler)
  {
    VkDescriptorImageInfo& imageInfo =
        imageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = imageSampler,
            .imageView = image.ImageView(),
            .imageLayout = layout } );

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = type;
    descriptorWrite.dstBinding = bindSlot;
    descriptorWrite.descriptorCount = 1;
    // leave blank for now until it's time to write descriptor set
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
    descriptorWrite.pImageInfo = &imageInfo;
    //
    descriptorWrites.push_back(descriptorWrite);
  }


// TODO change ubo_Buffer size to UboSize etc.
  void FcDescriptorClerk::initDescriptorPools(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
  {
    pDevice = FcLocator::Device();

    mPoolRatios.clear();

    for (PoolSizeRatio ratio : poolRatios)
    {
      mPoolRatios.push_back(ratio);
    }

    VkDescriptorPool newPool = createPool(maxSets, mPoolRatios);

     // set this in order to grow it next allocation
    mSetsPerPool = maxSets * 1.5;

    mReadyPools.push_back(newPool);

     // TODO delete... Not sure if any of this is still needed
     // createUniformBuffers(maxSets);
     // createDescriptorSetLayout();
     // createDescriptorSets(maxSets);
     // create the layout for the SceneData uniform buffer
  }


   // TODO must handle empty buffer or image list case
   // TODO use a wrapper of some sort then delete the above method
   // TODO determine if we should return layout or hold or return ptr to pre-build descriptors
  VkDescriptorSetLayout
  FcDescriptorClerk::createDescriptorSetLayout(FcDescriptorBindInfo& bindingInfo
                                               , VkDescriptorSetLayoutCreateFlags flags)
  {

    // create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindingInfo.layoutBindings.size());
    layoutCreateInfo.pBindings = bindingInfo.layoutBindings.data();
    layoutCreateInfo.flags = flags;

    VkDescriptorSetLayout descriptorLayout;

    if (vkCreateDescriptorSetLayout(pDevice, &layoutCreateInfo, nullptr, &descriptorLayout)
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Descriptor Set Layout!");
    }

    return descriptorLayout;
  }


  VkDescriptorSet FcDescriptorClerk::createDescriptorSet(VkDescriptorSetLayout layout
                                                         , FcDescriptorBindInfo& bindingInfo)
  {
    VkDescriptorSet descriptorSet = allocateDescriptorSet(layout, nullptr);

    for (VkWriteDescriptorSet& write : bindingInfo.descriptorWrites)
    {
      write.dstSet = descriptorSet;
    }

    vkUpdateDescriptorSets(pDevice, bindingInfo.descriptorWrites.size()
                           , bindingInfo.descriptorWrites.data(), 0, nullptr);

    return descriptorSet;
  }


VkDescriptorSet FcDescriptorClerk::allocateDescriptorSet(VkDescriptorSetLayout layout, void* pNext)
  {
     // Get or create a pool to allocat from
    VkDescriptorPool nextPool = getPool();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = pNext;
    allocInfo.descriptorPool = nextPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet);

     // Check if allocation failed and if so, try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
      mFullPools.push_back(nextPool);

      nextPool = getPool();
       // ?? shouldn't need the following I think
       // TODO TEST with nullptr then if fails, try removing, then if succeeds, safe to elim
      allocInfo.descriptorPool = nextPool;

      if (vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to allocate Vulkan Descriptor Set");
      }
    }

    mReadyPools.push_back(nextPool);

    return descriptorSet;
  }


   // TODO do this automatically after create
  // void FcDescriptorClerk::cleanupLayoutBindings()
  // {
  //   mLayoutBindings.clear();
  // }

  void FcDescriptorClerk::destroyPools()
  {
    for (VkDescriptorPool pool : mReadyPools)
    {
      vkDestroyDescriptorPool(pDevice, pool, nullptr);
    }

    for (VkDescriptorPool pool : mFullPools)
    {
      vkDestroyDescriptorPool(pDevice, pool, nullptr);
       // make sure we place the now empty pool into ready pools list
    }

    mReadyPools.clear();
    mFullPools.clear();
  }

  void FcDescriptorClerk::clearPools()
  {
    for (VkDescriptorPool pool : mReadyPools)
    {
      vkResetDescriptorPool(pDevice, pool, 0);
    }

    for (VkDescriptorPool pool : mFullPools)
    {
      vkResetDescriptorPool(pDevice, pool, 0);
       // make sure we place the now empty pool into ready pools list
      mReadyPools.push_back(pool);
    }

    mFullPools.clear();
  }


  VkDescriptorPool FcDescriptorClerk::getPool()
  {
    VkDescriptorPool newPool;


    if (mReadyPools.size() != 0)
    {
       // Remove available pool from the ready pool
      newPool = mReadyPools.back();
      mReadyPools.pop_back();
    }
    else // Need to create a new pool
    {
      newPool = createPool(mSetsPerPool, mPoolRatios);

      mSetsPerPool = mSetsPerPool * 1.5;
      if (mSetsPerPool > 4092)
      {
        mSetsPerPool = 4092;
      }
    }

    return newPool;
  }


   // TODO think about getting rid of the std::span and just have simple declare of which descriptors
   // you want and how many... this method may have advantages I'm not aware of yet however.
  VkDescriptorPool FcDescriptorClerk::createPool(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
  {
     // "One very important thing to do with pools is that when you reset a pool, it destroys all of
     // the descriptor sets allocated from it. This is very useful for things like per-frame
     // descriptors. That way we can have descriptors that are used just for one frame, allocated
     // dynamically, and then before we start the frame we completely delete all of them in one
     // go. This is confirmed to be a fast path by GPU vendors, and recommended to use when you need
     // to handle per-frame descriptor sets."



     // Type of decriptors + how many DESCRIPTORS, not descriptor Sets (combined makes the pool size)
    std::vector<VkDescriptorPoolSize> poolSizes{};

    for (PoolSizeRatio ratio : poolRatios)
    {
      poolSizes.push_back(VkDescriptorPoolSize{ ratio.type
                                              , static_cast<uint32_t>(ratio.ratio * maxSets)} );
    }

     // -*-*-*-*-*-*-*-*-*-*-*-   REFERENCE FOR DYNAMIC UBOS   -*-*-*-*-*-*-*-*-*-*-*- //
     // VkDescriptorPoolSize modelPoolSize{};
     // modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
     // modelPoolSize.descriptorCount = static_cast<uint32_t>(mModelDynUniformBuffers.size());
     // modelPoolSize.descriptorCount = static_cast<uint32_t>(uniformBufferCount);
     //std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes = {poolSizes, modelPoolSize};
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &newPool);

    return newPool;

//     if (vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &mDescriptorPool2) != VK_SUCCESS)
//     {
//       throw std::runtime_error("Failed to create a Vulkan Descriptor Pool!");
//     }

  }


  void FcDescriptorClerk::allocateDynamicBufferTransferSpace()
  {
     // collect specifications of our physical deviece to determine how to align(space out) our dynamic UBOs
    VkPhysicalDeviceProperties deviceProperties;


     //vkGetPhysicalDeviceProperties(FcLocator::vkPhysicalDevice(), &deviceProperties);

     // return the memory allignment minimum for our UBOs
    VkDeviceSize uniformBufferOffsetMinimum = deviceProperties.limits.minUniformBufferOffsetAlignment;

     // Calculate the proper alignment of model data
    // mModelUniformAlignment = (sizeof(ModelMatrix) + uniformBufferOffsetMinimum - 1)
    //                          & ~(uniformBufferOffsetMinimum - 1);

     // REFERENCE FOR DYNAMIC UBOs
     // create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
     //pModelTransferSpace = (Model *)aligned_alloc(mModelUniformAlignment, MAX_OBJECTS * mModelUniformAlignment);
  }



  void FcDescriptorClerk::destroy()
  {
    std::cout << "calling: FcDescriptor::destroy" << std::endl;
    // REFERENCE FOR DYNAMIC UBOs
     // release the memory block we have allocated for our model matrix Uniform Buffer Objects
     // free(pModelTransferSpace);

         //vkDestroyDescriptorPool(device, mSamplerDescriptorPool, nullptr);
     //vkDestroyDescriptorSetLayout(device, mSamplerDescriptorSetLayout, nullptr);
     // TODO  should also check all this stuff to see if it's VK_NULL_HANDLE

     //vkDestroySampler(pGpu->VkDevice(), mTextureSampler, nullptr);

     //vkDestroyDescriptorPool(pDevice, mDescriptorPool, nullptr);

     // delete all the buffers (1 per swap chain image)

    // for (auto& buffer : mGlobalUniformBuffers)
    // {
    //    buffer.destroy();
    // }

     // for (auto& buffer : mModelDynUniformBuffers)
     // {
     //   buffer.destroy();
     // }

     //vkDestroyDescriptorSetLayout(pDevice, mUboDescriptorSetLayout, nullptr);

    destroyPools();
     //vkDestroyDescriptorSetLayout(device, mLayout, nullptr);
  }

}// --- namespace fc --- (END)
