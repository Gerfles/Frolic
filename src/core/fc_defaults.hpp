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
}
