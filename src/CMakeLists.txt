# src 目录构建配置

add_library(
  cmatch
  cmatch/math_utils.cpp
)

add_library(cmatch::cmatch ALIAS cmatch)

target_include_directories(
  cmatch
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
