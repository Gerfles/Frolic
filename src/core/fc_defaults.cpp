// fc_defaults.cpp

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_defaults.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_scene_renderer.hpp"
#include "fc_buffer.hpp"
#include "fc_locator.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "glm/packing.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL / UTIL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

namespace fc
{
  //FcDefaults::DefaultSamplers FcDefaults::Samplers;
  FcDefaults::DefaultSamplers FcDefaults::Samplers;
  FcDefaults::DefaultTextures FcDefaults::Textures;
  //
  VkSampler FcDefaults::DefaultSamplers::Terrain;
  VkSampler FcDefaults::DefaultSamplers::Linear;
  VkSampler FcDefaults::DefaultSamplers::Bilinear;
  VkSampler FcDefaults::DefaultSamplers::Trilinear;
  VkSampler FcDefaults::DefaultSamplers::Nearest;
  VkSampler FcDefaults::DefaultSamplers::ShadowMap;
  //
  FcImage FcDefaults::DefaultTextures::white;
  FcImage FcDefaults::DefaultTextures::black;
  FcImage FcDefaults::DefaultTextures::grey;
  FcImage FcDefaults::DefaultTextures::checkerboard;
  //
  FcMaterial FcDefaults::DefaultMaterials::blank;
  FcBuffer FcDefaults::DefaultMaterials::materialDataBuffer;


  void FcDefaults::init(VkDevice device)
  {
    Textures.init();
    Samplers.init(device);
    Materials.init();
  }

  // TODO implement
  void FcDefaults::addSampler(VkSamplerCreateInfo& samplerInfo)
  {

  }

