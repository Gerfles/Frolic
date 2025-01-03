#include "fc_descriptors.hpp"
#include <SDL_log.h>

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_locator.hpp"
#include "fc_gpu.hpp"
//#include "fc_mesh.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <span>

namespace fc
{
   // TODO change ubo_Buffer size to UboSize etc.
  void FcDescriptorClerk::init(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
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


// [[deprecated("Buffers will be moved out of FcDescriptorClerk")]]
//  void FcDescriptorClerk::createUniformBuffers(int uniformBufferCount)
//   {
//      // resize the uniform buffers so we can have one for each swapchain image
//     mGlobalUniformBuffers.resize(uniformBufferCount);
//      //mModelDynUniformBuffers.resize(uniformBufferCount);

//      // create standard UBO buffers
//      // ?? Should the UBO buffer get transfered to DEVICE_LOCAL?
//     for (auto& buffer : mGlobalUniformBuffers)
//     {
//       buffer.allocateBuffer(sizeof(GlobalUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
//     }

//      // REFERENCE FOR DYNAMIC UBOs
//      // first calculate the size we need for our dynamically allocated buffers
//      // VkDeviceSize dynamicUBOsize = mModelUniformAlignment * MAX_OBJECTS;

//      // // create dynamic UBO buffers
//      // for (auto& buffer : mModelDynUniformBuffers)
//      //   {
//      //     buffer.create(gpu, dynamicUBOsize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
//      //                   , VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
//      //   }
//   }



   // TODO this should eventuall replace createDescriptorSetLayout()
  // void FcDescriptorClerk::addBinding(uint32_t bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages)
  // {
  //   VkDescriptorSetLayoutBinding newBinding{};
  //    // newBinding point in shader (designated by newBinding number specified in shader)
  //   newBinding.binding = bindSlot;
  //    // type of descriptor (uniform, dynamic uniform, image sampler, etc)
  //   newBinding.descriptorType = type;
  //   newBinding.descriptorCount = 1;
  //    // shader stage to bind to (which pipeline stage is the descriptor set used/defined)
  //   newBinding.stageFlags = shaderStages;
  //    // for textures: can make sampler immutable (unchangeable) TODO look into techniques
  //   newBinding.pImmutableSamplers = nullptr;

  //   mLayoutBindings.push_back(newBinding);
  // }

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

   // CURRENT USE
  VkDescriptorSet FcDescriptorClerk::createDescriptorSet(std::vector<FcBufferBindingInfo>& bufferBindingInfos
                                                         , std::vector<FcImageBindingInfo>& imageBindingInfos
                                                         , VkDescriptorSetLayoutCreateFlags flags)
  {
    VkDescriptorSetLayout layout = createDescriptorSetLayout(bufferBindingInfos, imageBindingInfos, flags);

    VkDescriptorSet descriptorSet = allocateDescriptorSet(layout, nullptr);

    addDescriptorWrites(bufferBindingInfos, imageBindingInfos, descriptorSet);

    return descriptorSet;
  }

// TODO must handle empty buffer or image list case
// TODO determine if we should return layout or hold or return ptr to pre-build descriptors
  VkDescriptorSetLayout
  FcDescriptorClerk::createDescriptorSetLayout(std::vector<FcBufferBindingInfo>& bufferBindingInfos
                                               , std::vector<FcImageBindingInfo>& imageBindingInfos
                                               , VkDescriptorSetLayoutCreateFlags flags)
  {
     // allocate enough layout bindings for all our images and buffers
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(bufferBindingInfos.size() + imageBindingInfos.size()
                                                             , VkDescriptorSetLayoutBinding{});

     // first setup layout binding for all buffers in the descriptorset
    for (size_t i = 0; i < bufferBindingInfos.size(); i++)
    {
       // newBinding point in shader (designated by newBinding number specified in shader)
      layoutBindings[i].binding = bufferBindingInfos[i].bindingSlotNumber;
       // type of descriptor (uniform, dynamic uniform, image sampler, etc)
      layoutBindings[i].descriptorType = bufferBindingInfos[i].type;
      layoutBindings[i].descriptorCount = 1;
      layoutBindings[i].stageFlags = bufferBindingInfos[i].shaderStages;
       // TODO not necessary to set but look into: for textures, can make sampler immutable (unchangeable)
      layoutBindings[i].pImmutableSamplers = nullptr;
    }

     // next setup layout binding for all images in the descriptorset
    for(size_t i = 0, j = bufferBindingInfos.size();  i < imageBindingInfos.size(); i++, j++)
    {
      layoutBindings[j].binding = imageBindingInfos[i].bindingSlotNumber;
      layoutBindings[j].descriptorType = imageBindingInfos[i].type;
      layoutBindings[j].descriptorCount = 1;
      layoutBindings[j].stageFlags = imageBindingInfos[i].shaderStages;
    }

     // create descriptor set layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutCreateInfo.pBindings = layoutBindings.data();
    layoutCreateInfo.flags = flags;

    VkDescriptorSetLayout descriptorLayout;

    if (vkCreateDescriptorSetLayout(pDevice, &layoutCreateInfo, nullptr, &descriptorLayout)
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Descriptor Set Layout!");
    }

    return descriptorLayout;
  }



  void FcDescriptorClerk::addDescriptorWrites(std::vector<FcBufferBindingInfo>& bufferBindingInfos
                                              , std::vector<FcImageBindingInfo>& imageBindingInfos
                                              , VkDescriptorSet descriptorSet)
  {
    size_t numDescriptors = bufferBindingInfos.size() + imageBindingInfos.size();

     //
    std::vector<VkDescriptorBufferInfo> bufferInfos(bufferBindingInfos.size(), VkDescriptorBufferInfo{});
    std::vector<VkDescriptorImageInfo> imageInfos(imageBindingInfos.size(), VkDescriptorImageInfo{});

     //
    std::vector<VkWriteDescriptorSet> descriptorWrites(numDescriptors, VkWriteDescriptorSet{});

     // first setup descriptor writes for all buffers in the descriptorset
    for (size_t i = 0; i < bufferBindingInfos.size(); i++)
    {
       //
      bufferInfos[i].buffer = bufferBindingInfos[i].buffer;
      bufferInfos[i].offset = bufferBindingInfos[i].bufferOffset;
      bufferInfos[i].range = bufferBindingInfos[i].bufferSize;
       //
      descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[i].descriptorType = bufferBindingInfos[i].type;
      descriptorWrites[i].dstBinding = bufferBindingInfos[i].bindingSlotNumber;
      descriptorWrites[i].descriptorCount = 1;
      descriptorWrites[i].pBufferInfo = &bufferInfos[i];
      descriptorWrites[i].dstSet = descriptorSet;
    }

     // next setup descriptor writes for all images in the descriptorset
    for(size_t i = 0, j =  bufferInfos.size() - 1;  j < numDescriptors; i++, j++)
    {
       //
      imageInfos[i].imageView = imageBindingInfos[i].imageView;
      imageInfos[i].sampler = imageBindingInfos[i].imageSampler;
      imageInfos[i].imageLayout = imageBindingInfos[i].imageLayout;
       //
      descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[j].descriptorType = imageBindingInfos[i].type;
      descriptorWrites[j].dstBinding = imageBindingInfos[i].bindingSlotNumber;
      descriptorWrites[j].descriptorCount = 1;
      descriptorWrites[j].pImageInfo = &imageInfos[i];
      descriptorWrites[j].dstSet = descriptorSet;
    }

    vkUpdateDescriptorSets(pDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
  }



  // void FcDescriptorClerk::clearWrites()
  // {
  //   mImageInfos.clear();
  //   mBufferInfos.clear();
  //   mDescriptorWrites.clear();
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


  // void FcDescriptorClerk::createBufferWrite(int binding, VkBuffer buffer, size_t size
  //                                                   , size_t offset, VkDescriptorType type)
  // {
  //    // We are doing some memory tricks with the use of std::deque here. std::deque is guaranteed to
  //    // keep pointers to elements valid, so we can take advantage of that mechanic when we add new
  //    // VkWriteDescriptorSet into the writes array.
  //   VkDescriptorBufferInfo& info = mBufferInfos.emplace_back(VkDescriptorBufferInfo { .buffer = buffer
  //                                                                                   , .offset = offset
  //                                                                                   , .range = size });
  //   VkWriteDescriptorSet write{};
  //   write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //   write.descriptorCount = 1;
  //   write.dstBinding = binding;
  //    // Leave set empty for now, we will write to it later
  //   write.dstSet = VK_NULL_HANDLE;
  //   write.descriptorType = type;
  //   write.pBufferInfo = &info;

  //   mDescriptorWrites.push_back(write);
  // }



  // void FcDescriptorClerk::createImageWrite(int binding, VkImageView imageView, VkSampler sampler
  //                                        , VkImageLayout layout, VkDescriptorType type)
  // {
  //    // We are doing some memory tricks with the use of std::deque here. std::deque is guaranteed to
  //    // keep pointers to elements valid, so we can take advantage of that mechanic when we add new
  //    // VkWriteDescriptorSet into the writes array.
  //   VkDescriptorImageInfo& info = mImageInfos.emplace_back(VkDescriptorImageInfo { .sampler = sampler
  //                                                                                , .imageView = imageView
  //                                                                                , .imageLayout = layout });
  //   VkWriteDescriptorSet write{};
  //   write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //   write.descriptorCount = 1;
  //   write.dstBinding = binding;
  //    // Leave set empty for now, we will write to it later
  //   write.dstSet = VK_NULL_HANDLE;
  //   write.descriptorType = type;
  //   write.pImageInfo = &info;

  //   mDescriptorWrites.push_back(write);
  // }


  // void FcDescriptorClerk::createDescriptorSets2(FcImage& image)
  // {
  //    // resize descriptor set list so we have one for every buffer
  //    // mUboDescriptorSets.resize(uniformBufferCount);

  //   VkDescriptorSetLayout layout;

  //    // Descriptor set allocation info
  //   VkDescriptorSetAllocateInfo setAllocInfo{};
  //   setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  //   setAllocInfo.descriptorPool = getPool(); // pool to allocate descriptor set from
  //   setAllocInfo.descriptorSetCount = 1; // number of sets to allocate
  //   setAllocInfo.pSetLayouts = &layout; // layouts to use to allocate sets (1:1 ratio)

  //    // Allocate descriptor sets (multiple)
  //   VkDescriptorSet descriptorSet;
  //   if (vkAllocateDescriptorSets(pDevice, &setAllocInfo, &descriptorSet) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Failed to allocate Vulkan Descriptor Sets!");
  //   }

  //    // update all descriptor set buffer bindings (link descriptor sets with buffers that contain the actual data)

  //      // Image info and data offset info
  //   VkDescriptorImageInfo imageInfo = {};
  //   imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  //   imageInfo.imageView = image.ImageView();
  //    //imageInfo.sampler = image.TextureSampler();

  //    // data about connection between binding and image we want to update
  //   VkWriteDescriptorSet imageWriteDescriptors = {};
  //   imageWriteDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //   imageWriteDescriptors.dstSet = descriptorSet;
  //   imageWriteDescriptors.dstBinding = 0;
  //   imageWriteDescriptors.dstArrayElement = 0;
  //   imageWriteDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  //   imageWriteDescriptors.descriptorCount = 1;
  //   imageWriteDescriptors.pImageInfo = &imageInfo;

  //    // update the descriptor sets with new buffer/binding infor
  //    // TODO probably better to update all the descriptor sets at once
  //   vkUpdateDescriptorSets(pDevice, 1, &imageWriteDescriptors, 0, nullptr);
  // }



  // void FcDescriptorClerk::createDescriptorSetLayout()
  // {
  //    // view projection binding info
  //   VkDescriptorSetLayoutBinding globalUboLayoutBinding{};
  //    // binding point in shader (designated by binding number specified in shader)
  //   globalUboLayoutBinding.binding = 0;
  //    // type of descriptor (uniform, dynamic uniform, image sampler, etc)
  //   globalUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  //   globalUboLayoutBinding.descriptorCount = 1;
  //    // shader stage to bind to (which pipeline stage is the descriptor set used/defined)
  //   globalUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  //    // for textures: can make sampler immutable (unchangeable)
  //   globalUboLayoutBinding.pImmutableSamplers = nullptr;

  //    // "One very important thing to do with pools is that when you reset a pool, it destroys all of
  //    // the descriptor sets allocated from it. This is very useful for things like per-frame
  //    // descriptors. That way we can have descriptors that are used just for one frame, allocated
  //    // dynamically, and then before we start the frame we completely delete all of them in one
  //    // go. This is confirmed to be a fast path by GPU vendors, and recommended to use when you need
  //    // to handle per-frame descriptor sets."
  //    // DYNAMIC UBOs would set the following
  //    // modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  //    // modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  //   std::array<VkDescriptorSetLayoutBinding, 1> layoutBindings = {globalUboLayoutBinding};

  //    // create descriptor set layout with given bindings
  //   VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
  //   layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  //   layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  //   layoutCreateInfo.pBindings = layoutBindings.data();

  //   if (vkCreateDescriptorSetLayout(pDevice, &layoutCreateInfo, nullptr, &mUboDescriptorSetLayout)
  //       != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Failed to create a Vulkan Descriptor Set Layout!");
  //   }

  //    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT

  //    // texture binding info
  //   VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  //   samplerLayoutBinding.binding = 0;
  //   samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  //   samplerLayoutBinding.descriptorCount = 1;
  //   samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  //   samplerLayoutBinding.pImmutableSamplers = nullptr; //TRY above

  //    // Create a descriptor set layout with given bindings for texture
  //   VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
  //   textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  //   textureLayoutInfo.bindingCount = 1;
  //   textureLayoutInfo.pBindings = &samplerLayoutBinding;

  //    // create sampler descriptor set layout
  //   if (vkCreateDescriptorSetLayout(pDevice, &textureLayoutInfo, nullptr, &mSamplerDescriptorSetLayout) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Failed to create a Vulkan Descriptor Set Layout!");
  //   }



  // }


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


  // VkDescriptorPool FcDescriptorClerk::createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
  // {
  //   std::vector<VkDescriptorPoolSize> poolSizes;


  //   for (PoolSizeRatio ratio : poolRatios)
  //   {
  //     poolSizes.push_back(VkDescriptorPoolSize{ .type = ratio.type
  //                                             , .descriptorCount = static_cast<uint32_t>(ratio.ratio * setCount) });
  //   }

  //   VkDescriptorPoolCreateInfo poolInfo{};
  //   poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  //   poolInfo.flags = 0;
  //   poolInfo.maxSets = setCount;
  //   poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  //   poolInfo.pPoolSizes = poolSizes.data();

  //   VkDescriptorPool newPool;
  //   vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool);

  //   return newPool;
  // }


  // [[deprecated("Remove this function and refactor")]]
  // void FcDescriptorClerk::createDescriptorPool(int uniformBufferCount)
  // {
  //    // Type of decriptors + how many DESCRIPTORS, not descriptor Sets (combined makes the pool size)
  //   VkDescriptorPoolSize viewProjPoolSize{};
  //   viewProjPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  //   viewProjPoolSize.descriptorCount = static_cast<uint32_t>(mGlobalUniformBuffers.size());
  //    // viewProjPoolSize.descriptorCount = static_cast<uint32_t>(uniformBufferCount);



  //   std::array<VkDescriptorPoolSize, 1> descriptorPoolSizes = {viewProjPoolSize}; //, modelPoolSize};

  //   VkDescriptorPoolCreateInfo poolInfo{};
  //   poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  //   poolInfo.maxSets = static_cast<uint32_t>(uniformBufferCount);
  //   poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
  //   poolInfo.pPoolSizes = descriptorPoolSizes.data();

  //    //
  //   if (vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Failed to create a Vulkan Descriptor Pool!");
  //   }

  //    // CREATE SAMPLER DESCRIPTOR POOL
  //   VkDescriptorPoolSize samplerPoolSize{};
  //    // TODO vulkan programmers say that the optimal way to do this is to create separate image and sampler descriptors
  //   samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  //    // TODO not optimal here should handle descriptor count more dynamically
  //   samplerPoolSize.descriptorCount = MAX_OBJECTS; // assume each object will only have one texture

  //   VkDescriptorPoolCreateInfo samplerPoolInfo{};
  //    // TODO this is also not the best way here -- better to use array layers and texture atlases so that we only need one or a few descriptor sets and not one for each object
  //   samplerPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  //   samplerPoolInfo.maxSets = MAX_OBJECTS;
  //   samplerPoolInfo.poolSizeCount = 1;
  //   samplerPoolInfo.pPoolSizes = &samplerPoolSize;


  //   if (vkCreateDescriptorPool(pDevice, &samplerPoolInfo, nullptr, &mSamplerDescriptorPool) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Failed to create a Vulkan Descriptor Pool!");
  //   }
  // }




  // void FcDescriptorClerk::createDescriptorSets(int uniformBufferCount)
  // {
  //    // resize descriptor set list so we have one for every buffer
  //   mUboDescriptorSets.resize(uniformBufferCount);

  //   std::vector<VkDescriptorSetLayout> setLayouts(uniformBufferCount, mUboDescriptorSetLayout);

  //    // Descriptor set allocation info
  //   VkDescriptorSetAllocateInfo setAllocInfo{};
  //   setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  //   setAllocInfo.descriptorPool = mDescriptorPool; // pool to allocate descriptor set from
  //   setAllocInfo.descriptorSetCount = static_cast<uint32_t>(uniformBufferCount); // number of sets to allocate
  //   setAllocInfo.pSetLayouts = setLayouts.data(); // layouts to use to allocate sets (1:1 ratio)

  //    // Allocate descriptor sets (multiple)
  //   if (vkAllocateDescriptorSets(pDevice, &setAllocInfo, mUboDescriptorSets.data()) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Failed to allocate Vulkan Descriptor Sets!");
  //   }

  //    // update all of descriptor set buffer bindings (link our descriptor sets with our buffers that contain the actual data)
  //   for (int i = 0; i < mUboDescriptorSets.size(); ++i)
  //   {
  //      // buffer info and data offset info
  //     VkDescriptorBufferInfo globalUniformBufferInfo{};
  //     globalUniformBufferInfo.buffer = mGlobalUniformBuffers[i].getVkBuffer(); // buffer to get data from
  //     globalUniformBufferInfo.offset = 0; // position of start of data
  //     globalUniformBufferInfo.range = sizeof(GlobalUbo); // size of data

  //      // data about connection between binding and buffer
  //     VkWriteDescriptorSet globalUboSetWrite{};
  //     globalUboSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //     globalUboSetWrite.dstSet = mUboDescriptorSets[i]; // descriptor set to update
  //     globalUboSetWrite.dstBinding = 0; // binding to update (matches with binding in layout (shader))
  //     globalUboSetWrite.dstArrayElement = 0; // index in array to update
  //     globalUboSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // type of descriptor
  //     globalUboSetWrite.descriptorCount = 1; // amount to update
  //     globalUboSetWrite.pBufferInfo = &globalUniformBufferInfo;

  //      // MODEL DESCRIPTOR
  //      // Model buffer binding info
  //      // VkDescriptorBufferInfo modelBufferInfo{};
  //      // modelBufferInfo.buffer = mModelDynUniformBuffers[i].Vkbuffer();
  //      // modelBufferInfo.offset = 0;
  //      // modelBufferInfo.range = mModelUniformAlignment;

  //      // VkWriteDescriptorSet modelSetWrite{};
  //      // modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //      // modelSetWrite.dstSet = mDescriptorSets[i];
  //      // modelSetWrite.dstBinding = 1;
  //      // modelSetWrite.dstArrayElement = 0;
  //      // modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  //      // modelSetWrite.descriptorCount = 1;
  //      // modelSetWrite.pBufferInfo = &modelBufferInfo;

  //      // list of descriptor set writes
  //     std::array<VkWriteDescriptorSet, 1> setWrites = {globalUboSetWrite}; // , modelSetWrite};
  //      // update the descriptor sets with new buffer/binding infor
  //      //TODO probably better to update all the descriptor sets at once
  //     vkUpdateDescriptorSets(pDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
  //   }

  // }

   // define push constant - no create needed...
   // could put this in pipeline...
  // void FcDescriptor::setUpPipelineLayout(VkPipelineLayoutCreateInfo& pipelineLayoutInfo)
  // {
    // mPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // shader stage push constant will go to
    // mPushConstantRange.offset = 0;                              // offset into given data to pass to push constant
    // mPushConstantRange.size = sizeof(ModelMatrix);                    // size of data being passed

    //  //std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {mDescriptorSetLayout, mSamplerSetLayout};
    // mDescriptorSetLayouts[0] = mDescriptorSetLayout;
    // mDescriptorSetLayouts[1] = mSamplerSetLayout;

    // pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(mDescriptorSetLayouts.size());
    // pipelineLayoutInfo.pSetLayouts = mDescriptorSetLayouts.data();
    // pipelineLayoutInfo.pushConstantRangeCount = 1;
    // pipelineLayoutInfo.pPushConstantRanges = &mPushConstantRange;
//}





  void FcDescriptorClerk::allocateDynamicBufferTransferSpace()
  {
     // collect specifications of our physical deviece to determine how to align(space out) our dynamic UBOs
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(FcLocator::vkPhysicalDevice(), &deviceProperties);

     // return the memory allignment minimum for our UBOs
    VkDeviceSize uniformBufferOffsetMinimum = deviceProperties.limits.minUniformBufferOffsetAlignment;

     // Calculate the proper alignment of model data
    // mModelUniformAlignment = (sizeof(ModelMatrix) + uniformBufferOffsetMinimum - 1)
    //                          & ~(uniformBufferOffsetMinimum - 1);

     // REFERENCE FOR DYNAMIC UBOs
     // create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
     //pModelTransferSpace = (Model *)aligned_alloc(mModelUniformAlignment, MAX_OBJECTS * mModelUniformAlignment);
  }



  // uint32_t FcDescriptorClerk::createTextureDescriptor(VkImageView textureImageView, VkSampler& textureSampler)
  // {
  //   VkDescriptorSet descriptorSet;

  //    // Descriptor set allocation info
  //   VkDescriptorSetAllocateInfo setAllocInfo{};
  //   setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  //   setAllocInfo.descriptorPool = mSamplerDescriptorPool;
  //    // TODO better to create all descriptor sets at the same time--obviously after the textures have all been created
  //   setAllocInfo.descriptorSetCount = 1;
  //   setAllocInfo.pSetLayouts = &mSamplerDescriptorSetLayout;

  //    // Allocate descriptor sets
  //   if (vkAllocateDescriptorSets(pDevice, &setAllocInfo, &descriptorSet) != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Failed to allocate Vulkan Descriptor Set for a texture!");
  //   }

  //    // Texture image info
  //   VkDescriptorImageInfo imageInfo{};
  //   imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // image layout when in use
  //   imageInfo.imageView = textureImageView;
  //    // must delete after use
  //   imageInfo.sampler = textureSampler;

  //    // descriptor write info
  //   VkWriteDescriptorSet descriptorWrite{};
  //   descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  //   descriptorWrite.dstSet = descriptorSet;
  //   descriptorWrite.dstBinding = 0;
  //   descriptorWrite.dstArrayElement = 0;
  //   descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  //   descriptorWrite.descriptorCount = 1;
  //   descriptorWrite.pImageInfo = &imageInfo;

  //    // update the new descriptor set
  //   vkUpdateDescriptorSets(pDevice, 1, &descriptorWrite, 0, nullptr);

  //    // add descriptor set to list
  //   mSamplerDescriptorSets.push_back(descriptorSet);

  //   return mSamplerDescriptorSets.size() - 1;
  // }








  //[[deprecated("No longer need to update within descriptorClerk")]]
  // void FcDescriptorClerk::update(uint32_t imgIndex, void* data)
  // {
  //    // TODO notice that this function updates the static UBO AND dynamic UBO... this would be better done
  //    // with separate update functions since we don't always need to update them at the same time
  //    // copy view projection data
  //    // TODO think about storing the sizeof results as a global number so the following is no longer a function call
  //   mGlobalUniformBuffers[imgIndex].overwriteData(data, sizeof(GlobalUbo));

  //    // SAVED FOR REFERENCE TO DYNAMIC UNIFORM BUFFERS
  //    // copy Model data
  //    // for (size_t i = 0; i < meshList.size(); i++)
  //    // {
  //    //    // get the address for the memory to write each model within the reserved memory chunk in modelTransferSpace
  //    //   Model* thisModel = (Model* )((uint64_t )pModelTransferSpace + (i * mModelUniformAlignment));
  //    //    // now write to the appropriate location in memory
  //    //   *thisModel = meshList[i].getModel();
  //    // }

  //    //  // now that we have written the data properly spaced, copy it to our buffer
  //    // mModelDynUniformBuffers[imgIndex].storeData(pModelTransferSpace, mModelUniformAlignment * meshList.size());
  // }



  void FcDescriptorClerk::destroy()
  {
    std::cout << "calling: FcDescriptor::destroy" << std::endl;
    // REFERENCE FOR DYNAMIC UBOs
     // release the memory block we have allocated for our model matrix Uniform Buffer Objects
     // free(pModelTransferSpace);

     // TEXTURES
    VkDevice device = pDevice;

    vkDestroyDescriptorPool(device, mSamplerDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, mSamplerDescriptorSetLayout, nullptr);
     // TODO  should also check all this stuff to see if it's VK_NULL_HANDLE

     //vkDestroySampler(pGpu->VkDevice(), mTextureSampler, nullptr);

    vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);

     // delete all the buffers (1 per swap chain image)

    for (auto& buffer : mGlobalUniformBuffers)
    {
       buffer.destroy();
    }

     // for (auto& buffer : mModelDynUniformBuffers)
     // {
     //   buffer.destroy();
     // }

    vkDestroyDescriptorSetLayout(device, mUboDescriptorSetLayout, nullptr);

    destroyPools();
     //vkDestroyDescriptorSetLayout(device, mLayout, nullptr);
  }

}// --- namespace fc --- (END)
