#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// TODO change to #include "fc_mesh.hpp"
// #include "core/fc_font.hpp"
#include "fc_light.hpp"
#include "fc_mesh.hpp"
#include "fc_buffer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "glm/mat4x4.hpp"
#include "vulkan/vulkan_core.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
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

    struct GlobalUbo
  {
     glm::mat4 projection {1.f};
     glm::mat4 view {1.f};
     glm::mat4 invView {1.f};
     glm::vec4 ambientLightColor {1.f, 1.f, 1.f, 0.1f}; // w is light intensity
     PointLight pointLights[10];
     int numLights{0};
  };


  struct PoolSizeRatio
  {
     VkDescriptorType type;
     float ratio;
  };




    class FcMesh;

   // TODO ?? could change this to FcDescriptorSet or FcDescriptorBank...
  class FcDescriptor
  {
   private:

     const FcGpu* pGpu;
     VkDescriptorSetLayout mUboDescriptorSetLayout;
     VkDescriptorSetLayout mSamplerDescriptorSetLayout;
      // - TEXTURES -
     VkDescriptorPool mSamplerDescriptorPool; // TODO SHOULD REMOVE - don't have to have multiple pools here but not sure if any advantages??
     VkDescriptorPool mDescriptorPool;

     std::vector<VkDescriptorSet> mUboDescriptorSets; // one for each swap chain image
     std::vector<VkDescriptorSet> mSamplerDescriptorSets; // one for each texture (NOT tied to number of swap chain images)
      // standard uniform buffers that are the same for every model
     std::vector<FcBuffer> mGlobalUniformBuffers;
      // dynamic uniform buffers that are unique to each mesh
      //std::vector<FcBuffer> mModelDynUniformBuffers;
     size_t mModelUniformAlignment;
      // NO LONGER NEED but left as reference for dynamic uniform buffer objects
      //Model* pModelTransferSpace;
      // push constants
     void allocateDynamicBufferTransferSpace();
     void createDescriptorPool(int uniformBufferCount);
     void createDescriptorSetLayout();
     void createDescriptorSets(int uniformBufferCount);
     void createUniformBuffers(int uniformBufferCount);
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkDescriptorSetLayout mLayout;
     VkDescriptorPool mDescriptorPool2;
     VkDescriptorSet mDescriptorSet;
     std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings;

   public:

     void addBinding(uint32_t bindSlot, VkDescriptorType type, VkShaderStageFlags shaderStages);
     void createDescriptorPool2(uint32_t maxSets, std::vector<PoolSizeRatio> poolRatios);
     void createDescriptorSetLayout2(VkDescriptorSetLayoutCreateFlags flags = 0);
     void cleanupLayoutBindings();
     void clearDescriptors();
     void createDescriptorSets2(FcImage& image);
     void destroyPool();
     VkDescriptorSetLayout* vkDescriptorLayout() { return &mLayout; }
     const VkDescriptorSet* vkDescriptor() const { return &mDescriptorSet; }
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     void create(const FcGpu* gpu, int uniformBufferCount);
     uint32_t createTextureDescriptor(VkImageView textureImageView, VkSampler& textureSampler);
     void update(uint32_t imgIndex, void* data);
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
