//>--- fc_resources.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  enum class ResourceDeletionType : uint8_t
  {
    Buffer
  , Texture
  , Billboard
  , Pipeline
  , Sampler
  , DescriptorSetLayout
  , DescriptorSet
  , RenderPass
  , ShaderState
  , Count
  };


  static const u32 INVALID_INDEX = 0xffffffff;
  static const u16 INVALID_TEXTURE_INDEX = U16_MAX;

  typedef u32 ResourceHandle;
  /* using ResourceHandle = u32; */

  struct TextureHandle
  {
     ResourceHandle index;
  };

}// --- namespace fc --- (END)
