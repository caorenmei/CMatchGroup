# proto 目录构建配置

add_library(cmatch_proto)

add_library(cmatch::proto ALIAS cmatch_proto)

target_link_libraries(
  cmatch_proto
  PUBLIC
  protobuf::libprotobuf
)

target_include_directories(
  cmatch_proto
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

protobuf_generate(
  TARGET cmatch_proto
  PROTOS
    cmatch/config.proto
    cmatch/table.proto
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
