//>--- fc_debug.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
/* #include <SDL_log.h> */
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdarg>
#include <cstdio>
#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


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

}// --- dprintf (_) --- (END)




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
