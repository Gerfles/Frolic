#include "fc_debug.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <unordered_set>

namespace fc
{

  VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
                , VkDebugUtilsMessageTypeFlagsEXT messageType
                , const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData
                , void* pUserData)
  {
     //   The first parameter specifies the severity of the message, which is one of the following flags:

     // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
     // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource
     // VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error, but very					 likely a bug in you r application
     //  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes
     // The values of this enumeration are set up in such a way that you can use a comparison operation to check if a message is equal or wors e compared to some level of severity, for example:

     // if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
     //     // Message is important enough to show
     // }
     // The messageType parameter can have the following values:

     // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened that is unrelated to the specification or performance
     // VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the specification or indicates a possible mistake
     // VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan
     // The pCallbackData parameter refers to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message itself, with the most important members being:

     // pMessage: The debug message as a null-terminated string
     // pObjects: Array of Vulkan object handles related to the message
     // objectCount: Number of objects in array
     // Finally, the pUserData parameter contains a pointer that was specified during the setup of the callback and allows you to pass your o wn data to it.

     // The callback returns a boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted. If the  callback returns true, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error. This is normally only used to test the v alidation layers themselves, so you should always return VK_FALSE.

     //?? not sure why this is needed, seems it already prints callback message
     // probably prints to some Log or something ?? research cerr output
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

     //  The callback returns a boolean that indicates if the Vulkan call that triggered the validation
     //  layer message should be aborted. If the callback returns true, then the call is aborted with the
     //  VK_ERROR_VALIDATION_FAILED_EXT error. This is normally only used to test the validation layers
     //  themselves, so you should always return VK_FALSE.
    return VK_FALSE;
  }



  VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger)
  {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                          "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  void DestroyDebugUtilsMessengerExt(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks* pAllocator)
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                           "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
      func(instance, debugMessenger, pAllocator);
    }
  }



  bool areValidationLayersSupported(std::vector<const char*>& validationLayers)
  {
     // make a list of all vulkan layers available to us
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::unordered_set<std::string> requiredLayers(validationLayers.begin(), validationLayers.end());

     // make sure our required layers are covered by the layers found availablelayers
    for (const auto& layer : availableLayers)
    {
      requiredLayers.erase(layer.layerName);
    }

     // return true if all the all the required layers were found in vulkans available layers
    return requiredLayers.empty();
  }


  void displayFrameRate()
  {
  }

} // namespace fc _END_
