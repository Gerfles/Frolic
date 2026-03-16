//>--- utilities.cpp ---<//
#include "utilities.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_assert.hpp"
#include "core/fc_locator.hpp"
#include "core/log.hpp"
#include "platform.hpp"
#include "fc_debug.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
/* #include "vulkan/utility/ */
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstring>
#include <filesystem>
#include <iostream>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  void FcLog::openLogOutput(const std::string& filename, bool clearContents)
  {
    // open stream from give file and tell it to start reading from end
    if(clearContents)
    {
      mFile = std::ofstream(filename, std::ios::out | std::ios::trunc);
    }
    else
    {
      mFile = std::ofstream(filename, std::ios::out | std::ios::ate |std::ios::app);
    }

    // verify file opened
    if(!mFile.is_open())
    {
      throw std::runtime_error("Failed to open Log file: " + filename);
    }
  }

  // std::ostream& operator <<(std::string& output, const FcLog& log)
  std::ofstream& FcLog::operator <<(std::string string)
  {
    mFile.write(string.c_str(), string.size());

    return mFile;
  }

  void FcLog::logOutput()
  {
    std::string str{"Testing"};
    mFile.write(str.c_str(), 3);
  }


  void FcLog::closeLogOutput()
  {
    // End the provided file and close
    {
      mFile << "\n";
      mFile.close();
    }
  }


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


  //
  void printIOtable(std::vector<glm::ivec2>& input, std::function<int(glm::ivec2)> PFN_func)
  {
    // print columns
    std::cout << "[Input (x,y)]"
              << "\t[Output (x,y)]\n---------------------------------------\n";

    // print values
    for(size_t i = 0; i < input.size(); ++i)
    {
      int result = PFN_func(input[i]);
      std::cout << std::setw(4) << "(" << input[i].x << ", " << input[i].y << ")" << "   -->"
                << std::setw(6) << result << "\n";
      //
    }
    std::cout << std::endl;
  }


  void printMat(const glm::mat4& mat, std::string_view name)
  {
    std::cout << name << " matrix: \n";

    for (int i = 0; i < 4; i++)
    {
      std::cout << "| ";
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

  VkResult setObjectDebugName(VkDevice device, VkObjectType type, u64 handle, const char* name)
  {
    if (!name || !*name ||!vkSetDebugUtilsObjectNameEXT)
    {
      return VK_SUCCESS;
    }

    const VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT}
    , .objectHandle {handle}
    , .objectType {type}
    , .pObjectName {name}
    };

    return SetDebugUtilsObjectNameEXT(FcLocator::vkInstance(), device, &nameInfo);
  }


  //
  VkSemaphore createSemaphore(VkDevice device, const char* debugName)
  {
    const VkSemaphoreCreateInfo semaphoreInfo = {
      .sType {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}
    , .flags {0}
    };

    VkSemaphore semaphore {VK_NULL_HANDLE};

    // TODO &&
    VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
    VK_ASSERT(setObjectDebugName(device, VK_OBJECT_TYPE_SEMAPHORE, (u64)semaphore, debugName));
  }


  VkFence createFence(VkDevice device, bool isSignaled, const char* debugName)
  {
    const VkFenceCreateInfo fenceInfo {
      .sType {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}
    , .flags {isSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u}
    };

    VkFence fence {VK_NULL_HANDLE};

    VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &fence));

    VK_ASSERT(setObjectDebugName(device, VK_OBJECT_TYPE_FENCE, (u64)fence, debugName));
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

    //
    bool wantFileUpdateTime = false;
    if (wantFileUpdateTime)
    {
      std::cout << filename << " - Last Updated: " << std::asctime(std::localtime(&file_time));
      //  NOTE: Easier than above but requires C+23
      //std::println("Last Update: {}", fileTime);
    }


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
