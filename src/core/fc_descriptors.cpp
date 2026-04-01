//>--- fc_descriptors.cpp ---<//
#include "fc_descriptors.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_assert.hpp"
#include "fc_buffer.hpp"
#include "fc_image.hpp"
#include "fc_locator.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <iostream>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  void FcDescriptorBindInfo::addBinding(uint32_t bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages)
  {
    VkDescriptorSetLayoutBinding layoutBinding{};
    // newBinding point in shader (designated by newBinding number specified in shader)
    layoutBinding.binding = bindSlot;
    // type of descriptor (uniform, dynamic uniform, image sampler, etc)
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = shaderStages;

    layoutBindings.emplace_back(std::move(layoutBinding));
  }

  //
  //
  void FcDescriptorBindInfo::enableBindlessTextures()
  {
    // Signal that this descriptor will be using bindless texture indexing
    mIsBindlessIndexingUsed = true;

    VkDescriptorSetLayoutBinding layoutBinding {};

    // Create the layout binding for the textures
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // TODO use index stop thingy
    layoutBinding.descriptorCount = MAX_BINDLESS_RESOURCES;
    layoutBinding.binding = BINDLESS_TEXTURE_BIND_SLOT;

    layoutBindings.push_back(layoutBinding);

    // Create the layout binding for the storage image
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBinding.binding = layoutBinding.binding + 1;

    layoutBindings.push_back(layoutBinding);
  }

  //
  //
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

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = type;
    descriptorWrite.dstBinding = bindSlot;
    descriptorWrite.descriptorCount = 1;
    // Leave these blank for now until it's time to write descriptor set
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;

    //
    descriptorWrites.emplace_back(std::move(descriptorWrite));
  }


  void FcDescriptors::attachBindingOnly(u32 bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages) noexcept
  {
    VkDescriptorSetLayoutBinding layoutBinding{};
    // newBinding point in shader (designated by newBinding number specified in shader)
    layoutBinding.binding = bindSlot;
    // type of descriptor (uniform, dynamic uniform, image sampler, etc)
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = shaderStages;

    mLayoutBindings.emplace_back(std::move(layoutBinding));

    createDescriptorSetLayout();
    // VkWriteDescriptorSet descriptorWrite{};
    // descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // descriptorWrite.dstBinding = bindSlot;
    // descriptorWrite.descriptorCount = 1;
    // // leave blank for now until it's time to write descriptor set
    // descriptorWrite.dstSet = VK_NULL_HANDLE;
    // descriptorWrite.pBufferInfo = &bufferInfo;
    // descriptorWrite.pImageInfo = VK_NULL_HANDLE;

    // //
    // mDescriptorWrites.emplace_back(std::move(descriptorWrite));
  }


  //
  void FcDescriptors::attachUniformBuffer(uint32_t bindSlot, const FcBuffer& buffer, VkDeviceSize size,
                                          VkDeviceSize offset, VkShaderStageFlags shaderStages) noexcept
  {
    attachBuffer(bindSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer, size, offset, shaderStages);
  }


  //
  void FcDescriptors::attachBuffer(u32 bindSlot, VkDescriptorType type ,const FcBuffer& buffer,
                                   VkDeviceSize size, VkDeviceSize offset,
                                   VkShaderStageFlags shaderStages) noexcept
  {
    VkDescriptorSetLayoutBinding layoutBinding{};
    // newBinding point in shader (designated by newBinding number specified in shader)
    layoutBinding.binding = bindSlot;
    // type of descriptor (uniform, dynamic uniform, image sampler, etc)
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = shaderStages;

    mLayoutBindings.emplace_back(std::move(layoutBinding));

    VkDescriptorBufferInfo& bufferInfo =
      mBufferInfos.emplace_back(VkDescriptorBufferInfo {
          .buffer = buffer.getVkBuffer()
        , .offset = offset
        // TODO could have size determined from FcBuffer if there are no situations we use less than WHOLE
        , .range = size
        });

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.dstBinding = bindSlot;
    descriptorWrite.descriptorCount = 1;
    // leave blank for now until it's time to write descriptor set
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = VK_NULL_HANDLE;

    //
    mDescriptorWrites.emplace_back(std::move(descriptorWrite));
  }






  FcDescriptors::~FcDescriptors()
  {
    if (mLayout != VK_NULL_HANDLE)
    {
      // TODO
      /* vkDestroyDescriptorSetLayout(FcLocator::Device(), mLayout, nullptr); */
    }
  }


  //
  //
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


  void FcDescriptorBindInfo::attachImage2(u32 bindSlot, const FcImage& image, VkSampler imageSampler)
  {
    addBinding(bindSlot, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    // TODO think about attaching samplers to FcImages as long as we are not creating them
    VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back(
      VkDescriptorImageInfo {
        .sampler = imageSampler,
	.imageView = image.ImageView(),
	.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      } );

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.dstBinding = bindSlot;
    descriptorWrite.descriptorCount = 1;
    // leave blank for now until it's time to write descriptor set
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
    descriptorWrite.pImageInfo = &imageInfo;
    //
    descriptorWrites.push_back(descriptorWrite);
  }


  //
  void FcDescriptors::attachImage(u32 bindSlot, const FcImage& image,
                                  VkSampler imageSampler, VkShaderStageFlags shaderStages) noexcept
  {
    VkDescriptorSetLayoutBinding layoutBinding{};
    // newBinding point in shader (designated by newBinding number specified in shader)
    layoutBinding.binding = bindSlot;
    // type of descriptor (uniform, dynamic uniform, image sampler, etc)
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = shaderStages;

    mLayoutBindings.emplace_back(std::move(layoutBinding));


    // TODO think about attaching samplers to FcImages as long as we are not creating them
    VkDescriptorImageInfo& imageInfo = mImageInfos.emplace_back(
      VkDescriptorImageInfo {
        .sampler = imageSampler,
	.imageView = image.ImageView(),
	.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      } );

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.dstBinding = bindSlot;
    descriptorWrite.descriptorCount = 1;
    // leave blank for now until it's time to write descriptor set
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
    descriptorWrite.pImageInfo = &imageInfo;
    //
    mDescriptorWrites.push_back(descriptorWrite);
  }


  // TODO return void to eliminate potential layout caching
  VkDescriptorSetLayout FcDescriptors::createDescriptorSetLayout() noexcept
  {
    // create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(mLayoutBindings.size());
    layoutInfo.pBindings = mLayoutBindings.data();

    // Only required when using Bindless Descriptors
    std::vector<VkDescriptorBindingFlags> bindingFlags;
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo {};

    for (VkDescriptorSetLayoutBinding& binding : mLayoutBindings)
    {
      if (binding.descriptorCount == MAX_BINDLESS_RESOURCES)
      { // bindless resources required flags
        FC_ASSERT(isBindlessSupported);

        bindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
                               | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT);
      }
      else
      { // This is a normal descriptor so don't send any special flags
        bindingFlags.push_back(0);
      }
    }

    extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    extendedInfo.bindingCount = static_cast<u32>(mLayoutBindings.size());
    extendedInfo.pBindingFlags = bindingFlags.data();

    //
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.pNext = &extendedInfo;

    VK_ASSERT(vkCreateDescriptorSetLayout(FcLocator::Device(), &layoutInfo, nullptr, &mLayout));

    return mLayout;
  }


  //
  VkDescriptorSet FcDescriptors::createDescriptorSet() noexcept
  {
    // Make sure we delete any previous descriptor set layouts properly
    if (mLayout != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorSetLayout(FcLocator::Device(), mLayout, nullptr);
    }

    // Create the descriptor sets and layouts
    mLayout = createDescriptorSetLayout();
    FcLocator::DescriptorClerk().createDescriptorSet(*this);

    return mDescriptorSet;
  }


