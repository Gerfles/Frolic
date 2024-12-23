#include "utilities.hpp"

// - FROLIC ENGINE -

// - EXTERNAL LIBRARIES -
#include <vulkan/vulkan.h>
// - STD LIBRARIES -
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <iostream>


// DELETE
#include <filesystem>
#include <format>

namespace fc
{
   // static VKAPI_ATTR VkBool32 VKAPI_CALL
   // debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   //               VkDebugUtilsMessageTypeFlagsEXT messageType,
   //               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
   //               void* pUserData)
   // {

   //    //?? not sure why this is needed, seems it already prints callback message
   //    // probably prints to some Log or something ?? research cerr output
   //   std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

   //   return VK_FALSE;
   // }

   // VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
   //                                       const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
   //                                       const VkAllocationCallbacks* pAllocator,
   //                                       VkDebugUtilsMessengerEXT* pDebugMessenger)
   // {
   //   auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
   //                                                                         "vkCreateDebugUtilsMessengerEXT");
   //   if (func != nullptr)
   //   {
   //     return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
   //   }

   //   return VK_ERROR_EXTENSION_NOT_PRESENT;
   // }

   // void DestroyDebugUtilsMessengerExt(VkInstance instance,
   //                                    VkDebugUtilsMessengerEXT debugMessenger,
   //                                    const VkAllocationCallbacks* pAllocator)
   // {
   //   auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
   //                                                                          "vkDestroyDebugUtilsMessengerEXT");
   //   if (func != nullptr)
   //   {
   //     func(instance, debugMessenger, pAllocator);
   //   }
   // }

   // bool areValidationLayersSupported(std::vector<const char*>& validationLayers)
   // {
   //    // make a list of all vulkan layers available to us
   //   uint32_t layerCount;
   //   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
   //   std::vector<VkLayerProperties> availableLayers(layerCount);
   //   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

   //   std::unordered_set<std::string> requiredLayers(validationLayers.begin(), validationLayers.end());

   //    // make sure our required layers are covered by the layers found availablelayers
   //   for (const auto& layer : availableLayers)
   //   {
   //     requiredLayers.erase(layer.layerName);
   //   }

   //    // return true if all the all the required layers were found in vulkans available layers
   //   return requiredLayers.empty();
   // }

  //
  void fcLog(std::string header, int resetLogCount)
  {
    if (resetLogCount >= 0)
    {
      logCount = resetLogCount;
    }
    if (header != "")
    {
      std::cout << "Logging Funtion: " << header
                << std::right << ":\tLog Count: "<< logCount++ << std::endl;
    }
    else
    {
      std::cout << "\t\t fcLog() called: " << std::right
                << "\tLog Count:" << logCount++ << std::endl;
    }
  }



  void printMat(glm::mat4& mat)
  {
    std::cout << "matrix elements: \n";

    for (int i = 0; i < 4; i++)
    {
      std::cout << "\n| ";
      for (int j = 0; j < 4; j++)
      {
        std::cout << mat[j][i] << " ";
      }
      std::cout << "|\n";
    }
    std::cout << std::endl;
  }

  void printMat(glm::mat3& mat)
  {
    std::cout << "matrix elements: \n";

    for (int i = 0; i < 3; i++)
    {
      std::cout << "\n| ";
      for (int j = 0; j < 3; j++)
      {
        std::cout << mat[j][i] << " ";
      }
      std::cout << "|\n";
    }
    std::cout << std::endl;
  }


  void printVec(glm::vec3 vector, const char* title)
  {
    std::cout << title <<" vector:\n[";

    for (int i = 0; i < 3; i++)
    {
      std::cout << vector[i] << ", ";

      if (i == 2)
      {
        std::cout << "\b\b]";
      }
    }
    std::cout << std::endl;
  }


  void printVec(glm::vec4 vector, const char* title)
  {
    std::cout << title <<" vector:\n[";

    for (int i = 0; i < 4; i++)
    {
      std::cout << vector[i] << ", ";

      if (i == 2)
      {
        std::cout << "\b\b]";
      }
    }
    std::cout << std::endl;
  }

   // TODO set all environment variables from here
  void initEnv()
  {
     // transfer and use soln from
     // https://stackoverflow.com/questions/8591762/ifdef-debug-with-cmake-independent-from-platform
#ifndef NDEBUG
    std::printf("\n---- DEBUG ----\n");
#else
    std::printf("\n---- RELEASE ----\n");
#endif

// Good method to differentiate between different compositors in within linux
    if (strcmp(secure_getenv("XDG_SESSION_TYPE"), "wayland") == 0) {
      printf("We're on wayland.\n");
    } else if (strcmp(secure_getenv("XDG_SESSION_TYPE"), "x11") == 0) {
      printf("We're on X11.\n");
    } else {
      printf("NOT IDENTIFIED ?\n");
    }
  }



  std::vector<char> readFile(const std::string& filename)
  {
     // open stream from given file ('ate' tells stream to start reading from end (AT End))
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

     // make sure file stream sucsessfully opened
    if (!file.is_open())
    {
      throw std::runtime_error("Failed to open file: " + filename);
    }

     // *-*-*-*-*-   SANITY CHECK TO VERIFY SHADERS WERE COMPILED RECENTLY   *-*-*-*-*- //
     // TODO probably best to print in cmake instead but keep this template for future
     // read back from the filesystem
    std::filesystem::file_time_type last_write = std::filesystem::last_write_time(filename);

    using namespace std::chrono;
    auto st = time_point_cast<system_clock::duration>(last_write
                                                      - std::filesystem::file_time_type::clock::now()
                                                      + system_clock::now());
    std::time_t file_time =  system_clock::to_time_t(st);

    std::cout << filename << " - Last Updated: " << std::asctime(std::localtime(&file_time));

     //  Easier than above but requires C+23
     //std::println("Last Update: {}", fileTime);

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   CONTINUE LOADING   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // get current read position (since this is at the end of file) to get file size
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> fileBuffer(fileSize);

     // move read position to start of file
    file.seekg(0);

     // read all the file data into the file buffer
    file.read(fileBuffer.data(), fileSize);

     // close stream
    file.close();

     // TODO re-write this function to accept a std::vector<char> pointer so nothing is copied
    return fileBuffer;
  }

} // namespace fc - END
