cmake_minimum_required(VERSION 3.14)

# 项目根配置
project(
  CMatchGroup
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "CMatchGroup 项目模板"
)

# 全局 C++ 标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 导出编译数据库，供 clang-tidy 等工具使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 标准安装路径
include(GNUInstallDirs)

# 包含目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 子目录
add_subdirectory(src)

# 启用 CTest 测试
enable_testing()
add_subdirectory(tests)

# 安装导出的 CMake 包配置
include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/CMatchGroupConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/CMatchGroupConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CMatchGroup"
)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/CMatchGroupConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
  "${PROJECT_BINARY_DIR}/CMatchGroupConfig.cmake"
  "${PROJECT_BINARY_DIR}/CMatchGroupConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CMatchGroup"
)
