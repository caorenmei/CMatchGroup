# CMatch Library 实施进度

> 使用两级结构管理：`## 大步骤` 为一级，`### 步骤内部进度` 为二级。
> 每完成一个子任务，将其状态标记为 `[x]`；每完成一个大步骤，将一级标题状态更新为 `[x]`。

## [x] 步骤 1：Protobuf 配置与数据结构

### 步骤内部进度

- [x] 1.1 梳理 `config.md` 与 `data_structures.md` 中的 Protobuf 消息，确认字段、类型、包名。
- [x] 1.2 根据当前代码库状态微调需求与验收条件。
- [x] 1.3 创建 `proto/cmatch/config.proto` 与 `proto/cmatch/table.proto`。
- [x] 1.4 配置 CMake 以使用 `protobuf_generate_cpp` 编译 `.proto` 文件。
- [x] 1.5 构建项目，确认生成的 C++ 头文件/源文件可被 `src/` 正常引用。
- [x] 1.6 编写最小测试，验证 Protobuf 消息序列化/反序列化正确。
- [x] 1.7 应用 `clang-format` 与 `clang-tidy`，确保无新增警告。
- [x] 1.8 提交变更，更新本文件状态。

## [x] 步骤 2：核心接口定义

### 步骤内部进度

- [x] 2.1 梳理 `config.md`、`user_interface.md` 中的接口定义。
- [x] 2.2 根据当前代码库状态（已存在 Protobuf 生成代码）微调需求与验收条件。
- [x] 2.3 定义 `SeasonConfigInterface`（纯虚接口）。
- [x] 2.4 定义 `TicketEntityInterface` 与 `TicketEntityPtr`。
- [x] 2.5 定义 `TicketEntityManagerInterface`。
- [x] 2.6 提供基于内存的 mock 实现，供后续单元测试使用。
- [x] 2.7 编写接口行为测试，确认生命周期与引用正确。
- [x] 2.8 应用 `clang-format` 与 `clang-tidy`。
- [x] 2.9 提交变更，更新本文件状态。

## [x] 步骤 3：TicketManager 匹配算法

### 步骤内部进度

- [x] 3.1 梳理 `matching_algorithm.md` 与 `user_interface.md` 中 `TicketManager` 的职责。
- [x] 3.2 根据当前代码库状态（已存在接口与 mock）微调需求与验收条件。
- [x] 3.3 实现 `TicketManager` 构造函数与析构函数。
- [x] 3.4 实现 `BuildSeason`：加载完成后构建/修复赛季分组。
- [x] 3.5 实现 `NextSeason`：赛季切换与段位升降。
- [x] 3.6 实现 `AddTicket`：新凭据加入赛季分组。
- [x] 3.7 实现分组 ID 分配器、赛季判断、结算、分组、段位升降等私有辅助函数。
- [x] 3.8 编写单元测试覆盖：赛季判断、结算排名、分组 ID 唯一性、段位升降、随机分组。
- [x] 3.9 应用 `clang-format` 与 `clang-tidy`。
- [x] 3.10 提交变更，更新本文件状态。

## [x] 步骤 4：用户接口服务实现

### 步骤内部进度

- [x] 4.1 梳理 `user_interface.md` 中 `MatchGroupServiceImpl` 的所有 RPC 方法。
- [x] 4.2 根据当前代码库状态（已存在 `TicketManager` 与实体管理接口）微调需求与验收条件。
- [x] 4.3 定义 `MatchGroupServiceImpl` 纯虚接口。
- [x] 4.4 实现 `GetSeasonList`。
- [x] 4.5 实现 `SubmitTicket`。
- [x] 4.6 实现 `GetTicket`。
- [x] 4.7 实现 `RegisterSeason`。
- [x] 4.8 实现 `GetTicketList`。
- [x] 4.9 实现 `GetGroupMembers`。
- [x] 4.10 实现 `GetSettlementList`。
- [x] 4.11 实现 `RemoveSettlementList`。
- [x] 4.12 编写各 RPC 方法的单元测试。
- [x] 4.13 应用 `clang-format` 与 `clang-tidy`。
- [x] 4.14 提交变更，更新本文件状态。

## [x] 步骤 5：异常处理与数据修复

### 步骤内部进度

- [x] 5.1 梳理 `exception_handling.md` 中的实体数据修复与赛季时间修复规则。
- [x] 5.2 根据当前代码库状态（已存在服务实现）微调需求与验收条件。
- [x] 5.3 在 `BuildSeason` 中实现“当前时间不在赛季内”凭据的数据修复。
- [x] 5.4 实现赛季时间不一致时的修复逻辑（以配置时间为准）。
- [x] 5.5 编写异常场景测试：跨赛季凭据修复、时间戳修复、合服后分组 ID 冲突修复。
- [x] 5.6 应用 `clang-format` 与 `clang-tidy`。
- [x] 5.7 提交变更，更新本文件状态。

## [x] 步骤 6：集成测试与文档完善

### 步骤内部进度

- [x] 6.1 梳理所有规格文档，确认无遗漏的公共 API 与行为。
- [x] 6.2 根据当前代码库状态微调集成测试范围与文档更新清单。
- [x] 6.3 编写端到端集成测试：配置加载 → 提交凭据 → 报名 → 切换赛季 → 查询结算。
- [x] 6.4 更新项目根 `README.md`，补充构建说明、库功能简介与快速开始。
- [x] 6.5 为新增公共头文件补充 Doxygen 风格简体中文注释。
- [x] 6.6 全量构建：`cmake --build --preset ninja-debug`。
- [x] 6.7 全量测试：`ctest --preset ninja-debug`。
- [x] 6.8 全量格式与静态分析检查。
- [x] 6.9 提交变更，更新本文件状态。

## 备注

- 每个大步骤的状态 `[ ]` 必须在该步骤所有内部子任务都完成后才能改为 `[x]`。
- 若在某一步骤发现前置步骤的缺陷，应回退到对应步骤修复，并在本文件中记录阻塞原因。
