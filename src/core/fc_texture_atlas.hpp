#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <vulkan/vulkan.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>

namespace fc
{

class FcTextureAtlas
{
   std::vector<VkDescriptorSet> mSamplerDescriptorSets; // one for each texture
};

}// --- namespace fc --- (END)
