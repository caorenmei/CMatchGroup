# 测试目录构建配置

find_package(GTest REQUIRED)
include(GoogleTest)

# 示例测试可执行文件
add_executable(
  test_math_utils
  test_math_utils.cpp
)

target_link_libraries(
  test_math_utils
  PRIVATE
  cmatch
  GTest::gtest_main
)

# 自动发现测试并注册到 CTest
gtest_discover_tests(test_math_utils)
