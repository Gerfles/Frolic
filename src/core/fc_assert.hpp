//>--- fc_assert.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "log.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_log.h"
#include <vulkan/vk_enum_string_helper.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //

// TODO for release version, prefer message boxes for some things
// std::ostringstream errorMsg;
// errorMsg << "Window coulde not be created! SDL Error: " << SDL_GetError();
// SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL", errorMsg.str().c_str(), mWindow);
// return false;

namespace fc
{

#define FC_ERROR(message, function,  result)                            \
  {                                                                     \
    fcPrintEndl("ERROR: %s failed: %s: Line:%i\n  Function: %s\n  Result: %s", \
                message, __FILE__, __LINE__, #function, result);        \
  }


// Vulkan assert function for debuging
#define VK_ASSERT(vkFunction) {                                         \
    const VkResult result = vkFunction;                                 \
    if (result != VK_SUCCESS)                                           \
    {                                                                   \
      FC_ERROR("Vulkan API call", vkFunction, string_VkResult(result)); \
      FC_DEBUG_BREAK;                                                   \
    }                                                                   \
  }


// FreeType assert funtion for debuging
#define FT_ASSERT(freeTypeFunction) {                                   \
    const int result = freeTypeFunction;                                \
    if (result != 0)                                                    \
    {                                                                   \
      FC_ERROR("FreeType function call", freeTypeFunction, result);     \
      FC_DEBUG_BREAK;                                                   \
    }                                                                   \
  }                                                                     \


// General Frolic assert function for debuging
#define FC_ASSERT(condition) {                          \
    if( !(condition) )                                  \
    {                                                   \
      FC_ERROR("Frolic assert", condition, "FALSE");    \
      FC_DEBUG_BREAK;                                   \
    }                                                   \
  }


// Frolic assert with custom message added for debuging
#if defined (_MSC_VER)
#define FC_ASSERT_MSG(condition, message, ...) {        \
    if ( !(condition) )                                 \
    {                                                   \
      FC_ERROR("Frolic assert", condition, "FALSE");    \
      FcPrintEndl("  " message, __VA_ARGS__);           \
      FC_DEBUG_BREAK;                                   \
    }                                                   \
  }
#else
#define  FC_ASSERT_MSG(condition, message, ...) {       \
    if ( !(condition) )                                 \
    {                                                   \
      FC_ERROR("Frolic assert", condition, "FALSE");    \
      fcPrintEndl("  " message, ## __VA_ARGS__);        \
      FC_DEBUG_BREAK;                                   \
    }                                                   \
  }
#endif // (_MSC_VER) - (END)


// NAN Test
#define FC_VALID(num) {                         \
    if( (num)!=(num) )                          \
    {                                           \
      FC_ASSERT(false);                         \
    }                                           \
  }

} // --- namespace frolic  --- (END)
