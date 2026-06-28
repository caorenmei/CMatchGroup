# CMatch Library 实施路线

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `docs/specs/cmatch_library/` 中的规格说明落地为可构建、可测试的 CMatch Library，覆盖 Protobuf 定义、核心接口、匹配算法、用户接口服务、异常处理与集成测试。

**Architecture：** 采用分层架构：底层为 Protobuf 生成的数据结构；中间层为纯虚接口（配置、实体、实体管理器）与 `TicketManager` 算法核心；上层为 `MatchGroupServiceImpl` 服务实现。各层通过接口解耦，便于单元测试时注入内存 mock。

**Tech Stack：** CMake 3.14+、Ninja、g++ C++20、Google Test、CTest、Protobuf、clang-format、clang-tidy

## Global Constraints

- C++ 标准：C++20（`CMAKE_CXX_STANDARD 20`）
- 构建预设：`ninja-debug`
- 测试框架：Google Test（gtest）
- 数据交换格式：Protobuf 3
- 所有文档、注释、提交信息使用简体中文
- 公共 API 头文件必须包含 Doxygen 风格简体中文注释
- 提交前必须完成：`cmake --build --preset ninja-debug` 构建成功、`ctest --preset ninja-debug` 全部通过、`clang-format` 已应用、`clang-tidy` 无新增警告
- 原始规格文档（`docs/specs/cmatch_library/*.md`）保持只读，实施细节写入 `docs/specs/cmatch_library/implementation/`

---

## 说明

本计划为“实施路线”型计划，将完整实现拆分为 6 个递进的大步骤。每个大步骤的详细需求、验收条件、预计交付文件、风险与注意事项均保存在独立的步骤文档中；整体进度由 `todolist.md` 管理。

详细文档位置：`docs/specs/cmatch_library/implementation/`

- [实施路线总览](docs/specs/cmatch_library/implementation/README.md)
- [进度看板](docs/specs/cmatch_library/implementation/todolist.md)
- [步骤 1：Protobuf 配置与数据结构](docs/specs/cmatch_library/implementation/step-01-protobuf-config-data-structures.md)
- [步骤 2：核心接口定义](docs/specs/cmatch_library/implementation/step-02-core-interfaces.md)
- [步骤 3：TicketManager 匹配算法](docs/specs/cmatch_library/implementation/step-03-ticket-manager-algorithm.md)
- [步骤 4：用户接口服务实现](docs/specs/cmatch_library/implementation/step-04-user-interface-service.md)
- [步骤 5：异常处理与数据修复](docs/specs/cmatch_library/implementation/step-05-exception-handling-repair.md)
- [步骤 6：集成测试与文档完善](docs/specs/cmatch_library/implementation/step-06-integration-tests-docs.md)

---

## 高层任务清单

### Task 1: Protobuf 配置与数据结构

**Files:** 详见 `step-01-protobuf-config-data-structures.md`

**Interfaces:**
- Produces: `cmatch::config::GradeInfo`、`cmatch::config::SeasonInfo`、`cmatch::config::SeasonTime`
- Produces: `cmatch::table::Ticket`、`cmatch::table::SeasonGroup`、`cmatch::table::SeasonSettlement`

- [ ] **Step 1.1:** 创建 `proto/cmatch/config.proto` 与 `proto/cmatch/table.proto`
- [ ] **Step 1.2:** 在 CMake 中集成 Protobuf 生成
- [ ] **Step 1.3:** 编写 `tests/cmatch/test_protobuf_messages.cpp` 验证消息
- [ ] **Step 1.4:** 构建、测试、格式化、静态分析
- [ ] **Step 1.5:** 提交

### Task 2: 核心接口定义

**Files:** 详见 `step-02-core-interfaces.md`

**Interfaces:**
- Consumes: Protobuf 类型（Task 1）
- Produces: `SeasonConfigInterface`、`TicketEntityInterface`、`TicketEntityManagerInterface`、内存 Mock

- [ ] **Step 2.1:** 定义配置与实体接口头文件
- [ ] **Step 2.2:** 提供 `tests/cmatch/mock_ticket_entity_manager.h`
- [ ] **Step 2.3:** 编写接口行为测试
- [ ] **Step 2.4:** 构建、测试、格式化、静态分析
- [ ] **Step 2.5:** 提交

### Task 3: TicketManager 匹配算法

**Files:** 详见 `step-03-ticket-manager-algorithm.md`

