# src 目录构建配置

add_library(
  cmatch
  cmatch/math_utils.cpp
  cmatch/match_group_service_default.cpp
  cmatch/ticket_manager.cpp
)

add_library(cmatch::cmatch ALIAS cmatch)

target_include_directories(
  cmatch
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
  cmatch
  PUBLIC
  cmatch::proto
)