  // TODO check that mag filter is working properly with default linear
  // TODO place reference sampler at top (one with all settings and comments)
  void FcDefaults::DefaultSamplers::init(VkDevice device)
  {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // *-*-*-*-*-*-*-*-*-*-   NEAREST SAMPLER / NEAREST MIPMAP   *-*-*-*-*-*-*-*-*-*- //
    // How to render when image is magnified on the screen
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    // How to render when image is minified on the screen
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    /* samplerInfo.minFilter = VK_FILTER_NEAREST; */
    // How to handle wrap in the U (x) direction
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // How to handle wrap in the V (y) direction
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // How to handle wrap in the W (z) direction
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // Border beyond texture (when clamp to border is used--good for shadow maps)
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // WILL USE NORMALIZED COORD. (coords will be between 0-1)
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // Mipmap interpolation mode (between two levels of mipmaps)
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    // used to force vulkan to use lower level of detail and mip level
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    // maximum level of detail to pick mip level
    // TODO if mipLevels determine sampler maxLod then perhaps we need to create
    // a separate sampler for each texture (for images, could still keep defaults) or
    // create several based on mipLevels and assign to textures accordingly...
    /* samplerInfo.maxLod = static_cast<float>(mMipLevels); */
    samplerInfo.maxLod = 0.0f;
    // enable anisotropy
    samplerInfo.anisotropyEnable = VK_TRUE;
    // TODO should allow this to be user definable or at least profiled at install/runtime
    // Amount of anisotropic samples being taken
    /* samplerInfo.maxAnisotropy = gpu.Properties().maxSamplerAnisotropy; */
    // TODO DON'T hard code
    samplerInfo.maxAnisotropy = VK_SAMPLE_COUNT_16_BIT;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &Samplers.Nearest) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }

    // -*-*-*-*-*-*-*-*-*-*-   NEAREST SAMPLER / LINEAR MIPMAP   -*-*-*-*-*-*-*-*-*-*- //
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    /* samplerInfo.maxLod = 0.0; */
    if (vkCreateSampler(device, &samplerInfo, nullptr, &Samplers.Bilinear) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }

    // *-*-*-*-*-*-*-*-*-*-   LINEAR SAMPLER / NEAREST MIPMAP   *-*-*-*-*-*-*-*-*-*- //
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &Samplers.Linear) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }

    // -*-*-*-*-*-*-*-*-*-*-   LINEAR SAMPLER / LINEAR MIPMAP   -*-*-*-*-*-*-*-*-*-*- //
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &Samplers.Trilinear) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TERRAIN SAMPLER   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    /* samplerInfo.maxLod = 0.0f; */
    if (vkCreateSampler(device, &samplerInfo, nullptr, &Samplers.Terrain) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW MAP SAMPLER   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    // TODO determine if border color would be better suited as float for shader
    // ORIGINAL VALUE
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    /* samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK; */
    /* samplerInfo.maxLod = 1.0f; */
    if (vkCreateSampler(device, &samplerInfo, nullptr, &Samplers.ShadowMap) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }
  }


  void FcDefaults::DefaultTextures::init()
  {
    // TODO make sure these are implemented within bindless paths
    // -*-*-*-*-   3 DEFAULT TEXTURES--WHITE, GREY, BLACK AND CHECKERBOARD   -*-*-*-*- //
    uint32_t whiteValue = glm::packUnorm4x8(glm::vec4(1.f, 1.f, 1.f, 1.f));
    white.createTexture(1, 1, static_cast<void*>(&whiteValue)
                        , sizeof(whiteValue));

    uint32_t greyValue = glm::packUnorm4x8(glm::vec4(0.36f, 0.36f, 0.36f, 1.f));
    grey.createTexture(1, 1, static_cast<void*>(&greyValue)
                               , sizeof(greyValue));

    uint32_t blackValue = glm::packUnorm4x8(glm::vec4(0.f, 0.f, 0.f, 1.f));
    black.createTexture(1, 1, static_cast<void*>(&blackValue)
                                , sizeof(blackValue));

    // checkerboard image
    uint32_t checkerValue = glm::packUnorm4x8(glm::vec4(1.f, 0.f, 1.f, 1.f));
    std::array<uint32_t, 16 * 16> pixels;
    for (int x = 0; x < 16; x++)
    {
      for (int y = 0; y < 16; y++)
      {
        pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? checkerValue : blackValue;
      }
    }
    checkerboard.createTexture(16, 16, static_cast<void*>(&pixels)
                                       , pixels.size() * sizeof(pixels[0]));
  }


  //
  //
  void FcDefaults::DefaultMaterials::init()
  {
    // Create buffer to hold the material data
      materialDataBuffer.allocate(sizeof(MaterialConstants), FcBufferTypes::Uniform);

      /* materials[i] = std::make_shared<FcMaterial>(); */
      blank.materialType = FcMaterial::Type::Opaque;

      FcDescriptorBindInfo bindInfo{};

      bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);

      // create the descriptor set Layout for the material
      VkDescriptorSetLayout descriptorSetLayout =
        FcLocator::DescriptorClerk().createDescriptorSetLayout(bindInfo);


      MaterialConstants constants;
      constants.colorFactors.x = 1.0f;
      constants.colorFactors.y = 1.0f;
      constants.colorFactors.z = 1.0f;
      constants.colorFactors.w = 1.0f;
      // Metal Rough
      constants.metalRoughFactors.x = 0.5;
      constants.metalRoughFactors.y = 0.5;
      // Index of Refraction
      constants.iorF0 = 0.04;

      // emmisive factors
      constants.emmisiveFactors = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

      //TODO verify what to set default as
      constants.occlusionFactor = 1.0f;

      // TODO Set to the checkerboard texture to spot it easier
      /* constants.colorIndex = drawCollection.mTextures.get(index)->Handle(); */
      constants.colorIndex = 0;

      // *-*-*-*-*-*-*-*-*-*-*-*-*-   MATERIAL DATA BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*- //
      bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, materialDataBuffer
                            , sizeof(MaterialConstants), 0);

      // Write material parameters to buffer.
      MaterialConstants* defaultMaterialConstants = static_cast<MaterialConstants*>
                                                    (materialDataBuffer.getAddress());
      *defaultMaterialConstants = constants;

      // TODO make the ubo descriptor set the same for all materials:
      // perhaps store an addressable buffer within the the GPU mem:

      // Build descriptor sets for each material
      blank.materialSet
        = FcLocator::DescriptorClerk().createDescriptorSet(descriptorSetLayout, bindInfo);

      // FIXME
      /* FcLocator::DescriptorClerk().destroyDescriptorSetLayout(descriptorSetLayout); */
  }



  void FcDefaults::destroy()
  {
    Materials.destroy();
    Samplers.destroy();
    Textures.destroy();
  }


  //
  //
  void FcDefaults::DefaultMaterials::destroy()
  {
    // TODO implement
    materialDataBuffer.destroy();
  }


  void FcDefaults::DefaultTextures::destroy()
  {
    white.destroy();
    grey.destroy();
    black.destroy();
    checkerboard.destroy();
  }

  void FcDefaults::DefaultSamplers::destroy()
  {
    vkDestroySampler(FcLocator::Device(), Terrain, nullptr);
    vkDestroySampler(FcLocator::Device(), Nearest, nullptr);
  }





}// --- namespace fc --- (END)
