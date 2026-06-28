# CMatch Library

CMatch Library 是一个基于 C++20 的赛季制匹配分组库，采用 CMake + Ninja 构建系统，遵循 Google C++ Style Guide，并使用 Google Test 进行单元测试。

## 适用场景

- 赛季制匹配分组：根据凭据（Ticket）的段位（Grade）与积分（Score）将参与者分组。
- 段位升降：根据赛季结算规则对参与者进行段位晋升或降级。
- 赛季结算：在赛季结束时对凭据进行结算与排行（Rank）计算。

## 构建要求

- CMake >= 3.14
- Ninja
- g++（支持 C++20）
- Google Test（gtest）
- Protobuf
- clang-format
- clang-tidy

## 快速开始

```bash
# 配置 Debug 构建
cmake --preset ninja-debug

# 构建项目
cmake --build --preset ninja-debug

# 运行测试
ctest --preset ninja-debug
```

## 项目结构

```
src/cmatch/                          # 库源码
├── season_config_interface.h        # 赛季配置接口
├── ticket_entity_interface.h        # 凭据实体接口
├── ticket_entity_manager_interface.h # 凭据实体管理器接口
├── ticket_manager.h / .cpp          # 匹配算法实现
├── match_group_service_impl.h       # 用户接口服务实现
├── match_group_service_default.h / .cpp # 默认服务实现
└── math_utils.h / .cpp              # 工具函数（示例）

tests/cmatch/                        # 测试代码
├── test_protobuf_messages.cpp       # Protobuf 消息测试
├── test_core_interfaces.cpp         # 核心接口测试
├── test_ticket_manager.cpp          # 匹配算法测试
├── test_match_group_service.cpp     # 用户接口服务测试
├── test_exception_handling.cpp      # 异常处理测试
├── test_cmatch_integration.cpp      # 集成测试
├── test_math_utils.cpp              # 工具函数测试
└── mock_ticket_entity_manager.h     # Mock 管理器

proto/cmatch/                        # Protobuf 定义
├── config.proto                     # 赛季配置定义
├── lib.proto                        # 数据结构定义
└── table.proto                      # 表格数据定义

docs/specs/cmatch_library/           # 规格文档
├── README.md                        # 规格总览
├── config.md                        # 配置规格
├── data_structures.md               # 数据结构规格
├── user_interface.md                # 用户接口规格
├── matching_algorithm.md            # 匹配算法规格
└── exception_handling.md            # 异常处理规格

docs/specs/cmatch_library/implementation/ # 实施路线
├── README.md                        # 实施路线总览
├── todolist.md                      # 进度看板
└── step-01-protobuf-config-data-structures.md  # 各步骤文档...
```

## 文档索引

- [规格文档](docs/specs/cmatch_library/README.md)
- [实施路线](docs/specs/cmatch_library/implementation/README.md)

## 代码规范

- 遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)。
- 使用 `clang-format` 格式化代码：
  ```bash
  find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
  ```
- 使用 `clang-tidy` 进行静态分析：
  ```bash
  find src/cmatch -name "*.cpp" | xargs clang-tidy -p build/debug
  ```

## 贡献指南

- 默认分支为 `main`，禁止直接推送，请通过 Pull Request 合并代码。
- 提交信息遵循 Conventional Commits 规范，描述使用简体中文。
- 提交前请确保构建、测试、格式检查和静态分析均通过。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
