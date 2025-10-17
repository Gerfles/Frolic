#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_buffer.hpp"
#include "fc_image.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <glm/ext/vector_float4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL / UTILS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <deque>
#include <span>
#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
   // TODO handle this differently
  const int MAX_OBJECTS = 30;
  const int MAX_LIGHTS = 10;
  const int MAX_BILLBOARDS = 30;



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
       void attachBuffer(uint32_t bindSlot, VkDescriptorType type
                         ,const FcBuffer& buffer, VkDeviceSize size, VkDeviceSize offset);
       void attachImage(uint32_t bindSlot, VkDescriptorType type
                        ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler);
       void attachImageBindless(uint32_t bindSlot, VkDescriptorType type
                        ,const FcImage& image, VkImageLayout layout, VkSampler imageSampler);
       void addBinding(uint32_t bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages);
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
     // Total number of textures allowed to be bound dynamically
     static const uint32_t MAX_BINDLESS_TEXTURES = 1024;
     //?? not sure why we use 10 here
     static const uint32_t BINDLESS_TEXTURE_BIND_SLOT = 10;

     size_t mModelUniformAlignment;
     VkDevice pDevice;
     // ?? Could use as an atlas if wanted/needed
      //std::vector<VkDescriptorSetLayout> mLayouts;
     std::vector<PoolSizeRatio> mPoolRatios;
     std::vector<VkDescriptorPool> mFullPools;
     std::vector<VkDescriptorPool> mReadyPools;
     uint32_t mSetsPerPool;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   BINDLESS RESOURCES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
     bool isBindlessSupported = true;
     VkDescriptorPool mBindlessDescriptorPool;

   public:

     // TODO
     VkDescriptorSet mBindlessDescriptorSet;
     VkDescriptorSetLayout mBindlessDescriptorLayout;

     FcDescriptorClerk() = default;
     FcDescriptorClerk(const FcDescriptorClerk&) = delete;
     FcDescriptorClerk(FcDescriptorClerk&&) = delete;
     FcDescriptorClerk& operator=(const FcBuffer&) = delete;
     FcDescriptorClerk& operator=(FcDescriptorClerk&&) = delete;
     void initDescriptorPools(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
     void createBindlessDescriptorSets();
     VkDescriptorSetLayout createDescriptorSetLayout(FcDescriptorBindInfo& bindingInfo
                                                     , VkDescriptorSetLayoutCreateFlags flags = 0);
     VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout layout, FcDescriptorBindInfo& bindInfo);
     VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout, void* pNext);
     size_t ModelUniformAlignment() { return mModelUniformAlignment; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   POOL MANAGEMENT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void clearPools();
     void destroyPools();
     VkDescriptorPool getPool();
     VkDescriptorPool createPools(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);
     //
     void destroy();
  };

}// --- namespace fc --- (END)
