# 步骤 1：Protobuf 配置与数据结构

## 目标

将 `docs/specs/cmatch_library/config.md` 与 `docs/specs/cmatch_library/data_structures.md` 中的 Protobuf 定义落地为可编译、可测试的 `.proto` 文件，并集成到 CMake 构建流程中。

## 原始规格引用

- `config.md`：
  - `GradeInfo`：段位信息。
  - `SeasonInfo`：赛事信息。
  - `SeasonTime`：赛季时间。
  - 包名：`cmatch.config`。
- `data_structures.md`：
  - `Ticket`：凭据信息。
  - `SeasonGroup`：赛季分组。
  - `SeasonSettlement`：结算信息。
  - 包名：`cmatch.table`。

## 当前代码库基线

- 项目已使用 CMake + Ninja，C++20。
- 尚无 Protobuf 依赖与 `.proto` 文件。
- `src/cmatch/` 目录已存在，用于存放库头文件与实现。
- 根 `CMakeLists.txt` 与 `src/CMakeLists.txt` 已配置为构建 `cmatch` 静态库。

## 开始前的校准清单

在正式启动本步骤前，请逐项确认并可能微调需求：

1. **Protobuf 版本与查找方式**：检查系统已安装的 Protobuf 版本（`protoc --version`），确认 CMake 使用 `find_package(Protobuf REQUIRED)` 还是 `pkg_check_modules`。若版本与预设不一致，调整 CMake 配置。
2. **生成代码目录**：确认生成的 `.pb.h` / `.pb.cc` 放在 `build/` 内还是 `src/` 内。当前基线倾向于放在构建目录，避免污染源码。
3. **包名与命名空间**：确认规格中的 `cmatch.config` 与 `cmatch.table` 在 C++ 生成代码中映射为 `cmatch::config` 与 `cmatch::table`。
4. **字段类型映射**：确认 `uint64`、`uint32`、`float`、`bool`、`map`、`repeated` 在 Protobuf 3 中的默认值行为是否满足业务要求（例如未设置的 `uint32` 默认 0，与规格中“配置为 0 表示不生效”一致）。
5. **是否保留 `cmatch.lib` 包的消息到后续步骤**：`user_interface.md` 中 `MatchGroupService` 的请求/响应消息属于服务接口，建议放到步骤 4 实现，避免本步骤一次性引入过多消息。

## 需求

### R1.1 创建 Protobuf 文件

在 `proto/cmatch/config.proto` 中定义：

```protobuf
syntax = "proto3";
package cmatch.config;

message GradeInfo {
    uint32 grade = 1;
    uint32 prev_grade = 2;
    uint32 next_grade = 3;
    uint32 group_size = 4;
    uint32 promote_rank = 5;
    float promote_rank_percent = 6;
    uint32 demote_rank = 7;
    float demote_rank_percent = 8;
}

message SeasonInfo {
    uint32 type = 1;
    map<uint32, GradeInfo> grades = 2;
    uint64 initial_score = 3;
    uint64 min_score = 4;
    uint32 score_attr_id = 5;
    bool reset_score = 6;
    uint32 initial_grade = 7;
}

message SeasonTime {
    uint64 begin_time = 1;
    uint64 end_time = 2;
}
```

在 `proto/cmatch/table.proto` 中定义：

```protobuf
syntax = "proto3";
package cmatch.table;

message Ticket {
    uint64 id = 1;
    uint32 zone_id = 2;
    uint32 auto_id = 3;
    map<uint32, uint64> attributes = 4;
    repeated uint32 registrations = 5;
    map<uint32, SeasonGroup> seasons = 6;
    map<uint32, SeasonSettlement> settlements = 7;
}

message SeasonGroup {
    uint32 type = 1;
    uint64 begin_time = 2;
    uint64 end_time = 3;
    uint32 grade = 4;
    uint64 group_id = 5;
}

message SeasonSettlement {
    uint32 type = 1;
    uint64 begin_time = 2;
    uint64 end_time = 3;
    uint32 grade = 4;
    uint64 group_id = 5;
    uint32 rank = 6;
    float rank_percent = 7;
    uint64 score = 8;
}
```

### R1.2 CMake 集成

- 在根 `CMakeLists.txt` 中启用 `find_package(Protobuf REQUIRED)`。
- 使用 `protobuf_generate_cpp` 或 `protobuf_generate` 生成代码。
- 将生成的 `.pb.cc` 加入 `cmatch` 库目标，并链接 `protobuf::libprotobuf`。
- 将 `proto/` 目录通过 `target_include_directories` 暴露给库使用者（生成代码中的 include 路径通常需要 `${CMAKE_CURRENT_BINARY_DIR}`）。
- 调整 `tests/CMakeLists.txt`，使测试目录结构与 `src/` 保持一致。示例：

```cmake
# 测试目录构建配置

find_package(GTest REQUIRED)
include(GoogleTest)

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
```

后续步骤新增的测试可执行文件均按 `tests/cmatch/<name>.cpp` 组织。

### R1.3 测试验证

- 创建 `tests/cmatch/test_protobuf_messages.cpp`。
- 至少包含：
  - `GradeInfo` 字段赋值与读取。
  - `SeasonInfo` 的 `grades` map 插入与读取。
  - `Ticket` 的序列化与反序列化。
  - 验证未设置的 `uint32` 字段默认值为 0。

## 验收条件

- [ ] `cmake --build --preset ninja-debug` 成功，无 Protobuf 相关编译错误。
- [ ] `ctest --preset ninja-debug` 中新增测试全部通过。
- [ ] `clang-format -i proto/cmatch/*.proto src/cmatch/*.h src/cmatch/*.cpp tests/cmatch/*.cpp` 执行后无文件内容变更。
- [ ] `clang-tidy` 对新增 C++ 文件无新增警告。
- [ ] 生成的 `.pb.h` 可被 `#include "cmatch/config.pb.h"` 与 `#include "cmatch/table.pb.h"` 引用（或实际生成的路径一致）。

## 预计交付文件

- 新增：
  - `proto/cmatch/config.proto`
  - `proto/cmatch/table.proto`
  - `tests/cmatch/test_protobuf_messages.cpp`
- 修改：
  - `CMakeLists.txt`
  - `src/CMakeLists.txt`
  - `tests/CMakeLists.txt`

## 风险与注意事项

- **Protobuf 依赖缺失**：若构建环境未安装 Protobuf 或版本过低，会导致配置失败。需在开始本步骤前安装 `libprotobuf-dev` / `protobuf-compiler`（或对应包管理器命令）。
- **生成代码污染源码树**：应确保生成文件位于 `build/` 目录，避免被 Git 跟踪。
- **map 字段默认行为**：`map<uint32, GradeInfo>` 在反序列化空 map 时行为正确，但业务逻辑中不应依赖其排序。
- **与步骤 2 的衔接**：本步骤完成后，`cmatch::config::SeasonInfo`、`cmatch::config::SeasonTime`、`cmatch::table::Ticket` 等类型必须可被步骤 2 的接口直接引用。

## 完成后的下一步

进入 [步骤 2：核心接口定义](step-02-core-interfaces.md)。
