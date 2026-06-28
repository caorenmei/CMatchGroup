# tests/cmatch 目录构建配置

find_package(GTest REQUIRED)
include(GoogleTest)

# Protobuf 消息测试
add_executable(
  test_protobuf_messages
  test_protobuf_messages.cpp
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
  test_core_interfaces.cpp
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
  test_ticket_manager.cpp
)

target_link_libraries(
  test_ticket_manager
  PRIVATE
  cmatch
  GTest::gtest_main
)

gtest_discover_tests(test_ticket_manager)

# 匹配分组服务测试
add_executable(
  test_match_group_service
  test_match_group_service.cpp
)

target_link_libraries(
  test_match_group_service
  PRIVATE
  cmatch
  GTest::gtest_main
)

gtest_discover_tests(test_match_group_service)

# 异常处理与数据修复测试
add_executable(
  test_exception_handling
  test_exception_handling.cpp
)

target_link_libraries(
  test_exception_handling
  PRIVATE
  cmatch
  GTest::gtest_main
)

gtest_discover_tests(test_exception_handling)

# 端到端与边界场景集成测试
add_executable(
  test_cmatch_integration
  test_cmatch_integration.cpp
)

target_link_libraries(
  test_cmatch_integration
  PRIVATE
  cmatch
  GTest::gtest_main
)

gtest_discover_tests(test_cmatch_integration)
