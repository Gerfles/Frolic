#include "fc_descriptors.hpp"

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_buffer.hpp"
#include "core/fc_image.hpp"
#include "core/fc_locator.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL / UTIL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
#include <cstdint>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <span>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

//
//
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


  // void FcDescriptorBindInfo::attachInput(uint32_t bindSlot, VkDescriptorType type
  //                     ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler)
  // {
  //   VkDescriptorImageInfo& imageInfo =
  //       imageInfos.emplace_back(VkDescriptorImageInfo{
  //           .sampler = imageSampler,
  //           .imageView = image.ImageView(),
  //           .imageLayout = layout } );


  //   VkWriteDescriptorSet descriptorWrite{};
  //   descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //   descriptorWrite.descriptorType = type;
  //   descriptorWrite.dstBinding = bindSlot;
  //   descriptorWrite.descriptorCount = 1;


  //   // leave blank for now until it's time to write descriptor set
  //   descriptorWrite.dstSet = VK_NULL_HANDLE;
  //   descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
  //   descriptorWrite.pImageInfo = &imageInfo;
  //   //
  //   descriptorWrites.push_back(descriptorWrite);
  // }

  void FcDescriptorBindInfo::attachImageBindless(uint32_t bindSlot, VkDescriptorType type
                      ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler)
  {
    VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back(
      VkDescriptorImageInfo {
        .sampler = imageSampler,
	.imageView = image.ImageView(),
	.imageLayout = layout
      } );

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



  void FcDescriptorBindInfo::attachImage(uint32_t bindSlot, VkDescriptorType type
                      ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler)
  {
    VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back(
      VkDescriptorImageInfo {
        .sampler = imageSampler,
	.imageView = image.ImageView(),
	.imageLayout = layout
      } );

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

    // First create the main pool that will be used for everything except the bindless resources
    mPoolRatios.clear();

    for (PoolSizeRatio ratio : poolRatios)
    {
      mPoolRatios.push_back(ratio);
    }

    VkDescriptorPool newPool = createPools(maxSets, mPoolRatios);

    // set this in order to grow it next allocation
    mSetsPerPool = maxSets * 1.5;

    mReadyPools.push_back(newPool);

    // Next create the pool that will only be used for bindless resources
    if (isBindlessSupported)
    {
      std::array<VkDescriptorPoolSize, 2> bindlessPoolSizes { {
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_TEXTURES},
          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_TEXTURES} }
      };

      // Need to update after bind here, for each binding and in the descriptor set layout
      VkDescriptorPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      // required for descriptor sets that can be updated after they are already bound
      poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
      poolInfo.maxSets = MAX_BINDLESS_TEXTURES * bindlessPoolSizes.size();
      poolInfo.poolSizeCount = static_cast<uint32_t>(bindlessPoolSizes.size());
      poolInfo.pPoolSizes = bindlessPoolSizes.data();

      if (vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &mBindlessDescriptorPool) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create bindless descriptor pool!");
      }
    }
  }



  void FcDescriptorClerk::createBindlessDescriptorLayout()
  {
    // First create the pool that will only be used for bindless resources
    if (isBindlessSupported)
    {
      std::array<VkDescriptorPoolSize, 2> bindlessPoolSizes { {
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_TEXTURES},
          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_TEXTURES} }
      };

      std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings;

      // Create the layout binding for the textures
      layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      // TODO may want to use a more fine grained approach
      layoutBindings[0].stageFlags = VK_SHADER_STAGE_ALL;
      layoutBindings[0].descriptorCount = MAX_BINDLESS_TEXTURES;
      layoutBindings[0].binding = BINDLESS_TEXTURE_BIND_SLOT;
      layoutBindings[0].pImmutableSamplers = nullptr;

      // Create the layout binding for the storage image
      layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      layoutBindings[1].stageFlags = layoutBindings[0].stageFlags;
      layoutBindings[1].descriptorCount = MAX_BINDLESS_TEXTURES;
      layoutBindings[1].binding = layoutBindings[0].binding + 1;
      layoutBindings[1].pImmutableSamplers = nullptr;

      // bindless resources required flags
      VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                               //VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                                               VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

      std::array<VkDescriptorBindingFlags, 2> bindingFlags = {{ bindlessFlags, bindlessFlags }};

      VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo;
      extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
      extendedInfo.bindingCount = bindlessPoolSizes.size();
      extendedInfo.pBindingFlags = bindingFlags.data();

      // create descriptor set layout with given bindings
      VkDescriptorSetLayoutCreateInfo layoutInfo{};
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = bindlessPoolSizes.size();
      layoutInfo.pBindings = layoutBindings.data();
      layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
      layoutInfo.pNext = &extendedInfo;

      if (vkCreateDescriptorSetLayout(pDevice, &layoutInfo, nullptr, &mBindlessDescriptorLayout)
          != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a Vulkan descriptor set layout!");
      }

      // -*-*-   CREATE DESCRIPTOR SET THAT WILL BE USED FOR REST OF APPLICATION   -*-*- //
      VkDescriptorSetVariableDescriptorCountAllocateInfo descCountInfo;
      descCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
      descCountInfo.descriptorSetCount = 1;
      uint32_t maxBindingSlot = MAX_BINDLESS_TEXTURES - 1;
      descCountInfo.pDescriptorCounts = &maxBindingSlot;

      VkDescriptorSetAllocateInfo descAllocInfo;
      descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      descAllocInfo.descriptorSetCount = 1;
      descAllocInfo.descriptorPool = mBindlessDescriptorPool;
      descAllocInfo.pSetLayouts = &mBindlessDescriptorLayout;
      /* descAllocInfo.pNext = &descCountInfo; */

      if (vkAllocateDescriptorSets(pDevice, &descAllocInfo, &mBindlessDescriptorSet) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create Vulkan descriptor set!");
      }
    }
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
    // | VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    // bindless resources required flags
    // VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
    //                                          VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    // // ?? why 4 and not 2
    // VkDescriptorBindingFlags bindingFlags[2];
    // bindingFlags[0] = bindlessFlags;
    // bindingFlags[1] = bindlessFlags;

    // VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo;
    // extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    // extendedInfo.bindingCount = layoutCreateInfo.bindingCount;
    // extendedInfo.pBindingFlags = bindingFlags;

    // layoutCreateInfo.pNext = &extendedInfo;

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

      if (vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to allocate Vulkan Descriptor Set");
      }
    }

    mReadyPools.push_back(nextPool);

    return descriptorSet;
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
      newPool = createPools(mSetsPerPool, mPoolRatios);

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
  VkDescriptorPool FcDescriptorClerk::createPools(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
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

    // static const uint32_t maxPoolSets = 128;
    // VkDescriptorPoolSize poolSizes[] =
    //   {
    //     {VK_DESCRIPTOR_TYPE_SAMPLER, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, maxPoolSets},
    //     {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, maxPoolSets}
    //   };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    // This would allow us to free individual descriptors but requires overhead
    // poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &newPool);

    return newPool;
  }



   // TODO do this automatically after create
  // void FcDescriptorClerk::cleanupLayoutBindings()
  // {
  //   mLayoutBindings.clear();
  // }


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


  // BUG destroy bindless resources too!!
  void FcDescriptorClerk::destroy()
  {
    std::cout << "calling: FcDescriptorClerk::destroy" << std::endl;
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