//
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
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_RESOURCES},
          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_RESOURCES} }
      };

      // Need to update after bind here, for each binding and in the descriptor set layout
      VkDescriptorPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      // required for descriptor sets that can be updated after they are already bound
      poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
      poolInfo.maxSets = MAX_BINDLESS_RESOURCES * bindlessPoolSizes.size();
      poolInfo.poolSizeCount = static_cast<uint32_t>(bindlessPoolSizes.size());
      poolInfo.pPoolSizes = bindlessPoolSizes.data();

      VK_ASSERT(vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &mBindlessDescriptorPool));
    }
  }

  //
  //
  VkDescriptorSetLayout FcDescriptorClerk::createDescriptorSetLayout(FcDescriptorBindInfo& bindInfo)
  {
    // create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(bindInfo.layoutBindings.size());
    layoutInfo.pBindings = bindInfo.layoutBindings.data();

    // Only neede if using Bindless Descriptors
    std::vector<VkDescriptorBindingFlags> bindingFlags;
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo {};

    if (bindInfo.mIsBindlessIndexingUsed && isBindlessSupported)
    {
      for (VkDescriptorSetLayoutBinding& binding : bindInfo.layoutBindings)
      {
        if (binding.descriptorCount == MAX_BINDLESS_RESOURCES)
        { // bindless resources required flags
          bindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
                                 | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT);
        }
        else
        { // This is a normal descriptor so don't send any special flags
        bindingFlags.push_back(0);
        }
      }

      extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
      extendedInfo.bindingCount = static_cast<u32>(bindInfo.layoutBindings.size());
      extendedInfo.pBindingFlags = bindingFlags.data();

      layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
      layoutInfo.pNext = &extendedInfo;
    }

    VkDescriptorSetLayout descriptorLayout;

    VK_ASSERT(vkCreateDescriptorSetLayout(pDevice, &layoutInfo, nullptr, &descriptorLayout));

    return descriptorLayout;
  }


  void FcDescriptorClerk::createBindlessDescriptorSet(FcDescriptors* descriptors) noexcept
  {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptors->mLayout;

    // TODO de-dupe code below and use this check first with error message
    FC_ASSERT(isBindlessSupported);
    // TODO throw message ... FC_ASSERT_MSG_BOX() using SDL message error box
    /* fcPrintEndl("ERROR: Bindless descriptor set created when bindless rendering is NOT supported!"); */

    uint32_t maxBindingSlot = MAX_BINDLESS_RESOURCES - 1;

    VkDescriptorSetVariableDescriptorCountAllocateInfo descCountInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
    , .descriptorSetCount = 1
    , .pDescriptorCounts = &maxBindingSlot
    };

    allocInfo.descriptorPool = mBindlessDescriptorPool;
    allocInfo.pNext = &descCountInfo;

    VK_ASSERT(vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptors->mDescriptorSet));

    //
    if (descriptors->mDescriptorWrites.size())
    {
      for (VkWriteDescriptorSet& write : descriptors->mDescriptorWrites)
      {
        write.dstSet = descriptors->mDescriptorSet;
      }

      vkUpdateDescriptorSets(pDevice, descriptors->mDescriptorWrites.size()
                             , descriptors->mDescriptorWrites.data(), 0, nullptr);
    }

    // BUG must delete layouts!!
    // TODO return better indicator or throw error if failed
  }


  VkDescriptorSet FcDescriptors::createBindlessDescriptorSet() noexcept
  {
    VkDescriptorSetLayoutBinding layoutBinding {};

    // Create the layout binding for the textures
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // TODO use index stop thingy
    layoutBinding.descriptorCount = MAX_BINDLESS_RESOURCES;
    layoutBinding.binding = BINDLESS_TEXTURE_BIND_SLOT;

    // First add the bindless image slot
    mLayoutBindings.push_back(layoutBinding);

    // Create the layout binding slot for the storage image
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBinding.binding = layoutBinding.binding + 1;

    // Now add the storage image slot @ [image_slot + 1]
    mLayoutBindings.push_back(layoutBinding);

    if (mLayout == VK_NULL_HANDLE)
    {
      fcPrintEndl("Creating NEW DS layout")
      mLayout = createDescriptorSetLayout();
    }
    else
    {
      fcPrintEndl("Using existing DS layout")
    }

    FcLocator::DescriptorClerk().createBindlessDescriptorSet(this);

    return mDescriptorSet;
  }



  //
  void FcDescriptorClerk::createDescriptorSet(FcDescriptors& descriptors) noexcept
  {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptors.mLayout;

    // Get or create a pool to allocat from
    VkDescriptorPool nextPool = getPool();
    allocInfo.descriptorPool = nextPool;

    VkResult result = vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptors.mDescriptorSet);

    // Check if allocation failed and if so, try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
      mFullPools.push_back(nextPool);

      nextPool = getPool();

      VK_ASSERT(vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptors.mDescriptorSet));
    }

    mReadyPools.push_back(nextPool);

    //
    if (descriptors.mDescriptorWrites.size())
    {
      for (VkWriteDescriptorSet& write : descriptors.mDescriptorWrites)
      {
        write.dstSet = descriptors.mDescriptorSet;
      }

      vkUpdateDescriptorSets(pDevice, descriptors.mDescriptorWrites.size()
                             , descriptors.mDescriptorWrites.data(), 0, nullptr);
    }

    // BUG must delete layouts!!
    // TODO return better indicator or throw error if failed
    /* return mDdescriptorSet; */
  }


  //
  // TODO make private
  VkDescriptorSet FcDescriptorClerk::createDescriptorSet(VkDescriptorSetLayout layout,
                                                         FcDescriptorBindInfo& bindingInfo)
  {
    VkDescriptorSet descriptorSet;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (bindingInfo.mIsBindlessIndexingUsed == true && isBindlessSupported)
    {
      VkDescriptorSetVariableDescriptorCountAllocateInfo descCountInfo {};
      descCountInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
      descCountInfo.descriptorSetCount = 1;
      uint32_t maxBindingSlot = MAX_BINDLESS_RESOURCES - 1;
      descCountInfo.pDescriptorCounts = &maxBindingSlot;

      allocInfo.descriptorPool = mBindlessDescriptorPool;
      allocInfo.pNext = &descCountInfo;

      VK_ASSERT(vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet));
    }
    else
    {
      // Get or create a pool to allocat from
      VkDescriptorPool nextPool = getPool();
      allocInfo.descriptorPool = nextPool;

      VkResult result = vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet);

      // Check if allocation failed and if so, try again
      if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
      {
        mFullPools.push_back(nextPool);

        nextPool = getPool();

        VK_ASSERT(vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet));
      }

      mReadyPools.push_back(nextPool);
    }

    //
    if (bindingInfo.descriptorWrites.size())
    {
      for (VkWriteDescriptorSet& write : bindingInfo.descriptorWrites)
      {
        write.dstSet = descriptorSet;
      }

      vkUpdateDescriptorSets(pDevice, bindingInfo.descriptorWrites.size()
                             , bindingInfo.descriptorWrites.data(), 0, nullptr);
    }

    // BUG must delete layouts!!
    // TODO return better indicator or throw error if failed
    return descriptorSet;
  }


  // TODO de-duplicate code from above
  VkDescriptorSet FcDescriptorClerk::createBindlessDescriptorSet(VkDescriptorSetLayout layout,
                                                                 FcDescriptorBindInfo& bindingInfo)
  {
    FcDescriptorBindInfo bindlessBindInfo;
    bindlessBindInfo.enableBindlessTextures();
    VkDescriptorSetLayout bindlessLayout = createDescriptorSetLayout(bindlessBindInfo);

    VkDescriptorSet descriptorSet;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (bindingInfo.mIsBindlessIndexingUsed == true && isBindlessSupported)
    {
      VkDescriptorSetVariableDescriptorCountAllocateInfo descCountInfo {};
      descCountInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
      descCountInfo.descriptorSetCount = 1;
      uint32_t maxBindingSlot = MAX_BINDLESS_RESOURCES - 1;
      descCountInfo.pDescriptorCounts = &maxBindingSlot;

      allocInfo.descriptorPool = mBindlessDescriptorPool;
      allocInfo.pNext = &descCountInfo;

      VK_ASSERT(vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet));
    }


    if (bindingInfo.descriptorWrites.size())
    {
      for (VkWriteDescriptorSet& write : bindingInfo.descriptorWrites)
      {
        write.dstSet = descriptorSet;
      }

      vkUpdateDescriptorSets(pDevice, bindingInfo.descriptorWrites.size()
                             , bindingInfo.descriptorWrites.data(), 0, nullptr);
    }

    // BUG must delete layouts!!
    // TODO return better indicator or throw error if failed
    return descriptorSet;
  }




