#pragma  once

#include "core/fc_descriptors.hpp"
#include "fc_pipeline.hpp"
// #include "fc_image.hpp"
//#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_core.h>
//#include <cstdint>


namespace fc
{
  class FcRenderer;
  class FcImage;


  enum class MaterialPass : uint8_t {
    MainColor,
    Transparent,
    Other
  };


  enum MaterialFeatures : uint32_t {
    ColorTexture = 1 << 0,
    NormalTexture = 1 << 1,
    RoughMetalTexture = 1 << 2,
    OcclusionTexture = 1 << 3,
    EmissiveTexture = 1 << 4,
    VertexTangentAttribute = 1 << 5,
    VertexTextureCoordinates = 1 << 6
  };


  struct MaterialInstance
  {
     FcPipeline* pPipeline;
     VkDescriptorSet materialSet;
     MaterialPass passType;
  };


  // ?? TODO could pack these tighter but need to study the downfalls, etc.
  // https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout
  struct alignas(16) MaterialConstants
  {
     glm::vec4 colorFactors;
     glm::vec4 metalRoughFactors;
     glm::vec4 emmisiveFactors; // w = emmisive strength
     //
     float  occlusionFactor;
     float iorF0;
     uint32_t flags{0};
     // Vulkan has a minimum requirement for uniform buffer alignment.
     // glm::vec4 padding[14]; // We'll use 256 bytes as its a fairly universal gpu target
  };




  class GLTFMetallicRoughness
  {
   private:
     FcPipeline mOpaquePipeline;
     FcPipeline mTransparentPipeline;
     VkDescriptorSetLayout mMaterialDescriptorLayout;
   public:
     // FIXME should use pointers to resources I believe
     struct MaterialResources
     {
//        FcImage* colorImage;
        FcImage colorImage;
        VkSampler colorSampler;
//        FcImage* metalRoughImage;
        FcImage metalRoughImage;
        VkSampler metalRoughSampler;
        FcImage normalTexture;
        VkSampler normalSampler;
        FcImage occlusionTexture;
        VkSampler occlusionSampler;
        FcImage emissiveTexture;
        VkSampler emissiveSampler;
        FcBuffer dataBuffer;
        uint32_t dataBufferOffset; // TODO should this be VkDeviceSize??
     };

      // TODO think about including
      // FcDescriptorClerk descriptorClerk;
     void buildPipelines(FcRenderer* renderer);
     void clearResources(VkDevice device);

     MaterialInstance writeMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources);
  };
}
