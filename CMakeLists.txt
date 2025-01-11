# cmake_minimum_required(VERSION 3.11.0)

# #set the project name
# project(lve VERSION 1.0)

# #_____________________________________________________________________
# #configure file to version number into source code
# #----------------------------------------------------------------------
# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)





# #configuration types
#set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG "_DEBUG")
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
# SET(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "Configs" FORCE)
IF(DEFINED CMAKE_BUILD_TYPE AND CMAKE_VERSION VERSION_GREATER "3.1")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
ENDIF()

SET(CMAKE_DEBUG_POSTFIX "d")

# # Add compiler errors/warnings flags
# # target_compile_options(${PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:MSVC>:/W4 /WX>)
# # target_compile_options(${PROJECT_NAME} PRIVATE $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>)

# # add the core (engine) library
# add_subdirectory("${PROJECT_SOURCE_DIR}/src/core")

# #add_subdirectory("${PROJECT_SOURCE_DIR}/libs")

# # needed by CMake to find the SDL2 headers on OSX
# include_directories(/usr/local/include)

# # FreeType
# #include_directories (/usr/local/Cellar/freetype/2.11.1/include/freetype2)
# #include_directories (/usr/local/include/freetype2)

# # Add all c++ source files under the src directory
# # not reccommended to do it the way below (make and other programs won't be able to refactor)
# file(GLOB SOURCES "src/*.cpp")
# set(SOURCE_FILES ${SOURCES})# src/vulkan_extern.cpp src/vulkan_function.cpp)

# # add the executable
# add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# # add binary tree directory to paths to search for includes
# #target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}"
# #  "${PROJECT_SOURCE_DIR}/src/core")

# # Add all headers files under the include directory

# #comment out later to see if necessary
# #target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src/core")

# #_____________________________________________________________________
# # find all the SDL2 libraries
# #--------------------------------------------------------------------

# # Add SDL2 CMake modules
# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# # Add SDL2 library
# find_package(SDL2 REQUIRED)

# # Add SDL2_image library
# #find_package(SDL2_image REQUIRED)

# # Add SDL2_ttf library
# #find_package(SDL2_ttf REQUIRED)

# # Add SDL2_mixer library
# find_package(SDL2_mixer REQUIRED)

# # Add SDL2_net library
# #find_package(SDL2_net REQUIRED)

# # Add SDL2_gfx library
# #find_package(SDL2_gfx REQUIRED)
# #______________________________________________________________________

# #--------------------------------------------------------------------
# # find vulkan/utility libraries
# #____________________________________________________________________

# #include(FindVulkan)
# #set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} “${CMAKE_SOURCE_DIR}/cmake/Modules”)

# find_package(Vulkan REQUIRED)
#find_package(Freetype REQUIRED)



# #______________________________________________________________________


# # add header files
# set(FREETYPE_PATH /usr/local/Cellar/freetype/2.11.1/include/freetype2)

# #target_include_directories(${PROJECT_NAME} PRIVATE ${FREETYPE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR})
# #  ${Vulkan_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2} Core)
#   #  ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS}

#  #${Vulkan_INCLUDE_DIRS} ${FREETYPE_PATH} Core) #${FREETYPE_INCLUDE_DIRS} Core)
# #target_include_directories(${PROJECT_NAME} PRIVATE /Users/macbookpro/Programming/C++/vulkan/tutorials/lve_engine/hidden)

#   #${PROJECT_SOURCE_DIR}/src /Users/macbookpro/Programming/C++/vulkan/tutorials/lve_engine/libs)

# #______________________________________________________________________
# # add external libraries
# #file(GLOB LIBS_INCLUDES "/libs/*.h")
# #set (LIBS_SOURCES ${LIBS_INCLUDES})

# #add_library(External_Libs ${LIBS_SOURCES})
# if (NOT TINYOBJ_PATH)
#   set(TINYOBJ_PATH /Users/macbookpro/Programming/C++/vulkan/tutorials/lve_engine/external/tinyobjloader)
#   message(STATUS "TINYOBJ_PATH not specified in .env.cmake, using ${TINYOBJ_PATH}")
# endif()

# include_directories(external)
# target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}/src ${TINYOBJ_PATH})
# message(STATUS "CREATING BUILD FOR UNIX")

# # add libraries, including from our game engine
# TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2_MIXER_LIBRARIES}
#   #${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES}
#   ${Vulkan_LIBRARIES} ${FREETYPE_LIBRARIES} Core)# "${PROJECT_SOURCE_DIR}/src/core")




# # Copy assets to build folder
# #---------------------------------------------------
# #file(COPY graphics DESTINATION ${CMAKE_BINARY_DIR})
# #file(COPY text DESTINATION ${CMAKE_BINARY_DIR})
# file(COPY fonts DESTINATION ${CMAKE_BINARY_DIR})
# #file(COPY audio DESTINATION ${CMAKE_BINARY_DIR})
# file(COPY maps DESTINATION ${CMAKE_BINARY_DIR})
# file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
# file(COPY models DESTINATION ${CMAKE_BINARY_DIR})
# file(COPY textures DESTINATION ${CMAKE_BINARY_DIR})
# file(COPY shaders/compile_shaders.sh DESTINATION ${CMAKE_BINARY_DIR})


