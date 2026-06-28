# 测试目录构建配置

find_package(GTest REQUIRED)
include(GoogleTest)

# 示例测试可执行文件
add_executable(
  test_math_utils
  cmatch/test_math_utils.cpp
)

target_link_libraries(
  test_math_utils
  PRIVATE
  cmatch
  GTest::gtest_main
)

# 自动发现测试并注册到 CTest
gtest_discover_tests(test_math_utils)

# Protobuf 消息测试
add_executable(
  test_protobuf_messages
  cmatch/test_protobuf_messages.cpp
)

target_link_libraries(
  test_protobuf_messages
  PRIVATE
  cmatch
  GTest::gtest_main
)

gtest_discover_tests(test_protobuf_messages)

# 核心接口测试
add_executable(
  test_core_interfaces
  cmatch/test_core_interfaces.cpp
)

target_link_libraries(
  test_core_interfaces
  PRIVATE
  cmatch
  GTest::gtest_main
)

gtest_discover_tests(test_core_interfaces)

# TicketManager 算法测试
add_executable(
  test_ticket_manager
  cmatch/test_ticket_manager.cpp
)

target_link_libraries(
  test_ticket_manager
  PRIVATE
  cmatch
  GTest::gtest_main
)

gtest_discover_tests(test_ticket_manager)
