#pragma once

#include "SDL2/SDL_log.h"
#include "vulkan/vulkan_core.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include <signal.h>
#include <stdio.h> //vsprintf_s
// TODO look into assert.h for proper debug asserts
// #include <assert.h>

// TODO
// Lots of testing required to see if the following code is working correctly.
// #if defined(_WIN32) // or _MSC_VER
// #define DEBUG_BREAK __debugbreak()
// #else // works in most POSIX systems and should be in macOS
// #define DEBUG_BREAK raise(SIGTRAP)
// #endif

// #define DBG_ASSERT(f) {if(!(f)){DEBUG_BREAK; }; }

// // NAN Test
// #define DBG_VALID(f) { if( (f)!=(f) ) {DBG_ASSERT(false);} }

// // Assert with message
// #define DBG_ASSERT_MSG(val, errmsg)             \
//   if(!(val))                                    \
//   {                                             \
//     DBG_ASSERT( false )                         \
//       }

// #define DBG_ASSERT_VULKAN_MSG(result, errmsg )    \
//   if (!((result == VK_SUCCESS)))                   \
//   {                                             \
//     DBG_ASSERT( false )                         \
//       }


// TODO clear output.txt each time and think about printing differently
inline void dprintf(const char* fmt, ...)
{
  va_list parms;
  static char buf[2048] = {0};

   // try to print in the allocated space
  va_start(parms, fmt);
   vsprintf(buf, fmt, parms);
  va_end(parms);

   // write the information out ot a txt file
//  #if 0
  FILE *fp = fopen("output.txt", "w");
  fprintf(fp, "%s", buf);
  fclose(fp);
   // #endif

   // output to the visual studio window
   // OutputDebugStringA( buf ); 
} // ende dprintf(..)


#if defined(_WIN32) // or _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else // works in most POSIX systems and should be in macOS
#define DEBUG_BREAK raise(SIGTRAP)
#endif

#define DBG_ASSERT(f) {if(!(f)){DEBUG_BREAK; }; }

// NAN Test
#define DBG_VALID(f) { if( (f)!=(f) ) {DBG_ASSERT(false);} }

// Assert with message
#define DBG_ASSERT_MSG(val, errmsg)             \
  if(!(val))                                    \
  {                                             \
    DBG_ASSERT( false )                         \
      }

    // VK_SUCCESS = 0,
    // VK_NOT_READY = 1,
    // VK_TIMEOUT = 2,
    // VK_EVENT_SET = 3,
    // VK_EVENT_RESET = 4,
    // VK_INCOMPLETE = 5,
    // VK_ERROR_OUT_OF_HOST_MEMORY = -1,
    // VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
    // VK_ERROR_INITIALIZATION_FAILED = -3,
    // VK_ERROR_DEVICE_LOST = -4,
    // VK_ERROR_MEMORY_MAP_FAILED = -5,
    // VK_ERROR_LAYER_NOT_PRESENT = -6,
    // VK_ERROR_EXTENSION_NOT_PRESENT = -7,
    // VK_ERROR_FEATURE_NOT_PRESENT = -8,
    // VK_ERROR_INCOMPATIBLE_DRIVER = -9,
    // VK_ERROR_TOO_MANY_OBJECTS = -10,
    // VK_ERROR_FORMAT_NOT_SUPPORTED = -11,
    // VK_ERROR_FRAGMENTED_POOL = -12,
    // VK_ERROR_UNKNOWN = -13,
    // VK_ERROR_OUT_OF_POOL_MEMORY = -1000069000,
    // VK_ERROR_INVALID_EXTERNAL_HANDLE = -1000072003,
    // VK_ERROR_FRAGMENTATION = -1000161000,
    // VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS = -1000257000,
    // VK_PIPELINE_COMPILE_REQUIRED = 1000297000,
    // VK_ERROR_SURFACE_LOST_KHR = -1000000000,
    // VK_ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001,
    // VK_SUBOPTIMAL_KHR = 1000001003,
    // VK_ERROR_OUT_OF_DATE_KHR = -1000001004,
    // VK_ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001,
    // VK_ERROR_VALIDATION_FAILED_EXT = -1000011001,
    // VK_ERROR_INVALID_SHADER_NV = -1000012000,
    // VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT = -1000158000,
    // VK_ERROR_NOT_PERMITTED_KHR = -1000174001,
    // VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT = -1000255000,
    // VK_THREAD_IDLE_KHR = 1000268000,
    // VK_THREAD_DONE_KHR = 1000268001,
    // VK_OPERATION_DEFERRED_KHR = 1000268002,
    // VK_OPERATION_NOT_DEFERRED_KHR = 1000268003,
    // VK_ERROR_OUT_OF_POOL_MEMORY_KHR = VK_ERROR_OUT_OF_POOL_MEMORY,
    // VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR = VK_ERROR_INVALID_EXTERNAL_HANDLE,
    // VK_ERROR_FRAGMENTATION_EXT = VK_ERROR_FRAGMENTATION,
    // VK_ERROR_NOT_PERMITTED_EXT = VK_ERROR_NOT_PERMITTED_KHR,
    // VK_ERROR_INVALID_DEVICE_ADDRESS_EXT = VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,
    // VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR = VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,
    // VK_PIPELINE_COMPILE_REQUIRED_EXT = VK_PIPELINE_COMPILE_REQUIRED,
    // VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT = VK_PIPELINE_COMPILE_REQUIRED,

#define DBG_ASSERT_VULKAN_MSG(result, errMsg )     \
  if (!(result == VK_SUCCESS))                     \
  {                                                \
                                                   \
    SDL_Log("ERROR::VULKAN[%d]: %s", result, errMsg); \
    DBG_ASSERT( false )                            \
  }

#define DBG_ASSERT_FT(result, errMsg)                                   \
  if (!(result == 0))                                                   \
  {                                                                     \
    SDL_Log("ERROR::FREETYPE:(%#.2X) %s", result, errMsg);                  \
    DBG_ASSERT(false)                                                   \
  }


namespace fc
{
  
   //TODO try removing static...etc.
   // ?? doesn't declaring this static here cause a new local func to be used in each TU
   // local callback functions
  VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void* pUserData);

  VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

  void DestroyDebugUtilsMessengerExt(VkInstance instance
                                     , VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

  bool areValidationLayersSupported(std::vector<const char*>& validationLayers);

  void displayFrameRate();
  
} // namespace fc _END_
