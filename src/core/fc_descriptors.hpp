//>--- fc_descriptors.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <deque>
#include <span>
#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcImage; class FcBuffer; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // TODO maybe rename [resourceDocks, resourceMaps, assetKeys, ...]
  // TODO could combine with descriptorManager
  class FcDescriptors
  {
   private:
     //
     // TODO add pointer or static instance of descriptor clerk
     // TODO delete all layouts and don't pass them around. then we can safely delete them...
     // if that is not an option, we can create a wrapper class with a destructor
     std::vector<VkWriteDescriptorSet> mDescriptorWrites;

     std::deque<VkDescriptorBufferInfo> mBufferInfos;
     std::deque<VkDescriptorImageInfo> mImageInfos;
     VkDescriptorSet mDescriptorSet {VK_NULL_HANDLE};
     VkDescriptorSetLayout mLayout {VK_NULL_HANDLE};
     std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings;
     // DELETE or make static
     bool isBindlessSupported {true};
     //
     friend class FcDescriptorClerk;
     //
   public:



     ~FcDescriptors();
     //
     void attachImage(u32 bindSlot, const FcImage& image,VkSampler imageSampler,
                      VkShaderStageFlags shaderStages) noexcept;
     void attachUniformBuffer(uint32_t bindSlot, const FcBuffer& buffer, VkDeviceSize size,
                              VkDeviceSize offset, VkShaderStageFlags shaderStages) noexcept;
     void attachBuffer(u32 bindSlot, VkDescriptorType type ,const FcBuffer& buffer,
                       VkDeviceSize size, VkDeviceSize offset, VkShaderStageFlags shaderStages) noexcept;
     void attachBindingOnly(u32 bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages) noexcept;
     //
     void createDescriptorSetLayout() noexcept;
     VkDescriptorSet createDescriptorSet() noexcept;

     // TODO might need to delete
     VkDescriptorSet createBindlessDescriptorSet() noexcept;
     // and use instead:
     void attachBindlessDescriptors() noexcept;



     inline VkDescriptorSet VkDescriptorSet() noexcept { return mDescriptorSet; }
     inline VkDescriptorSetLayout VkDescriptorSetLayout() noexcept { return mLayout; }
  };


  // TODO make this struct a public struct defined within FcDescriptorClerk so it would be
  // declared as FcDescriptorClerk::PoolSizeRatio
  struct PoolSizeRatio
  {
     VkDescriptorType type;
     float ratio;
  };


  class FcDescriptorClerk
  {
   private:
     VkDevice pDevice;
     std::vector<PoolSizeRatio> mPoolRatios;
     std::vector<VkDescriptorPool> mFullPools;
     std::vector<VkDescriptorPool> mReadyPools;
     u32 mSetsPerPool;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   BINDLESS RESOURCES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // TODO default false and test to verify
     bool isBindlessSupported = true;
     // TODO Use mReadyPools instead of separate bindless
     VkDescriptorPool mBindlessDescriptorPool;
     void destroyPools();
     VkDescriptorPool createPools(u32 setCount, std::span<PoolSizeRatio> poolRatios);
     void clearPools();
     //
   public:
     //
     VkDescriptorPool getPool();
     inline VkDescriptorPool getBindlessPool() { return mBindlessDescriptorPool; }
     // TRY to delete
     inline void markPoolAsFull(VkDescriptorPool pool) { mFullPools.push_back(pool); }
     inline void markPoolAsReady(VkDescriptorPool pool) { mReadyPools.push_back(pool); }
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcDescriptorClerk() = default;
     FcDescriptorClerk(const FcDescriptorClerk&) = delete;
     FcDescriptorClerk(FcDescriptorClerk&&) = delete;
     FcDescriptorClerk& operator=(const FcBuffer&) = delete;
     FcDescriptorClerk& operator=(FcDescriptorClerk&&) = delete;
     // TODO cleanup to reuse common code and eliminate duplicated methods
     void initDescriptorPools(u32 maxSets, std::span<PoolSizeRatio> poolRatios);
     /* void createDescriptorSet(FcDescriptors& descriptors) noexcept; */
     void createBindlessDescriptorSet(FcDescriptors* descriptors) noexcept;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   POOL MANAGEMENT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
  };

}// --- namespace fc --- (END)
