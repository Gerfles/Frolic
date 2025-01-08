

#include "core/fc_descriptors.hpp"
#include "core/fc_renderer.hpp"
#include "fc_pipeline.hpp"
#include "fc_image.hpp"
#include <vulkan/vulkan_core.h>
#include <cstdint>


namespace fc
{
  class FcPipeline;


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

     FcPipeline mOpaquePipeline;
     FcPipeline mTransparentPipeline;
     VkDescriptorSetLayout mMaterialDescriptorLayout;

     struct MaterialConstants
     {
        glm::vec4 colorFactors;
        glm::vec4 metalRoughFactors;
         // Vulkan has a minimum requirement for uniform buffer alignment 256 bytes is a fairly universal gpu target
        glm::vec4 padding[14];
     };

     struct MaterialResources
     {
        FcImage colorImage;
        VkSampler colorSampler;
        FcImage metalRoughImage;
        VkSampler metalRoughSampler;
        VkBuffer dataBuffer;
        uint32_t dataBufferOffset;
     };

   public:
      // TODO think about including
      // FcDescriptorClerk descriptorClerk;
     void buildPipelines(FcRenderer* renderer);
     void clearResources(VkDevice device);

     MaterialInstance writeMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources);
  };




}
