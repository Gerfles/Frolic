//>--- fc_assert.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "log.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_log.h"
/* #include <Vulkan-Utility-Libraries/vulkan_to_string.hpp> */
/* #include <vulkan_to_string.hpp> */
/* #include <Vulkan-Utility-Libraries/include/vulkan/vk_enum_string_helper.h> */
/* #include "vulkan/vulkan_to_string.hpp" */
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


// TODO look into assert.h for proper debug asserts // #include <assert.h>
//   #define assert(e)  \
//     ((void) ((e) ? ((void)0) : __assert (#e, __ASSERT_FILE_NAME, __LINE__)))
// #define __assert(e, file, line) \
//     ((void)printf ("%s:%d: failed assertion `%s'\n", file, line, e), abort())
// TEST
#if defined(_WIN32) // or _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else // works in most POSIX systems and should be in macOS
#define DEBUG_BREAK raise(SIGTRAP)
#endif

#define DBG_ASSERT(f) {if(!(f)){DEBUG_BREAK; }; }

// NAN Test
#define DBG_VALID(f) { if( (f)!=(f) ) {DBG_ASSERT(false);} }

// Assert with message
#define DBG_ASSERT_MSG(val, errmsg) if(!(val)) { DBG_ASSERT( false ) }

// DELETE
#define DBG_ASSERT_VULKAN_MSG(result, errMsg )          \
  if (!(result == VK_SUCCESS))                          \
  {                                                     \
    SDL_Log("ERROR::VULKAN[%d]: %s", result, errMsg);   \
    DBG_ASSERT( false )                                 \
  }


// KEEP
#define VK_ASSERT(vkFunction) {                                 \
    const VkResult result = vkFunction;                         \
    if (result != VK_SUCCESS)                                   \
    {                                                           \
      SDL_Log("ERROR: Vulkan API call failed: %s:%i\n %s\n %s\n",       \
              __FILE__, __LINE__, #vkFunction,                          \
              #vkFunction);                                             \
      /* string_VkResult(result));                           \ */       \
      DBG_ASSERT(false);                                                \
    }                                                                   \
  }




//
#define DBG_ASSERT_FT(result, errMsg)                                   \
  if (!(result == 0))                                                   \
  {                                                                     \
    SDL_Log("ERROR::FREETYPE:(%#.2X) %s", result, errMsg);              \
    DBG_ASSERT(false)                                                   \
  }


namespace frolic
{

#define FCASSERT(condition) if(!(condition)) {    \
    fcPrint(FROLIC_FILELINE("FALSE\n"));          \
    FROLIC_DEBUG_BREAK                            \
}

#if defined (_MSC_VER)
#define FCASSERTM(condition, message, ...) if(!(condition)) {            \
  fcPrint(FROLIC_FILELINE(FROLIC_CONCAT(message, "\n")), __VA_ARGS__);    \
  FROLIC_DEBUG_BREAK                                                    \
}
#else
#define FCASSERTM(condition, message, ...) if (!(condition)) {           \
  fcPrint(FROLIC_FILELINE(FROLIC_CONCAT(message, "\n")), ## __VA_ARGS__); \
  FROLIC_DEBUG_BREAK                                                    \
}
#endif // MSCV

} // --- namespace frolic  --- (END)