# # compile shaders
# set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
# file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag)

# foreach(SHADER ${SHADERS})
#   get_filename_component(FILENAME ${SHADER} NAME)
#   message("Shader: ${SHADER_DIR}")
#   add_custom_command(OUTPUT ${SHADER_DIR}/shaders/${FILENAME}.spv
#     COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_DIR}/../build/shaders/${FILENAME}.spv
#     DEPENDS ${SHADER}
#     COMMENT "Compiling ${FILENAME}")
#   list(APPEND SPV_SHADERS ${SHADER_DIR}/shaders/${FILENAME}.spv)
# endForeach()

# add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})


################################################################################################################
# NEW STYLE

include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.11.0)

set(NAME frolic)

#output the compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Choose the build program
 set(CMAKE_GENERATOR ninja)

# always compile for debugging
set(CMAKE_BUILD_TYPE DEBUG)

message(STATUS "using Cmake Generator: ${CMAKE_GENERATOR}")
if (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
    if (NOT MINGW_PATH)
      message(FATAL_ERROR "MINGW_PATH not set in .env.cmake")
    endif()
    set(USE_MINGW "True")
    set(CMAKE_C_COMPILER ${MINGW_PATH}/bin/gcc.exe)
    set(CMAKE_CXX_COMPILER  ${MINGW_PATH}/bin/g++.exe)
endif()

project(${NAME} VERSION 0.1.0)

# 1. Set VULKAN_SDK_PATH in .env.cmake to target specific vulkan version
if (DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include") # 1.1 Make sure this include path is correct
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib") # 1.2 Make sure lib path is correct
  set(Vulkan_FOUND "True")
else()
  find_package(Vulkan REQUIRED) # throws error if could not find Vulkan
  message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else()
	message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

# # needed by CMake to find the SDL2 headers on OSX
#include_directories(/usr/local/include)



#_____________________________________________________________________
# find all the required external libraries
#--------------------------------------------------------------------

# Add SDL2 CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Add SDL2 library
find_package(SDL2 REQUIRED)

# Add SDL2_image library
#find_package(SDL2_image REQUIRED)

# Add SDL2_ttf library
#find_package(SDL2_ttf REQUIRED)

# Add SDL2_mixer library
#find_package(SDL2_mixer REQUIRED)

# Add SDL2_net library
#find_package(SDL2_net REQUIRED)

# Add SDL2_gfx library
#find_package(SDL2_gfx REQUIRED)

# Add Assimp open asset importer libraries
find_package(assimp REQUIRED)

#find_package(imgui CONFIG REQUIRED)

find_package(Freetype REQUIRED)

#______________________________________________



# 2. Set GLFW_PATH in .env.cmake to target specific glfw
# if (DEFINED GLFW_PATH)
#   message(STATUS "Using GLFW path specified in .env")
#   set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
#   if (MSVC)
#     set(GLFW_LIB "${GLFW_PATH}/lib-vc2019") # 2.1 Update lib-vc2019 to use same version as your visual studio
#   elseif (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
#     message(STATUS "USING MINGW")
#     set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64") # 2.1 make sure matches glfw mingw subdirectory
#   endif()
# else()
#   find_package(glfw3 3.3 REQUIRED)
#   set(GLFW_LIB glfw)
#   message(STATUS "Found GLFW")
# endif()
# if (NOT GLFW_LIB)
# 	message(FATAL_ERROR "Could not find glfw library!")
# else()
# 	message(STATUS "Using glfw lib at: ${GLFW_LIB}")
# endif()

# add external file folder
add_subdirectory(external)
#include_directories(external)

# If TINYOBJ_PATH not specified in .env.cmake, try fetching from git repo
if (NOT TINYOBJ_PATH)
  message(STATUS "TINYOBJ_PATH not specified in .env.cmake, using external/tinyobjloader")
  set(TINYOBJ_PATH external/tinyobjloader)
endif()

if (assimp_FOUND)
  find_library(ASSIMP_LIBRARY NAMES ${ASSIMP_LIBRARIES} PATH ${ASSIMP_LIBRARY_DIRS})
   # find_library(ASSIMP_LIBRARY NAMES ${ASSIMP_LIBRARIES} PATH "usr/local/lib")
   # set(ASSIMP_LIBRARY "assimp")
   # add_library(${ASSIMP_LIBRARY} SHARED IMPORTED)
   # set_target_properties(${ASSIMP_LIBRARY} PROPERTIES IMPORTED_LOCATION "${ASSIMP_LIBRARY_DIRS}/libassimp.dylib")
   message(STATUS "Using Assimp lib at: ${ASSIMP_LIBRARIES}")

 endif(assimp_FOUND)

 message("freetype_FOUND: ${FREETYPE_INCLUDE_DIRS}")

# make sure we exclude emacs lock files with the following (lock files begin with #)
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/[a-z]*.cpp)

#file(GLOB_RECURSE EXTERNAL ${PROJECT_SOURCE_DIR}/external/[a-z]*.cpp)

add_executable(${PROJECT_NAME}
  ${SOURCES}
#  ${EXTERNAL}
)


# #specify the C++ standard
 # set(CMAKE_CXX_STANDARD 20)
 # set(CMAKE_CXX_STANDARD_REQUIRED true)
# set(CMAKE_CXX_COMPILER  $/usr/bin/c++)
 set(CMAKE_CXX_COMPILER  clang++)
 target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

# ?? in tutorial as:
#add_library(frolic_compiler_flags INTERFACE)
#target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")


if (WIN32)
  message(STATUS "CREATING BUILD FOR WINDOWS")

  if (USE_MINGW)
    target_include_directories(${PROJECT_NAME} PUBLIC
      ${MINGW_PATH}/include
    )
    target_link_directories(${PROJECT_NAME} PUBLIC
      ${MINGW_PATH}/lib
    )
  endif()

  target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/src
#    ${PROJECT_SOURCE_DIR}/external
    ${Vulkan_INCLUDE_DIRS}
    ${TINYOBJ_PATH}
    #${GLFW_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIR}
    ${GLM_PATH}
    )

  target_link_directories(${PROJECT_NAME} PUBLIC
    ${Vulkan_LIBRARIES}
    ${ASSIMP_LIBRARY}
  )

  target_link_libraries(${PROJECT_NAME} vulkan-1 fastgltf simdjson)

