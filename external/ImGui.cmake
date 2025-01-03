# ImGui

project(ImGui)

add_library(ImGui OBJECT)

target_compile_features(ImGui PRIVATE cxx_std_20)

# The needed freetype header ft2build.h seems to be found only when calling aux_source_directory();
aux_source_directory(imgui IMGUI_SRC)

# file(GLOB_RECURSE ...  could work if file folder is better cleaned out
  file(GLOB IMGUI_FILES ${IMGUI_SRC}
  imgui/misc/cpp/*.cpp
  imgui/*.h
  imgui/*.cpp
  imgui/backends/*.h
  imgui/backends/*.cpp
#  implot/*.cpp
#  implot/*.h
)

target_sources(ImGui PRIVATE ${IMGUI_FILES})

target_include_directories(ImGui SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
#  ${CMAKE_CURRENT_SOURCE_DIR}/implot
)
