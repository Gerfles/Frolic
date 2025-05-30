#pragma once


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstddef>
// TODO remove any vulkan initialization stuff from here and relocate if need be
#include <vulkan/vulkan.h>
#include <functional>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <fstream>
#include <vector>



// TODO don't forget the glm defines

namespace fc
{


// Index locations of queue families
  struct QueueFamilyIndices
  {
     int graphicsFamily = -1;
     int presentationFamily = -1;
      // check if queue family indices are valid
     bool isValid()
      {
        return graphicsFamily >= 0 && presentationFamily >= 0;
      }
  };


// TODO see if we can eliminate this from the utilities header
  // const std::vector<const char*> deviceExtensions = {
  //   VK_KHR_SWAPCHAIN_EXTENSION_NAME, };

      const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME};
  // More MacOS dependent stuff (I think)
  // "VK_KHR_portability_subset"};

  //  //TODO try removing static...etc.
  //  // ?? doesn't declaring this static here cause a new local func to be used in each TU
  //  // local callback functions
  // static VKAPI_ATTR VkBool32 VKAPI_CALL
  // debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  //               VkDebugUtilsMessageTypeFlagsEXT messageType,
  //               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  //               void* pUserData);

  // VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
  //                                       const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  //                                       const VkAllocationCallbacks* pAllocator,
  //                                       VkDebugUtilsMessengerEXT* pDebugMessenger);

  // void DestroyDebugUtilsMessengerExt(VkInstance instance,
  //                                    VkDebugUtilsMessengerEXT debugMessenger,
  //                                    const VkAllocationCallbacks* pAllocator);

  // bool areValidationLayersSupported(std::vector<const char*>& validationLayers);


  // TODO relocate all log/print utilities to a full class
  struct FcLog
  {
     // (EXAMPLE)
    // FcLog log1("log1", true);
     // log1 << "This is a log value: " << val;
    // log1.closeLogOutput();
     std::ofstream mFile;
     void openLogOutput(const std::string& filename, bool clearContents = false);
     //
     FcLog(const std::string& filename, bool clearContents = false)
      			{ openLogOutput(filename, clearContents); }
     std::ofstream& operator <<(std::string string);
     // variation if more complicated necessary
     void logOutput();
     void closeLogOutput();

  };


  static int logCount = 1;
  void fcLog(std::string header = "", int resetLogCount = -1);
  void calcFPS(uint64_t latestTick);
  void printMat(glm::mat4& mat);
  void printMat(glm::mat3& mat);
  void printVec(glm::vec3 vector, const char* title);
  void printVec(glm::vec4 vector, const char* title);
  // DELETE or move
  void initEnv();


  void printIOtable(std::vector<glm::ivec2>& input, std::function<int(glm::ivec2)> PFN_func);
  glm::mat4 perspective(float fovDegrees, float width, float height, float near, float far);
  glm::mat4 orthographic(float left, float right, float bottom
                         , float top, float near, float far);
  std::vector<char> readFile(const std::string& filename);


  template <typename TP>
  // std::time_t to_time_t(TP tp);
  time_t to_time_t(TP tp);
}
