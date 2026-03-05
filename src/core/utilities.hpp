//>--- utilities.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <functional>
#include <fstream>
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  enum class FeatureType : uint8_t
  {
    ValidationLayer,
    InstanceExtension,
    DeviceExtension,
  };


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

  void calcFPS(uint64_t latestTick);
  // TODO only the following printMat was updated, need to update the rest
  void printMat(const glm::mat4& mat, std::string_view name);
  void printMat(glm::mat3& mat);
  void printVec(glm::vec3 vector, const char* title);
  void printVec(glm::vec4 vector, const char* title);

  void printIOtable(std::vector<glm::ivec2>& input, std::function<int(glm::ivec2)> PFN_func);
  std::vector<char> readFile(const std::string& filename);

  const bool areFeaturesSupported(std::vector<const char*>& extensionsOrLayers,
                                  FeatureType type,
                                  VkPhysicalDevice device = nullptr) noexcept;

  template <typename TP>
  // std::time_t to_time_t(TP tp);
  time_t to_time_t(TP tp);
}
