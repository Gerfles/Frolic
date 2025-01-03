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

   // TODO combine with SceneData
  struct GlobalUbo
  {
     glm::mat4 projection {1.f};
     glm::mat4 view {1.f};
     glm::mat4 invView {1.f};
     glm::vec4 ambientLightColor {1.f, 1.f, 1.f, 0.1f}; // w is light intensity
     PointLight pointLights[10];
     int numLights{0};
  };


  struct SceneData
  {
     glm::mat4 view {1.f};
     glm::mat4 projection {1.f};
     glm::mat4 viewProj{1.f};
      //glm::mat4 invView {1.f};
     glm::vec4 ambientLight {1.f, 1.f, 1.f, 0.1f}; // w is light intensity
     glm::vec4 sunlightDirection; // w for power
     glm::vec4 sunlightColor;
      // PointLight pointLights[10];
      // int numLights{0};
  };


  struct FcBufferBindingInfo
  {
     uint32_t bindingSlotNumber{0};
     VkDescriptorType type{VK_DESCRIPTOR_TYPE_MAX_ENUM};
     VkShaderStageFlags shaderStages{0};
     VkBuffer buffer{nullptr};
     size_t bufferSize{0};
     size_t bufferOffset{0};
  };

  struct FcImageBindingInfo
  {
     uint32_t bindingSlotNumber{0};
     VkDescriptorType type{VK_DESCRIPTOR_TYPE_MAX_ENUM};
     VkShaderStageFlags shaderStages{0};
     VkImageView imageView{nullptr};
     VkSampler imageSampler{nullptr};
     VkImageLayout imageLayout{VK_IMAGE_LAYOUT_UNDEFINED};
  };


   // TODO ?? could change this to FcDescriptorSet or FcDescriptorBank...
  class FcDescriptorClerk
  {
   private:
      // TODO work to understand this better...
     struct PoolSizeRatio
     {
        VkDescriptorType type;
        float ratio;
     };
      //
      //   const FcGpu* pGpu;
//     VkDescriptorSetLayout mUboDescriptorSetLayout;
      //VkDescriptorSetLayout mSamplerDescriptorSetLayout;
      // - TEXTURES -
      //VkDescriptorPool mSamplerDescriptorPool; // TODO SHOULD REMOVE - don't have to have multiple pools here but not sure if any advantages??
      //VkDescriptorPool mDescriptorPool;

      //std::vector<VkDescriptorSet> mUboDescriptorSets; // one for each swap chain image

      // standard uniform buffers that are the same for every model
     std::vector<FcBuffer> mGlobalUniformBuffers;
      // dynamic uniform buffers that are unique to each mesh
      //std::vector<FcBuffer> mModelDynUniformBuffers;
     size_t mModelUniformAlignment;
      // NO LONGER NEED but left as reference for dynamic uniform buffer objects
      //Model* pModelTransferSpace;
     void allocateDynamicBufferTransferSpace();
     // void createDescriptorPool(int uniformBufferCount);
     // void createDescriptorSetLayout();
     // void createDescriptorSets(int uniformBufferCount);
     // void createUniformBuffers(int uniformBufferCount);
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     VkDevice pDevice;
      //VkDescriptorSetLayout mLayout;
      //std::vector<VkDescriptorSetLayout> mLayouts;
      //VkDescriptorSetLayout mSceneDataDescriptorLayout;
      // VkDescriptorPool mDescriptorPool2;
      //VkDescriptorSet mDescriptorSet;
     std::vector<PoolSizeRatio> mPoolRatios;
     std::vector<VkDescriptorPool> mFullPools;
     std::vector<VkDescriptorPool> mReadyPools;
     uint32_t mSetsPerPool;
      // descriptor sets
     // std::deque<VkDescriptorImageInfo> mImageInfos;
     // std::deque<VkDescriptorBufferInfo> mBufferInfos;
     // std::vector<VkWriteDescriptorSet> mDescriptorWrites;

   public:
//       void clearWrites();
      //void updateSet(VkDescriptorSet set);
     VkDescriptorSet createDescriptorSet(std::vector<FcBufferBindingInfo>& bufferBindingInfos
                                         , std::vector<FcImageBindingInfo>& imageBindingInfos
                                         , VkDescriptorSetLayoutCreateFlags flags);

//     void addBinding(uint32_t bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages);
//     void createDescriptorPool2(uint32_t maxSets, std::vector<PoolSizeRatio> poolRatios);
//       void createDescriptorSetLayout2(VkDescriptorSetLayoutCreateFlags flags = 0);
      // VkDescriptorSetLayout createDescriptorSetLayout2(std::vector<DescriptorBindingInfo> bindingInfoList
      //                                                  , VkDescriptorSetLayoutCreateFlags flags = 0);
     VkDescriptorSetLayout createDescriptorSetLayout(std::vector<FcBufferBindingInfo>& bufferBindingInfos
                                                     , std::vector<FcImageBindingInfo>& imageBindingInfos
                                                     , VkDescriptorSetLayoutCreateFlags flags);
     VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout, void* pNext);

     void addDescriptorWrites(std::vector<FcBufferBindingInfo>& bufferBindingInfos
                              , std::vector<FcImageBindingInfo>& imageBindingInfos
                              , VkDescriptorSet descriptorSet);


      //void cleanupLayoutBindings();
      //void createDescriptorSets2(FcImage& image);

     void clearPools();
     void destroyPools();
     VkDescriptorPool getPool();
     VkDescriptorPool createPool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);
      //void destroyPool();
      //VkDescriptorSetLayout* vkDescriptorLayout() { return &mLayout; }
      //const VkDescriptorSet* vkDescriptor() const { return &mDescriptorSet; }
      // void createBufferWrite(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
      // void createImageWrite(int binding, VkImageView imageView, VkSampler sampler
      //                       , VkImageLayout layout, VkDescriptorType type);
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     void init(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
     // uint32_t createTextureDescriptor(VkImageView textureImageView, VkSampler& textureSampler);
     //void update(uint32_t imgIndex, void* data);
     void destroy();
     size_t ModelUniformAlignment() { return mModelUniformAlignment; }
      //void setUpPipelineLayout(VkPipelineLayoutCreateInfo& pipelineLayoutInfo);
      // GETTER FUNCTIONS
     VkDescriptorSetLayout UboSetLayout() { return mUboDescriptorSetLayout; }
     VkDescriptorSetLayout SamplerSetLayout() { return mSamplerDescriptorSetLayout; }
     VkDescriptorSet UboDescriptorSet(uint32_t index) { return mUboDescriptorSets[index]; }
     VkDescriptorSet SamplerDescriptorSet(uint32_t index) { return mSamplerDescriptorSets[index]; }


  };



}//namespace lve _END_