// VkDescriptorSet FcDescriptorClerk::createSingleImageDescriptor(VkDescriptorSetLayout layout,
//                                                                FcDescriptorBindInfo& bindingInfo)
//   {
//     VkDescriptorSet descriptorSet;

//     VkDescriptorSetAllocateInfo allocInfo{};
//     allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//     allocInfo.descriptorSetCount = 1;
//     allocInfo.pSetLayouts = &layout;

//     if (bindingInfo.mIsBindlessIndexingUsed == true && isBindlessSupported)
//     {
//       VkDescriptorSetVariableDescriptorCountAllocateInfo descCountInfo {};
//       descCountInfo.sType =
//         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
//       descCountInfo.descriptorSetCount = 1;
//       uint32_t maxBindingSlot = MAX_BINDLESS_RESOURCES - 1;
//       descCountInfo.pDescriptorCounts = &maxBindingSlot;

//       allocInfo.descriptorPool = mBindlessDescriptorPool;
//       allocInfo.pNext = &descCountInfo;

//       VK_ASSERT(vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet));
//     }
//     else
//     {
//       // Get or create a pool to allocat from
//       VkDescriptorPool nextPool = getPool();
//       allocInfo.descriptorPool = nextPool;

//       VkResult result = vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet);

//       // Check if allocation failed and if so, try again
//       if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
//       {
//         mFullPools.push_back(nextPool);

