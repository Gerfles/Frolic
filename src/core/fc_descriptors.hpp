#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_light.hpp"
#include "fc_buffer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "glm/mat4x4.hpp"
#include "vulkan/vulkan_core.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <deque>
#include <glm/ext/vector_float4.hpp>
#include <vector>
#include <array>


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

  struct SceneData
  {
     glm::vec4 eye {0.0};
     glm::mat4 view {1.f};
     glm::mat4 projection {1.f};
     glm::mat4 viewProj{1.f};
     glm::mat4 lighSpaceTransform{1.f};
     glm::vec4 ambientLight {1.f, 1.f, 1.f, 0.1f}; // w is light intensity
     glm::vec4 sunlightDirection; // w for power
     glm::vec4 sunlightColor;
      // PointLight pointLights[10];
      // int numLights{0};
  };

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
     size_t mModelUniformAlignment;
     VkDevice pDevice;
     // ?? Could use as an atlas if wanted/needed
      //std::vector<VkDescriptorSetLayout> mLayouts;
     std::vector<PoolSizeRatio> mPoolRatios;
     std::vector<VkDescriptorPool> mFullPools;
     std::vector<VkDescriptorPool> mReadyPools;
     uint32_t mSetsPerPool;

   public:
     FcDescriptorClerk() = default;
     FcDescriptorClerk(const FcDescriptorClerk&) = delete;
     FcDescriptorClerk(FcDescriptorClerk&&) = delete;
     FcDescriptorClerk& operator=(const FcBuffer&) = delete;
     FcDescriptorClerk& operator=(FcDescriptorClerk&&) = delete;
     void initDescriptorPools(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
     VkDescriptorSetLayout createDescriptorSetLayout(FcDescriptorBindInfo& bindingInfo
                                                     , VkDescriptorSetLayoutCreateFlags flags = 0);
     VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout layout, FcDescriptorBindInfo& bindInfo);
     VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout, void* pNext);
     size_t ModelUniformAlignment() { return mModelUniformAlignment; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   POOL MANAGEMENT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void clearPools();
     void destroyPools();
     VkDescriptorPool getPool();
     VkDescriptorPool createPool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);
     //
     void destroy();
  };

}// --- namespace fc --- (END)
