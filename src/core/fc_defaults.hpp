// fc_defaults.hpp
#pragma once

#include <vulkan/vulkan.h>

//
namespace fc
{
  // ?? Might want to make these static members of fcImage class but it depends on
  // if we want them to exist for the life of the program or if the createInfos for
  // instance can be deleted once they go out of scope.
  struct Samplers
  {
     VkSampler Linear;
     VkSampler Nearest;
     Samplers();
  };

  enum class ImageTypes : uint8_t {
    Texture = 0,  // Default texture image
    TextureArray = 8, // Texture but with multiple array layers
    TextureMipMapped = 1,  // Texture with mipMaps
    Cubemap = 2,           // Cubemap image (with layers)
    HeightMap = 3,         // HeightMap
    DrawBuffer = 4,        // Draw buffer
    NormalMap = 5,         // Normal map
    DepthBuffer = 6,       // Depth buffer
    Custom = 7,            // User defined image using a passed in create info
  };


}
