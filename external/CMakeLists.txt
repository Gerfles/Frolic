# Frolic dependencies

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# option(FC_USE_IMGUI "Use ImGui" ON)
# if (FC_USE_IMGUI)
#   include(ImGui)
# endif ()

include(fastgltf)
include(ImGui)
include(stb)
include(ktx)
include(tlsf)
