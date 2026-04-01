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


  // TODO embedd or DELETE
  struct FcDescriptorBindInfo
    {
       std::vector<VkWriteDescriptorSet> descriptorWrites;
       std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
       std::deque<VkDescriptorBufferInfo> bufferInfos;
       std::deque<VkDescriptorImageInfo> imageInfos;
       bool mIsBindlessIndexingUsed {false};
       void attachBuffer(u32 bindSlot, VkDescriptorType type
                         ,const FcBuffer& buffer, VkDeviceSize size, VkDeviceSize offset);

       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DELETE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
       void attachImage(u32 bindSlot, VkDescriptorType type
                        ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler);

       void attachImage2(u32 bindSlot, const FcImage& image, VkSampler imageSampler);
       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

       // void attachImageBindless(u32 bindSlot, VkDescriptorType type
       //                  ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler);
       void addBinding(u32 bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages);
       void enableBindlessTextures();
    };



  // TODO maybe rename [resourceDocks, resourceMaps, assetKeys, ...]
  //
  class FcDescriptors
  {
   private:
     //
     // TODO add pointer or static instance of descriptor clerk
     // TODO delete all layouts and don't pass them around. then we can safely delete them...
     // if that is not an option, we can create a wrapper class with a destructor
     std::vector<VkWriteDescriptorSet> mDescriptorWrites;
     std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings;
     std::deque<VkDescriptorBufferInfo> mBufferInfos;
     std::deque<VkDescriptorImageInfo> mImageInfos;
     VkDescriptorSet mDescriptorSet {VK_NULL_HANDLE};
     std::vector<VkDescriptorSetLayout> mLayouts;
     VkDescriptorSetLayout mLayout {VK_NULL_HANDLE};

     // DELETE or make static
     bool isBindlessSupported {true};

     /* FcDescriptorBindInfo mBindInfo {}; */
     friend class FcDescriptorClerk;
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
     VkDescriptorSetLayout createDescriptorSetLayout() noexcept;
     VkDescriptorSet createDescriptorSet() noexcept;
     VkDescriptorSet createBindlessDescriptorSet() noexcept;
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
     // ?? Could use as an atlas if wanted/needed
      //std::vector<VkDescriptorSetLayout> mLayouts;
     std::vector<PoolSizeRatio> mPoolRatios;
     std::vector<VkDescriptorPool> mFullPools;
     std::vector<VkDescriptorPool> mReadyPools;
     u32 mSetsPerPool;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   BINDLESS RESOURCES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // TODO default false and test to verify
     bool isBindlessSupported = true;
     // TODO Use mReadyPools instead of separate bindless
     VkDescriptorPool mBindlessDescriptorPool;
     // DELETE mBindlessDescriptorSet and replace with a call to createBindless... for each frame!!
     /* VkDescriptorSet mBindlessDescriptorSet; */
     /* VkDescriptorSetLayout mBindlessDescriptorLayout; */
     VkDescriptorPool getPool();
     void destroyPools();
     VkDescriptorPool createPools(u32 setCount, std::span<PoolSizeRatio> poolRatios);
     void clearPools();

   public:

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcDescriptorClerk() = default;
     FcDescriptorClerk(const FcDescriptorClerk&) = delete;
     FcDescriptorClerk(FcDescriptorClerk&&) = delete;
     FcDescriptorClerk& operator=(const FcBuffer&) = delete;
     FcDescriptorClerk& operator=(FcDescriptorClerk&&) = delete;
     //
     // TODO cleanup to reuse common code and eliminate duplicated methods
     void initDescriptorPools(u32 maxSets, std::span<PoolSizeRatio> poolRatios);
     //
     VkDescriptorSetLayout createDescriptorSetLayout(FcDescriptorBindInfo& bindingInfo);
     //
     VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout layout, FcDescriptorBindInfo& bindInfo);
     // Creates the descriptor set within FcDescriptors
     void createDescriptorSet(FcDescriptors& descriptors) noexcept;
     void createBindlessDescriptorSet(FcDescriptors* descriptors) noexcept;
     VkDescriptorSet createBindlessDescriptorSet(VkDescriptorSetLayout layout, FcDescriptorBindInfo& bindingInfo);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   POOL MANAGEMENT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
  };

}// --- namespace fc --- (END)