//         nextPool = getPool();

//         VK_ASSERT(vkAllocateDescriptorSets(pDevice, &allocInfo, &descriptorSet));
//       }

//       mReadyPools.push_back(nextPool);
//     }

//     //
//     if (bindingInfo.descriptorWrites.size())
//     {
//       for (VkWriteDescriptorSet& write : bindingInfo.descriptorWrites)
//       {
//         write.dstSet = descriptorSet;
//       }

//       vkUpdateDescriptorSets(pDevice, bindingInfo.descriptorWrites.size()
//                              , bindingInfo.descriptorWrites.data(), 0, nullptr);
//     }

//     // TODO return better indicator or throw error if failed
//     return descriptorSet;
//   }



  //
  //
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

   // TODO think about getting rid of the std::span and just have simple declare of which
   // descriptors you want and how many... this method may have advantages I'm not aware
   // of yet however.
  VkDescriptorPool FcDescriptorClerk::createPools(uint32_t maxSets,
                                                  std::span<PoolSizeRatio> poolRatios)
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

  //
  //
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

  //
  //
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

  //
  // BUG destroy bindless resources too!!
  void FcDescriptorClerk::destroy()
  {
    std::cout << "calling: FcDescriptorClerk::destroy" << std::endl;

    // TODO  should also check all this stuff to see if it's VK_NULL_HANDLE
    // FIXME destroying bindless stuff causes segfault
    // if (mBindlessDescriptorLayout != VK_NULL_HANDLE)
    // {
    //   vkDestroyDescriptorSetLayout(pDevice, mBindlessDescriptorLayout, nullptr);
    // }

    // vkDestroyDescriptorPool(pDevice, mBindlessDescriptorPool, nullptr);

    destroyPools();
  }

}// --- namespace fc --- (END)
