//>--- fc_descriptors.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
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


   // struct PointLight
   // {
   //    glm::vec4 position{}; // ignore w
   //    glm::vec4 color{}; // w is intensity
   // };

   // struct GlobalUbo
   // {
   //    alignas(16) glm::mat4 projection {1.f};
   //    alignas(16) glm::mat4 view {1.f};
   //    alignas(16) glm::mat4 invView {1.f};
   //    alignas(16) glm::vec4 ambientLightColor {1.f, 1.f, 1.f, 0.02f}; // w is light intensity
   //    alignas(32) PointLight pointLights[10];
   //    alignas(8) int numLights {5};
   // };


    struct FcDescriptorBindInfo
    {
       std::vector<VkWriteDescriptorSet> descriptorWrites;
       std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
       std::deque<VkDescriptorBufferInfo> bufferInfos;
       std::deque<VkDescriptorImageInfo> imageInfos;
       bool mIsBindlessIndexingUsed {false};
       void attachBuffer(uint32_t bindSlot, VkDescriptorType type
                         ,const FcBuffer& buffer, VkDeviceSize size, VkDeviceSize offset);
       void attachImage(uint32_t bindSlot, VkDescriptorType type
                        ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler);
       // void attachImageBindless(uint32_t bindSlot, VkDescriptorType type
       //                  ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler);
       void addBinding(uint32_t bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages);
       void enableBindlessTextures();
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
     uint32_t mSetsPerPool;
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
     VkDescriptorPool createPools(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);
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
     void initDescriptorPools(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
     //
     VkDescriptorSetLayout createDescriptorSetLayout(FcDescriptorBindInfo& bindingInfo);
     //
     VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout layout, FcDescriptorBindInfo& bindInfo);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   POOL MANAGEMENT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroy();
  };

}// --- namespace fc --- (END)
