# src/cmatch 目录构建配置

add_library(
  cmatch
  match_group_service_impl.cpp
  ticket_manager.cpp
)

add_library(cmatch::cmatch ALIAS cmatch)

target_include_directories(
  cmatch
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(
  cmatch
  PUBLIC
  cmatch::proto
)
