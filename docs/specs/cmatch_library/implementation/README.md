# CMatch Library 实施路线

> **范围：** 本文档为 `docs/specs/cmatch_library/` 规格说明创建可执行的实施路线。
> **约束：** 原始规格文档（README.md、config.md、data_structures.md、user_interface.md、matching_algorithm.md、exception_handling.md）保持只读，不做任何修改。
> **目标：** 将规格拆分为 6 个递进的大步骤，每个步骤拥有独立的需求文档与验收条件；整体进度由 `todolist.md` 管理。

## 代码库基线

- 构建系统：CMake + Ninja，预设 `ninja-debug`。
- 编译器：g++，C++20。
- 测试：Google Test（gtest）+ CTest。
- 代码规范：Google C++ Style Guide，`clang-format`、`clang-tidy`。
- 当前状态：项目仅包含示例代码 `src/cmatch/math_utils.*` 与对应测试；尚无 Protobuf、业务接口与算法实现。

## 目录结构约定

- 源码目录：`src/cmatch/`，与库命名空间一致。
- 测试目录：`tests/cmatch/`，与 `src/cmatch/` 保持一致的子目录结构。
- 所有新增测试文件、Mock 头文件均应放在 `tests/cmatch/` 下，而非 `tests/` 根目录。
- `tests/CMakeLists.txt` 按测试可执行文件逐个注册，源文件路径使用 `cmatch/<name>.cpp`。

| 步骤 | 主题 | 核心交付物 | 依赖 |
|---|---|---|---|
| 步骤 1 | Protobuf 配置与数据结构 | `.proto` 文件、生成的 C++ 代码、CMake 集成 | 无 |
| 步骤 2 | 核心接口定义 | `SeasonConfigInterface`、`TicketEntityInterface`、`TicketEntityManagerInterface` | 步骤 1 |
| 步骤 3 | TicketManager 匹配算法 | `TicketManager` 实现、单元测试 | 步骤 2 |
| 步骤 4 | 用户接口服务实现 | `MatchGroupServiceImpl` 及全部 RPC 方法 | 步骤 3 |
| 步骤 5 | 异常处理与数据修复 | 实体数据修复、赛季时间修复逻辑与测试 | 步骤 4 |
| 步骤 6 | 集成测试与文档完善 | 集成测试、`README.md` 与 API 文档更新 | 步骤 5 |

## 执行规则

1. **每个步骤开始前，必须重新核对当前代码库状态**，并据此微调该步骤的需求与验收条件。
2. **每个步骤结束时**，必须满足该步骤需求文档中的所有验收条件，方可进入下一步。
3. **所有提交**必须满足 `AGENTS.md` 中的提交前检查：
   - `cmake --build --preset ninja-debug` 构建成功。
   - `ctest --preset ninja-debug` 全部测试通过。
   - `clang-format` 已应用。
   - `clang-tidy` 静态分析无新增警告。
4. **进度同步**：每完成一个子任务，立即更新 `todolist.md`。

## 文档索引

- [todolist.md](todolist.md)：两级进度看板。
- [步骤 1：Protobuf 配置与数据结构](step-01-protobuf-config-data-structures.md)
- [步骤 2：核心接口定义](step-02-core-interfaces.md)
- [步骤 3：TicketManager 匹配算法](step-03-ticket-manager-algorithm.md)
- [步骤 4：用户接口服务实现](step-04-user-interface-service.md)
- [步骤 5：异常处理与数据修复](step-05-exception-handling-repair.md)
- [步骤 6：集成测试与文档完善](step-06-integration-tests-docs.md)

## 变更日志

| 日期 | 版本 | 说明 |
|---|---|---|
| 2026-06-28 | 1.1 | 统一测试目录结构为 `tests/cmatch/`，与 `src/cmatch/` 保持一致。 |
| 2026-06-28 | 1.0 | 根据当前代码库基线创建实施路线。 |
