// fc_defaults.hpp
#pragma once

#include "fc_image.hpp"
#include <vulkan/vulkan.h>

//
namespace fc
{
  class FcDefaults
  {
   private:
     //
     struct DefaultSamplers
     {
        static VkSampler Terrain;
        // TODO create NEAREST
        static VkSampler Nearest;
        static VkSampler Linear;
        static VkSampler ShadowMap;
        static void init(VkDevice device);
        static void destroy();
     };
     //
     struct DefaultTextures
     {
        static FcImage white;
        static FcImage black;
        static FcImage grey;
        static FcImage checkerboard;
        static void init();
        static void destroy();
     };

   public:
     // ?? Might want to make these static members of fcImage class but it depends on
     // if we want them to exist for the life of the program or if the createInfos for
     // instance can be deleted once they go out of scope.
     static DefaultSamplers Samplers;
     static DefaultTextures Textures;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     static void init(VkDevice device);
     // TODO implement
     static void addSampler(VkSamplerCreateInfo& samplerInfo);
     static void destroy();
  }; // ---   class FcDefaults --- (END)
}// --- namespace fc --- (END)