elseif (UNIX)

    message(STATUS "CREATING BUILD FOR LINUX")

    target_include_directories(${PROJECT_NAME} PUBLIC
      ${PROJECT_SOURCE_DIR}/src
 #     ${PROJECT_SOURCE_DIR}/external
      ${TINYOBJ_PATH}
      ${FREETYPE_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME}
      ${Vulkan_LIBRARIES}
      ${SDL2_LIBRARIES}
      ${SDL2_MIXER_LIBRARIES}
      ${ASSIMP_LIBRARIES}
      ${FREETYPE_LIBRARIES}
      fastgltf
      ImGui
      simdjson
      stb
    )

  endif()




  message(STATUS "Cmake C++ flags: ${CMAKE_CXX_FLAGS}")


# ############## Build SHADERS #######################

# # Find all vertex and fragment sources within shaders directory
# # taken from VBlancos vulkan tutorial
# # https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
# find_program(GLSL_VALIDATOR glslangValidator HINTS
#   ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
#   /usr/bin
#   /usr/local/bin
#   ${VULKAN_SDK_PATH}/Bin
#   ${VULKAN_SDK_PATH}/Bin32
#   $ENV{VULKAN_SDK}/Bin/
#   $ENV{VULKAN_SDK}/Bin32/
# )

# # get all .vert and .frag files in shaders directory
# file(GLOB_RECURSE GLSL_SOURCE_FILES
#   "${PROJECT_SOURCE_DIR}/shaders/*.frag"
#   "${PROJECT_SOURCE_DIR}/shaders/*.vert"
# )

# foreach(GLSL ${GLSL_SOURCE_FILES})
#   get_filename_component(FILE_NAME ${GLSL} NAME)
#   set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
#   add_custom_command(
#     OUTPUT ${SPIRV}
#     COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
#     DEPENDS ${GLSL})
#   list(APPEND SPIRV_BINARY_FILES ${SPIRV})
# endforeach(GLSL)

# add_custom_target(
#     Shaders
#     DEPENDS ${SPIRV_BINARY_FILES}
# )


# Copy assets to build folder
#---------------------------------------------------
#file(COPY graphics DESTINATION ${CMAKE_BINARY_DIR})
#file(COPY text DESTINATION ${CMAKE_BINARY_DIR})
file(COPY fonts DESTINATION ${CMAKE_BINARY_DIR})
#file(COPY audio DESTINATION ${CMAKE_BINARY_DIR})
file(COPY maps DESTINATION ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(COPY models DESTINATION ${CMAKE_BINARY_DIR})
file(COPY textures DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders/compile_shaders.sh DESTINATION ${CMAKE_BINARY_DIR})

#file(COPY) compile_commands.json DESTINAMTION ../

# compile shaders
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp)

foreach(SHADER ${SHADERS})
  get_filename_component(FILENAME ${SHADER} NAME)
  message("Shader: ${SHADER_DIR}")
  add_custom_command(OUTPUT ${SHADER_DIR}/shaders/${FILENAME}.spv
    COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} ${SHADER} -V -o ${SHADER_DIR}/../build/shaders/${FILENAME}.spv
    DEPENDS ${SHADER}
    COMMENT "Compiling ${FILENAME}")
  list(APPEND SPV_SHADERS ${SHADER_DIR}/shaders/${FILENAME}.spv)
endForeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})
