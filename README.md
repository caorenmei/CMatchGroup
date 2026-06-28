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
- clangd（作为 C/C++ 语言服务器，并承担静态分析）

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
├── CMakeLists.txt                   # 库构建配置
├── season_config_interface.h        # 赛季配置接口
├── ticket_entity_interface.h        # 凭据实体接口
├── ticket_entity_manager_interface.h # 凭据实体管理器接口
├── ticket_manager.h / .cpp          # 匹配算法实现
└── match_group_service_impl.h / .cpp # 用户接口服务实现

tests/cmatch/                        # 测试代码
├── CMakeLists.txt                   # 测试构建配置
├── test_protobuf_messages.cpp       # Protobuf 消息测试
├── test_core_interfaces.cpp         # 核心接口测试
├── test_ticket_manager.cpp          # 匹配算法测试
├── test_match_group_service.cpp     # 用户接口服务测试
├── test_exception_handling.cpp      # 异常处理测试
├── test_cmatch_integration.cpp      # 集成测试
└── mock_ticket_entity_manager.h     # Mock 管理器

proto/cmatch/                        # Protobuf 定义
├── config.proto                     # 赛季配置定义
├── lib.proto                        # 服务消息定义
└── table.proto                      # 数据结构定义

docs/specs/cmatch_library/           # 规格文档
├── design/                          # 设计文档
│   ├── README.md                    # 规格总览
│   ├── config.md                    # 配置规格
│   ├── data_structures.md           # 数据结构规格
│   ├── user_interface.md            # 用户接口规格
│   ├── matching_algorithm.md        # 匹配算法规格
│   └── exception_handling.md        # 异常处理规格
└── implementation/                  # 实施路线
    ├── README.md                    # 实施路线总览
    ├── todolist.md                  # 进度看板
    └── step-01-protobuf-config-data-structures.md  # 各步骤文档...
```

## 文档索引

- [规格文档](docs/specs/cmatch_library/design/README.md)
- [实施路线](docs/specs/cmatch_library/implementation/README.md)

## 代码规范

- 遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)。
- 使用 `clang-format` 格式化代码：
  ```bash
  find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
  ```
- 使用 `clangd` 进行静态分析：
  - 确保已执行 `cmake --preset ninja-debug` 生成 `build/debug/compile_commands.json`。
  - 在编辑器中打开任意 `.cpp` 或 `.h` 文件，`clangd` 会自动加载 [`.clangd`](.clangd) 配置并运行 ClangTidy 检查。
  - 也可通过命令行手动检查单个文件：
    ```bash
    clangd --check=src/cmatch/ticket_manager.cpp
    ```

## 贡献指南

- 默认分支为 `main`，禁止直接推送，请通过 Pull Request 合并代码。
- 提交信息遵循 Conventional Commits 规范，描述使用简体中文。
- 提交前请确保构建、测试、格式检查和静态分析均通过。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
