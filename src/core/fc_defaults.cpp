// fc_defaults.cpp
#include "fc_defaults.hpp"
#include "fc_locator.hpp"


namespace fc
{
  Samplers::Samplers()
  {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // How to render when image is magnified on the screen
    samplerInfo.magFilter = VK_FILTER_LINEAR;

    // How to render when image is minified on the screen
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    // How to handle wrap in the U (x) direction
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    // How to handle wrap in the V (y) direction
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    // How to handle wrap in the W (z) direction
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    // Border beyond texture (when clamp to border is used--good for shadow maps)
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    // WILL USE NORMALIZED COORD. (coords will be between 0-1)
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // Mipmap interpolation mode (between two levels of mipmaps)
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // used to force vulkan to use lower level of detail and mip level
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

    // maximum level of detail to pick mip level
    samplerInfo.maxLod = 0.0f;
    // enable anisotropy
    samplerInfo.anisotropyEnable = VK_FALSE;
    // TODO should allow this to be user definable or at least profiled at install/runtime
    // Amount of anisotropic samples being taken
    samplerInfo.maxAnisotropy = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateSampler(FcLocator::Device(), &samplerInfo, nullptr, &Linear) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }
  }
}
