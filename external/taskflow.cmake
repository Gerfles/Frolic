#taskflow

project(taskflow)

set(TASKFLOW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/taskflow/taskflow)

set(TASKFLOW_SOURCES
  ${TASKFLOW_DIR}/taskflow.hpp
  ${TASKFLOW_DIR}/core/async.hpp
  ${TASKFLOW_DIR}/core/async_task.hpp
  ${TASKFLOW_DIR}/core/atomic_notifier.hpp
  ${TASKFLOW_DIR}/core/declarations.hpp
  ${TASKFLOW_DIR}/core/executor.hpp
  ${TASKFLOW_DIR}/core/environment.hpp
  ${TASKFLOW_DIR}/core/error.hpp
  ${TASKFLOW_DIR}/core/flow_builder.hpp
  ${TASKFLOW_DIR}/core/freelist.hpp
  ${TASKFLOW_DIR}/core/graph.hpp
  ${TASKFLOW_DIR}/core/nonblocking_notifier.hpp
  ${TASKFLOW_DIR}/core/observer.hpp
  ${TASKFLOW_DIR}/core/runtime.hpp
  ${TASKFLOW_DIR}/core/semaphore.hpp
  ${TASKFLOW_DIR}/core/taskflow.hpp
  ${TASKFLOW_DIR}/core/task_group.hpp
  ${TASKFLOW_DIR}/core/task.hpp
  ${TASKFLOW_DIR}/core/topology.hpp
  ${TASKFLOW_DIR}/core/worker.hpp
  ${TASKFLOW_DIR}/core/wsq.hpp
  ${TASKFLOW_DIR}/algorithm/algorithm.hpp
  ${TASKFLOW_DIR}/algorithm/data_pipeline.hpp
  ${TASKFLOW_DIR}/algorithm/find.hpp
  ${TASKFLOW_DIR}/algorithm/for_each.hpp
  ${TASKFLOW_DIR}/algorithm/module.hpp
  ${TASKFLOW_DIR}/algorithm/partitioner.hpp
  ${TASKFLOW_DIR}/algorithm/pipeline.hpp
  ${TASKFLOW_DIR}/algorithm/reduce.hpp
  ${TASKFLOW_DIR}/algorithm/scan.hpp
  ${TASKFLOW_DIR}/algorithm/sort.hpp
  ${TASKFLOW_DIR}/algorithm/transform.hpp
)

# file(GLOB_RECURSE TASKFLOW_FILES ${CMAKE_CURRENT_SOURCE_DIR}/taskflow/taskflow/[a-z]*.hpp
# )

add_library(taskflow INTERFACE ${TASKFLOW_SOURCES})
target_compile_features(taskflow INTERFACE cxx_std_20)

# target_sources(taskflow INTERFACE ${TASKFLOW_FILES})
# target_sources(taskflow INTERFACE ${TASKFLOW_SOURCES})

target_include_directories(taskflow SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/taskflow)
# target_include_directories(taskflow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/taskflow/taskflow/)

# Disabling all complier warnings
if (FC_COMPILER_MSVC)
  target_compile_options(taskflow INTERFACE /w)
else()
  target_compile_options(taskflow INTERFACE -w)
endif()
