#pragma once


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
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
  // TODO only the following printMat was updated, need to update the rest
  void printMat(const glm::mat4& mat, std::string_view name);
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
