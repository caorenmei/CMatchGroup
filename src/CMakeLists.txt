# src 目录构建配置

# 主静态库
add_library(
  mylib
  math_utils.cpp
)

target_include_directories(
  mylib
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# 安装目标
install(
  TARGETS mylib
  EXPORT CMatchGroupTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# 安装导出的目标
install(
  EXPORT CMatchGroupTargets
  FILE CMatchGroupTargets.cmake
  NAMESPACE CMatchGroup::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CMatchGroup"
)

# 安装公共头文件
install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
