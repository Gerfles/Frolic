# simdjson

project(simdjson)

add_library(simdjson OBJECT)

target_compile_features(simdjson PRIVATE cxx_std_20)

set (SIMDJSON_FILES
  simdjson/simdjson.cpp
  simdjson/simdjson.h
)

target_sources(simdjson PRIVATE ${SIMDJSON_FILES})

target_include_directories(simdjson SYSTEM PUBLIC simdjson)

# Disable all compiler warnings