**Interfaces:**
- Consumes: Protobuf 类型（Task 1）、核心接口（Task 2）
- Produces: `TicketManager`（`BuildSeason`、`NextSeason`、`AddTicket`）

- [ ] **Step 3.1:** 实现 `TicketManager` 框架与辅助函数
- [ ] **Step 3.2:** 实现赛季判断、结算、分组、段位升降
- [ ] **Step 3.3:** 实现 `BuildSeason`、`NextSeason`、`AddTicket`
- [ ] **Step 3.4:** 编写单元测试
- [ ] **Step 3.5:** 构建、测试、格式化、静态分析
- [ ] **Step 3.6:** 提交

### Task 4: 用户接口服务实现

**Files:** 详见 `step-04-user-interface-service.md`

**Interfaces:**
- Consumes: Protobuf 类型（Task 1）、核心接口（Task 2）、`TicketManager`（Task 3）
- Produces: `MatchGroupServiceImpl`、`MatchGroupServiceDefault`

- [ ] **Step 4.1:** 创建 `proto/cmatch/lib.proto`
- [ ] **Step 4.2:** 定义 `MatchGroupServiceImpl` 纯虚接口
- [ ] **Step 4.3:** 实现 `MatchGroupServiceDefault` 全部 RPC 方法
- [ ] **Step 4.4:** 编写服务单元测试
- [ ] **Step 4.5:** 构建、测试、格式化、静态分析
- [ ] **Step 4.6:** 提交

### Task 5: 异常处理与数据修复

**Files:** 详见 `step-05-exception-handling-repair.md`

**Interfaces:**
- Consumes: `TicketManager`（Task 3）、服务实现（Task 4）
- Produces: 完善的 `BuildSeason` 异常修复逻辑

- [ ] **Step 5.1:** 实现赛季时间修复
- [ ] **Step 5.2:** 实现不在当前赛季凭据的数据修复
- [ ] **Step 5.3:** 实现 `NextSeason` 积分重置
- [ ] **Step 5.4:** 编写异常处理测试
- [ ] **Step 5.5:** 构建、测试、格式化、静态分析
- [ ] **Step 5.6:** 提交

### Task 6: 集成测试与文档完善

**Files:** 详见 `step-06-integration-tests-docs.md`

**Interfaces:**
- Consumes: 全部前述实现
- Produces: 端到端集成测试、更新后的 README、完整 API 注释

- [ ] **Step 6.1:** 编写端到端集成测试
- [ ] **Step 6.2:** 更新项目根 `README.md`
- [ ] **Step 6.3:** 补充公共头文件 Doxygen 注释
- [ ] **Step 6.4:** 全量构建、测试、格式化、静态分析
- [ ] **Step 6.5:** 更新实施路线与 todolist 状态
- [ ] **Step 6.6:** 提交

---

## Self-Review

**1. Spec coverage:**
- `config.md` 中的 `GradeInfo`、`SeasonInfo`、`SeasonTime` → Task 1
- `data_structures.md` 中的 `Ticket`、`SeasonGroup`、`SeasonSettlement` → Task 1
- `user_interface.md` 中的 `SeasonConfigInterface` → Task 2
- `user_interface.md` 中的 `TicketEntityInterface`、`TicketEntityManagerInterface` → Task 2
- `user_interface.md` 中的 `TicketManager` → Task 3
- `user_interface.md` 中的 `MatchGroupService` 消息与服务实现 → Task 4
- `matching_algorithm.md` 中的赛季判断、结算、分组 ID、段位升降、赛季分组 → Task 3
- `exception_handling.md` 中的实体数据修复与赛季时间修复 → Task 5
- 文档与注释要求 → Task 6

无遗漏。

**2. Placeholder scan:**
- 无 TBD/TODO/"实现 later"。
- 详细实现代码在每个步骤文档中给出。
- 验收条件均为可测试项。

**3. Type consistency:**
- `SeasonConfigInterface`、`TicketEntityInterface`、`TicketEntityManagerInterface` 的签名与规格一致。
- `TicketManager` 的方法签名与规格一致（随机数引擎参数为根据当前代码库的微调整）。
- Protobuf 包名与命名空间一致：`cmatch.config`、`cmatch.table`、`cmatch.lib`。

---

**Plan complete. Detailed step documents saved to `docs/specs/cmatch_library/implementation/`. Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per step, review between steps, fast iteration

**2. Inline Execution** - Execute steps in this session using executing-plans, batch execution with checkpoints

**Which approach?**
