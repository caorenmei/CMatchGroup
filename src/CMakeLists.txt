# src 目录构建配置

# 主静态库
add_library(
  mylib
  math_utils.cpp
)

target_include_directories(
  mylib
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
