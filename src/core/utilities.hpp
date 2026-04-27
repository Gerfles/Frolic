//>--- utilities.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/platform.hpp"
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <functional>
#include <fstream>
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


// TODO add std::exchange utility to utilities

namespace fc
{
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
  //
  void populateImageMemoryBarrier(VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkImage image, VkImageMemoryBarrier2& barrier);
  //
  void calcFPS(uint64_t latestTick);
  // TODO only the following printMat was updated, need to update the rest
  void printMat(const glm::mat4& mat, std::string_view name);
  void printMat(glm::mat3& mat);
  void printVec(glm::vec3 vector, const char* title);
  void printVec(glm::vec4 vector, const char* title);

  void printIOtable(std::vector<glm::ivec2>& input, std::function<int(glm::ivec2)> PFN_func);

  std::vector<char> readFile(const std::string& filename);

  VkResult setObjectDebugName(VkInstance instance, VkDevice device, VkObjectType type, u64 handle, const char* name);
  //
  VkSemaphore createSemaphore(VkDevice device, const char* debugName);
  //
  VkSemaphore createTimelineSemaphore(VkDevice device, u64 initialValue, const char* debugName);
  // TODO could default is Signaled to false
  VkFence createFence(VkDevice device, bool isSignaled, const char* debugName);
  //
  VkDeviceSize getAlignedSize(u64 initialSize, u64 alignment);



  template <typename TP>
  // std::time_t to_time_t(TP tp);
  time_t to_time_t(TP tp);
}
