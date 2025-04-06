#pragma  once

#include "fc_descriptors.hpp"
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

  enum class MaterialFeatures : uint32_t {
    HasColorTexture = 0b1 << 0,
    HasNormalTexture = 0b1 << 1,
    HasRoughMetalTexture = 0b1 << 2,
    HasOcclusionTexture = 0b1 << 3,
    HasEmissiveTexture = 0b1 << 4,
    HasVertexTangentAttribute = 0b1 << 5,
    HasVertexTextureCoordinates = 0b1 << 6,
  };
  MaterialFeatures operator~ (MaterialFeatures const & rhs);
  MaterialFeatures operator|(MaterialFeatures lhs, MaterialFeatures rhs);
  MaterialFeatures operator&(MaterialFeatures lhs, MaterialFeatures rhs);
  MaterialFeatures& operator|=(MaterialFeatures& lhs, MaterialFeatures const& rhs);
  MaterialFeatures& operator&=(MaterialFeatures& lhs, MaterialFeatures const& rhs);



  // TODO Can incrementally update push constants according to:
  // https://docs.vulkan.org/guide/latest/push_constants.html
  // May provide some benefits if done correctly
  struct DrawPushConstants
  {
     glm::mat4 worldMatrix;
     glm::mat4 normalTransform;
     VkDeviceAddress vertexBuffer;
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
     // TODO create two structs for material constants, one for full feature rendering and
     // raytracing and one for actual game engine.
     glm::vec4 colorFactors;
     glm::vec4 metalRoughFactors;
     glm::vec4 emmisiveFactors; // w = emmisive strength
     //
     float  occlusionFactor;
     float iorF0;
     MaterialFeatures flags{0};
     // Vulkan has a minimum requirement for uniform buffer alignment.
     // glm::vec4 padding[14]; // We'll use 256 bytes as its a fairly universal gpu target
  };


  class GLTFMetallicRoughness
  {
   private:
     FcPipeline mOpaquePipeline;
     FcPipeline mTransparentPipeline;
     FcPipeline mNormalDrawPipeline;
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
