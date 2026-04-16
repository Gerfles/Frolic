//>--- utilities.cpp ---<//
#include "utilities.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_assert.hpp"
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
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
  void populateImageMemoryBarrier(VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkImage image, VkImageMemoryBarrier2& barrier)
  {
    // *-*-*-*-*-*-*-*-*-*-*-   TODO EXTRAPOLATE TO BUILD OP   *-*-*-*-*-*-*-*-*-*-*- //
    // Cache the write Memory Barrier that will transition swapchain image to transfer destination
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    // Queue family to transition to/from - IGNORED means don't bother transferring to a different queue
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    //
    barrier.subresourceRange.aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                                           || oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                          ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    // TODO We are currently targeting all layers and mipmap levels and this could potentially be better
    // implemented by targeting less
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    //
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    //
    barrier.image = image;

    // Determine Proper access flags and pipeline stages for image layout transition
    // Documentation: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
    auto determineAccessFlags = [&] (VkImageLayout layout, VkPipelineStageFlags2& stage, VkAccessFlags2& access)
    /* auto determineAccessFlags = [&] (VkImageLayout layout, VkImageMemoryBarrier2 arrier) */
     {
       switch (layout)
       {
           case VK_IMAGE_LAYOUT_UNDEFINED:
             stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
             access = VK_ACCESS_NONE;
             break;
           case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
             stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
             access = VK_ACCESS_NONE;
             break;
           case VK_IMAGE_LAYOUT_PREINITIALIZED:
             stage = VK_PIPELINE_STAGE_HOST_BIT;
             access = VK_ACCESS_HOST_WRITE_BIT;
             break;
           case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
             stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
             access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                      | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
             break;
           case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
             stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                     | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
             access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
             break;
           case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
             stage = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
             access = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;;
             break;
           case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
             stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                     | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
             access = VK_ACCESS_SHADER_READ_BIT
                      | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;;
             break;
           case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
             stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
             access = VK_ACCESS_TRANSFER_READ_BIT;
             break;
           case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
             stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
             access = VK_ACCESS_TRANSFER_WRITE_BIT;
             break;
           default:
             FC_ASSERT(false && "Image layout Unknown or Improperly set");
             stage = 0;
             access = 0;
             break;
       };
     };

    // First set the flags for pipeline stage and access mode for the old layout (source)
    determineAccessFlags(oldLayout, barrier.srcStageMask, barrier.srcAccessMask);

    // Next set the flags for pipeline stage and access mode for the new layout (destination)
    determineAccessFlags(newLayout, barrier.dstStageMask, barrier.dstAccessMask);
  }


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
    return VK_SUCCESS;


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


    // TODO get working
    /* return SetDebugUtilsObjectNameEXT(FcLocator::vkInstance(), device, &nameInfo); */
    // FIXME this should be done and sent to the VMA allocator (a list of functions), see lvk
    return vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
  }


  // TODO combine with below,
  // TODO utilize sychronization2 extension to let us signal what state in the command buffer queue
  // should be waited on before semaphore is signaled (i.e. COLOR_ATTACHMENT_OUTPUT) <- this should
  // be the default and even having that set can optimize the result regardless of the fact that
  // COLOR_.._OUTPUT is the final in a pipeline
  VkSemaphore createSemaphore(VkDevice device, const char* debugName)
  {
    const VkSemaphoreCreateInfo semaphoreInfo = {
      .sType {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}
    /* , .pNext = VK_NULL_HANDLE */
    , .flags {0}
    };

    VkSemaphore semaphore {VK_NULL_HANDLE};

    // TODO &&
    VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
    VK_ASSERT(setObjectDebugName(device, VK_OBJECT_TYPE_SEMAPHORE, (u64)semaphore, debugName));

    return semaphore;
  }


  VkSemaphore createTimelineSemaphore(VkDevice device, u64 initialValue, const char* debugName)
  {
    const VkSemaphoreTypeCreateInfo typeInfo = {
      .sType {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO}
    , .semaphoreType {VK_SEMAPHORE_TYPE_TIMELINE}
    , .initialValue {initialValue}
    };

    const VkSemaphoreCreateInfo semaphoreInfo = {
      .sType {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}
    , .pNext {&typeInfo}
    , .flags {0}
    };

    VkSemaphore semaphore {VK_NULL_HANDLE};
    VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
    VK_ASSERT(setObjectDebugName(device, VK_OBJECT_TYPE_SEMAPHORE, (u64)semaphore, debugName));

    return semaphore;
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

    return fence;
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
