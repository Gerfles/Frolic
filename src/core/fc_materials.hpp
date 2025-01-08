#pragma  once

#include "core/fc_descriptors.hpp"
#include "fc_pipeline.hpp"
#include "fc_image.hpp"
#include <vulkan/vulkan_core.h>
#include <cstdint>


namespace fc
{
  class FcPipeline;
  class FcRenderer;

enum class MaterialPass : uint8_t {
  MainColor,
  Transparent,
  Other
  };

  struct MaterialInstance
  {
     FcPipeline* pPipeline;
     VkDescriptorSet materialSet;
     MaterialPass passType;
  };

  class GLTFMetallicRoughness
  {
   private:
     FcPipeline mOpaquePipeline;
     FcPipeline mTransparentPipeline;
     VkDescriptorSetLayout mMaterialDescriptorLayout;
   public:
     struct MaterialConstants
     {
        glm::vec4 colorFactors;
        glm::vec4 metalRoughFactors;
        // Vulkan has a minimum requirement for uniform buffer alignment.
        glm::vec4 padding[14]; // We'll use 256 bytes as its a fairly universal gpu target
     };

     struct MaterialResources
     {
        FcImage* colorImage;
        VkSampler colorSampler;
        FcImage* metalRoughImage;
        VkSampler metalRoughSampler;
        FcBuffer* dataBuffer;
        uint32_t dataBufferOffset; // TODO should this be VkDeviceSize??
     };

      // TODO think about including
      // FcDescriptorClerk descriptorClerk;
     void buildPipelines(FcRenderer* renderer);
     void clearResources(VkDevice device);

     MaterialInstance writeMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources);
  };




}
