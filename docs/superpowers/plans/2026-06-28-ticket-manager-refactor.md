# TicketManager 重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: 使用 `superpowers:executing-plans`（本会话手动执行）或 `superpowers:subagent-driven-development`。

**Goal:** 实施 `docs/specs/cmatch_library/refactor/2026-06-28-ticket-manager-refactor.md` 中的 TicketManager 重构。

**Architecture:** 通过 `SeasonConfigInterface` 统一配置入口；`Initialize` 仅修复当前赛季内凭据时间并保留原分组；建立 `<season_type, grade>`、`group_id`、未满索引；`WriteSeasonGroups` 作为索引维护唯一入口；`NextSeason`/`AddTicket` 复用统一填充逻辑；`GetGroupMembers` 改为索引查询。

**Tech Stack:** C++20, CMake + Ninja, Google Test, protobuf, clang-format, clangd/ClangTidy。

## Global Constraints

- C++ 标准：C++20
- 构建系统：CMake + Ninja（`CMakePresets.json`）
- 代码规范：Google C++ Style Guide，`clang-format` 格式化
- 静态分析：`clangd` 内置 `ClangTidy` 无新增警告
- 测试框架：Google Test，运行器 `CTest`
- 所有文档、注释、提交信息使用简体中文；代码标识符保持英文
- 提交前必须完成：`cmake --build --preset ninja-debug`、`ctest --preset ninja-debug`、格式化、静态分析

---

# TicketManager 重构计划

> **目标：** 精简 `TicketManager` 公共接口、优化赛季初始化流程、删除无用配置字段，并建立分组索引以加速 `GetGroupMembers` 查询。

---

## 全局约束

- C++ 标准：C++20
- 构建系统：CMake + Ninja（`CMakePresets.json`）
- 代码规范：Google C++ Style Guide，`clang-format` 格式化
- 静态分析：`clangd` 内置 `ClangTidy` 无新增警告
- 测试框架：Google Test，运行器 `CTest`
- 所有文档、注释、提交信息使用简体中文；代码标识符保持英文
- 提交前必须完成：`cmake --build --preset ninja-debug`、`ctest --preset ninja-debug`、格式化、静态分析

---

## 文件映射

| 文件 | 操作 | 说明 |
|------|------|------|
| `proto/cmatch/lib.proto` | 修改 | `RemoveSettlementListReq.settlement_ids` 由 `uint64` 改为 `uint32` |
| `proto/cmatch/config.proto` | 修改 | 删除 `SeasonInfo.min_score` |
| `src/cmatch/ticket_manager.h` | 修改 | `BuildSeason` 重命名为 `Initialize`；`AddTicket` 参数与 `Initialize` 对齐；新增分组索引与查询接口 |
| `src/cmatch/ticket_manager.cpp` | 修改 | 实现参数变更、流程优化、索引维护；`NextSeason` 与 `AddTicket` 使用统一填充/索引 |
| `src/cmatch/match_group_service_impl.cpp` | 修改 | `GetGroupMembers` 改为调用 `TicketManager` 的 group_id 查询接口 |
| `tests/cmatch/test_ticket_manager.cpp` | 修改 | 更新 `BuildSeason`/`AddTicket` 调用；删除 `min_score` 相关断言；新增索引与填充测试 |
| `tests/cmatch/test_match_group_service.cpp` | 修改 | 更新 `BuildSeason` 调用（如有）；删除 `min_score` 相关代码 |
| `docs/specs/cmatch_library/design/config.md` | 修改 | 删除 `min_score` 字段说明 |
| `docs/specs/cmatch_library/design/user_interface.md` | 修改 | 更新 `TicketManager` 接口签名与 `settlement_ids` 类型 |
| `docs/specs/cmatch_library/design/matching_algorithm.md` | 修改 | 补充索引维护与分组查询说明 |

---

## 重构项清单

- [ ] **R1:** `proto/cmatch/lib.proto` 中 `RemoveSettlementListReq.settlement_ids` 改为 `repeated uint32`
- [ ] **R2:** `src/cmatch/ticket_manager.h` 中 `BuildSeason` 重命名为 `Initialize`
- [ ] **R3:** `Initialize` 参数改为 `const SeasonConfigInterface&`，不再传递 `season_info`、`season_time` 与 `season_type`；内部通过 `config.GetTypes()` 遍历所有类型，并委托私有方法逐个类型完成初始化
- [ ] **R4:** `Initialize` 中仅在凭据上的时间位于当前赛季时才修复赛季时间
- [ ] **R5:** `Initialize` 中位于当前赛季内的凭据不再重新分组
- [ ] **R6:** `proto/cmatch/config.proto` 删除 `SeasonInfo.min_score` 字段
- [ ] **R7:** `TicketManager` 建立并维护分组索引：`<season_type, grade> -> group_ids`、`group_id -> ticket_ids`、未满组队索引 `<season_type, grade> -> group_ids`
- [ ] **R8:** `TicketManager` 提供按 `group_id` 获取凭据 ID 列表的公共接口，并在 `MatchGroupServiceImpl::GetGroupMembers` 中使用
- [ ] **R9:** `NextSeason` 使用 `WriteSeasonGroups` 新签名维护索引
- [ ] **R10:** `AddTicket` 参数与 `Initialize` 一致，内部按 `config.GetTypes()` 分发调用；优先填入同段位未满旧分组，否则创建新分组
- [ ] **R11:** 同步更新 `docs/specs/cmatch_library/design/` 下设计文档

---

## Task 1: 协议字段类型精简

**Files:**
- 修改：`proto/cmatch/lib.proto:157`
- 修改：`proto/cmatch/config.proto:33-34`
- 测试：`tests/cmatch/test_match_group_service.cpp`

**Interfaces:**
- `lib::RemoveSettlementListReq.settlement_ids` 类型由 `uint64` 变为 `uint32`。
- `config::SeasonInfo` 移除 `min_score` 字段。

- [ ] **Step 1.1: 修改 `lib.proto`**

```protobuf
message RemoveSettlementListReq {
  // 凭据 ID
  uint64 id = 1;
  // 结算 ID 列表，实际对应赛季类型
  repeated uint32 settlement_ids = 2;
}
```

- [ ] **Step 1.2: 修改 `config.proto`**

删除以下两行：
```protobuf
  // 最小积分
  uint64 min_score = 4;
```

并将后续字段编号前移（或保持 proto3 删除字段习惯，直接移除，不重新编号其余字段；推荐保持其余字段原编号以避免破坏旧数据）：
- `score_attr_id` 保持 `5`
- `reset_score` 保持 `6`
- `initial_grade` 保持 `7`

> 注意：保留已删除字段的编号，避免未来复用。

- [ ] **Step 1.3: 清理测试中的 `min_score` 引用**

在 `tests/cmatch/test_ticket_manager.cpp:54` 和 `tests/cmatch/test_match_group_service.cpp:55` 中删除：
```cpp
info.set_min_score(0);
```

- [ ] **Step 1.4: 重新生成 protobuf 并构建**

```bash
cmake --build --preset ninja-debug
```

- [ ] **Step 1.5: 运行测试**

```bash
ctest --preset ninja-debug
```

- [ ] **Step 1.6: 提交**

```bash
git add proto/cmatch/lib.proto proto/cmatch/config.proto \
  tests/cmatch/test_ticket_manager.cpp tests/cmatch/test_match_group_service.cpp
git commit -m "refactor(proto): 结算 ID 列表改为 uint32 并删除未使用的 min_score"
```

---

## Task 2: 重命名 `BuildSeason` 为 `Initialize`

**Files:**
- 修改：`src/cmatch/ticket_manager.h:40`
- 修改：`src/cmatch/ticket_manager.cpp:58`
- 测试：`tests/cmatch/test_ticket_manager.cpp`（所有 `tm.BuildSeason(...)` 调用）
- 文档：`docs/specs/cmatch_library/design/user_interface.md`

**Interfaces:**
- 公共方法名由 `BuildSeason` 改为 `Initialize`，语义更准确地表达"实体加载完成后初始化"。
- 本任务保持原参数不变：`Initialize(const config::SeasonInfo& season_info, const config::SeasonTime& season_time, std::uint32_t now_time, std::mt19937& rng)`。

- [ ] **Step 2.1: 修改头文件声明**

```cpp
/// @brief 实体加载完成后，遍历每个赛事类型调用一次
///
/// 根据当前时间将凭据分为“在当前赛季内”与“不在当前赛季内”两类：
/// - 在当前赛季内：按当前段位重新分组。
/// - 不在当前赛季内：按原分组结算、计算新段位、优先填入当前赛季同段位未满分组。
/// 同时修复凭据上的赛季时间与配置不一致的问题。
/// @param[in] season_info 赛季信息
/// @param[in] season_time 赛季时间
/// @param[in] now_time 当前时间
/// @param[in,out] rng 随机数引擎
void Initialize(const config::SeasonInfo& season_info,
                const config::SeasonTime& season_time, std::uint32_t now_time,
                std::mt19937& rng);
```

- [ ] **Step 2.2: 修改实现文件函数签名**

```cpp
void TicketManager::Initialize(const config::SeasonInfo& season_info,
                               const config::SeasonTime& season_time,
                               std::uint32_t now_time, std::mt19937& rng) {
  // 原 BuildSeason 实现逻辑保持不变
}
```

- [ ] **Step 2.3: 更新测试调用**

将 `tests/cmatch/test_ticket_manager.cpp` 中所有 `tm.BuildSeason(info, time, ...)` 改为 `tm.Initialize(info, time, ...)`。

- [ ] **Step 2.4: 更新设计文档**

在 `docs/specs/cmatch_library/design/user_interface.md` 中，将 `TicketManager` 的 `BuildSeason` 方法名改为 `Initialize`。

- [ ] **Step 2.5: 构建、测试并提交**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

```bash
git add src/cmatch/ticket_manager.h src/cmatch/ticket_manager.cpp \
  tests/cmatch/test_ticket_manager.cpp docs/specs/cmatch_library/design/user_interface.md
git commit -m "refactor(ticket_manager): BuildSeason 重命名为 Initialize"
```

---

## Task 3: `Initialize` 参数改为 `const SeasonConfigInterface&`

**Files:**
- 修改：`src/cmatch/ticket_manager.h:40-42`
- 修改：`src/cmatch/ticket_manager.cpp:58-162`
- 测试：`tests/cmatch/test_ticket_manager.cpp`
- 文档：`docs/specs/cmatch_library/design/user_interface.md`

**Interfaces:**
- `Initialize(const SeasonConfigInterface& config, std::uint32_t now_time, std::mt19937& rng)`
- 新增私有方法：`void InitializeSeasonType(const SeasonConfigInterface& config, std::uint32_t season_type, std::uint32_t now_time, std::mt19937& rng)`
- 内部通过 `config.GetInfo(season_type, season_info)` 与 `config.GetTime(season_type, season_time)` 获取具体配置。

- [ ] **Step 3.1: 修改头文件**

```cpp
void Initialize(const SeasonConfigInterface& config, std::uint32_t now_time,
                std::mt19937& rng);

private:
/// @brief 逐个赛事类型完成初始化
/// @param[in] config 赛季配置接口
/// @param[in] season_type 赛季类型
/// @param[in] now_time 当前时间
/// @param[in,out] rng 随机数引擎
void InitializeSeasonType(const SeasonConfigInterface& config,
                          std::uint32_t season_type, std::uint32_t now_time,
                          std::mt19937& rng);
```

- [ ] **Step 3.2: 修改实现文件顶部逻辑**

```cpp
void TicketManager::Initialize(const SeasonConfigInterface& config,
                               std::uint32_t now_time, std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());

  for (std::uint32_t season_type : config.GetTypes()) {
    InitializeSeasonType(config, season_type, now_time, rng);
  }
}

void TicketManager::InitializeSeasonType(
    const SeasonConfigInterface& config, std::uint32_t season_type,
    std::uint32_t now_time, std::mt19937& rng) {
  config::SeasonInfo season_info;
  config::SeasonTime season_time;
  if (!config.GetInfo(season_type, season_info) ||
      !config.GetTime(season_type, season_time)) {
    return;
  }

  // ... 后续逻辑见 Task 4 ...
}
```

- [ ] **Step 3.3: 更新测试**

将测试中 `tm.Initialize(info, time, ...)` 调用统一改为传入 `MockSeasonConfig` 实例：

```cpp
MockSeasonConfig config(info);
config.SetTime(time);
// ...
tm.Initialize(config, now_time, rng);
```

- [ ] **Step 3.4: 构建并测试**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

- [ ] **Step 3.5: 提交**

```bash
git add src/cmatch/ticket_manager.h src/cmatch/ticket_manager.cpp \
  tests/cmatch/test_ticket_manager.cpp docs/specs/cmatch_library/design/user_interface.md
git commit -m "refactor(ticket_manager): Initialize 改为接收 SeasonConfigInterface 并委托私有方法逐类型初始化"
```

---

## Task 4: 优化 `Initialize` 流程（时间修复与赛季内分组保留）

**Files:**
- 修改：`src/cmatch/ticket_manager.cpp`
- 测试：`tests/cmatch/test_ticket_manager.cpp`

**Interfaces:**
- 行为变更：
  - 修复赛季时间的前提：凭据上的时间 `IsInSeason(now_time, group_time)` 为真。
  - 在当前赛季内的凭据：不重新分组，保留原 `group_id`；凭据进入 `in_season_entities` 集合，用于后续统一填充未满分组（见 Task 6）。
  - 不在当前赛季内的凭据：按原分组结算并重新计算段位，准备进入统一填充/新建分组流程。

- [ ] **Step 4.1: 仅在当前赛季内才修复时间**

将原修复时间逻辑从无条件执行改为条件执行：

```cpp
// 读取凭据上的赛季时间与段位，再修复时间
config::SeasonTime group_time;
group_time.set_begin_time(it->second.begin_time());
group_time.set_end_time(it->second.end_time());
const std::uint32_t grade = it->second.grade();
const std::uint64_t group_id = it->second.group_id();

if (IsInSeason(now_time, group_time)) {
  // 修复赛季时间：以配置为准
  table::SeasonGroup& mutable_group =
      (*ticket.mutable_seasons())[season_type];
  if (mutable_group.begin_time() != season_time.begin_time() ||
      mutable_group.end_time() != season_time.end_time()) {
    mutable_group.set_begin_time(season_time.begin_time());
    mutable_group.set_end_time(season_time.end_time());
    entity_manager_.SetDirty(ticket.id());
  }
  in_season_by_grade[grade].push_back(entity);
} else {
  out_of_season_by_group[group_id].push_back(entity);
}
```

- [ ] **Step 4.2: 收集当前赛季内的凭据，不再按段位重新分组**

删除对 `in_season_by_grade` 调用 `FormGroupSlots` 的循环。改为将当前赛季内的凭据按段位收集到 `in_season_by_grade` 中，供 Task 6 统一填充未满分组时使用；其 `group_id` 保持不变。

```cpp
// 在当前赛季内的凭据：保留原分组，仅按段位收集
for (auto& [grade, tickets] : in_season_by_grade) {
  // 不做任何重新分组操作
  (void)grade;
  (void)tickets;
}
```

- [ ] **Step 4.3: 调整 `grade_slots` 的用途**

`grade_slots` 在 Task 6 中用于统一填入：
- 不在当前赛季内、结算并计算新段位后的凭据。
- 无赛季数据的新凭据。
- 当前赛季内可填入同段位未满旧分组的凭据（利用索引快速定位未满分组）。

位于当前赛季内且无法填入旧分组的凭据，将在 `grade_slots` 中创建新分组。

- [ ] **Step 4.4: 更新或新增测试**

新增测试验证：
- 凭据时间在当前赛季内时，`Initialize` 不会变更其 `group_id`。
- 凭据时间不在当前赛季内时，`Initialize` 会生成新的 `group_id`。
- 凭据时间在当前赛季内时，`Initialize` 会修复时间为配置的 `season_time`。
- 凭据时间不在当前赛季内时，时间不修复（保持凭据原时间用于结算）。

示例测试：

```cpp
TEST(TicketManagerTest, InSeasonTicketsKeepGroupId) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  MockSeasonConfig config(info);
  config.SetTime(time);
  std::mt19937 rng(12345);

  auto entity = manager.GetOrCreateEntity(1, 1);
  SetScore(entity, info.score_attr_id(), 100);
  auto& group = (*entity->GetData().mutable_seasons())[1];
  group.set_type(1);
  group.set_begin_time(0);
  group.set_end_time(100);
  group.set_grade(1);
  group.set_group_id(12345);

  tm.Initialize(config, 50, rng);

  EXPECT_EQ(entity->GetData().seasons().at(1).group_id(), 12345);
}
```

- [ ] **Step 4.5: 构建、测试并提交**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

```bash
git add src/cmatch/ticket_manager.cpp tests/cmatch/test_ticket_manager.cpp
git commit -m "refactor(ticket_manager): Initialize 仅在当前赛季内修复时间且不重新分组"
```

---

## Task 5: 建立分组索引结构并保证与凭据数据一致

**Files:**
- 修改：`src/cmatch/ticket_manager.h`
- 修改：`src/cmatch/ticket_manager.cpp`
- 测试：`tests/cmatch/test_ticket_manager.cpp`

**Interfaces:**
- 新增私有索引：
  - `std::unordered_map<std::pair<std::uint32_t, std::uint32_t>, std::unordered_set<std::uint64_t>, PairHash> season_grade_to_groups_;`
  - `std::unordered_map<std::uint64_t, std::unordered_set<std::uint64_t>> group_to_tickets_;`
  - `std::unordered_map<std::pair<std::uint32_t, std::uint32_t>, std::unordered_set<std::uint64_t>, PairHash> unfilled_season_grade_groups_;`
- 公共查询接口：
  - `std::vector<std::uint64_t> GetGroupTicketIds(std::uint32_t season_type, std::uint64_t group_id) const;`
- **一致性约束：** 任何修改 `Ticket.seasons()` 中 `SeasonGroup` 的代码，必须通过 `WriteSeasonGroups` 或等价的索引更新函数同步维护上述索引，确保索引与凭据数据结构始终一致。

- [ ] **Step 5.1: 在头文件中定义索引与哈希辅助结构**

```cpp
struct PairHash {
  std::size_t operator()(
      const std::pair<std::uint32_t, std::uint32_t>& p) const noexcept {
    return std::hash<std::uint32_t>{}(p.first) * 31 +
           std::hash<std::uint32_t>{}(p.second);
  }
};

// 分组索引
std::unordered_map<std::pair<std::uint32_t, std::uint32_t>,
                   std::unordered_set<std::uint64_t>, PairHash>
    season_grade_to_groups_;
std::unordered_map<std::uint64_t, std::unordered_set<std::uint64_t>>
    group_to_tickets_;
std::unordered_map<std::pair<std::uint32_t, std::uint32_t>,
                   std::unordered_set<std::uint64_t>, PairHash>
    unfilled_season_grade_groups_;
```

- [ ] **Step 5.2: 在头文件中声明查询接口**

```cpp
/// @brief 获取指定分组内的凭据 ID 列表
/// @param[in] season_type 赛季类型
/// @param[in] group_id 分组 ID
/// @return 凭据 ID 列表（顺序不做保证）
std::vector<std::uint64_t> GetGroupTicketIds(
    std::uint32_t season_type, std::uint64_t group_id) const;
```

- [ ] **Step 5.3: 在 `WriteSeasonGroups` 中统一维护索引，确保索引与凭据数据一致**

每次写入 `SeasonGroup` 时，按槽位批量更新索引：先移除该槽位中所有凭据在旧分组中的索引，再写入新的 `SeasonGroup` 并建立新索引。

```cpp
void TicketManager::WriteSeasonGroups(std::vector<GroupSlot>& slots,
                                      std::uint32_t season_type,
                                      const config::SeasonInfo& season_info,
                                      const config::SeasonTime& season_time) {
  for (auto& slot : slots) {
    if (slot.entities.empty()) {
      continue;
    }

    // 1. 从旧分组索引中移除这些凭据，并记录受影响的旧分组
    std::unordered_map<std::uint64_t, std::uint32_t> affected_old_groups;
    for (const auto& entity : slot.entities) {
      const auto& ticket = entity->GetData();
      auto it = ticket.seasons().find(season_type);
      if (it != ticket.seasons().end()) {
        std::uint64_t old_group_id = it->second.group_id();
        std::uint32_t old_grade = it->second.grade();
        group_to_tickets_[old_group_id].erase(ticket.id());
        if (group_to_tickets_[old_group_id].empty()) {
          group_to_tickets_.erase(old_group_id);
          season_grade_to_groups_[{season_type, old_grade}].erase(old_group_id);
          unfilled_season_grade_groups_[{season_type, old_grade}].erase(old_group_id);
        } else {
          affected_old_groups[old_group_id] = old_grade;
        }
      }
    }

    // 2. 写入新的 SeasonGroup
    for (const auto& entity : slot.entities) {
      auto& ticket = entity->GetData();
      table::SeasonGroup& group = (*ticket.mutable_seasons())[season_type];
      group.set_type(season_type);
      group.set_begin_time(season_time.begin_time());
      group.set_end_time(season_time.end_time());
      group.set_grade(slot.grade);
      group.set_group_id(slot.group_id);
      entity_manager_.SetDirty(ticket.id());
    }

    // 3. 建立新分组索引
    season_grade_to_groups_[{season_type, slot.grade}].insert(slot.group_id);
    for (const auto& entity : slot.entities) {
      group_to_tickets_[slot.group_id].insert(entity->GetData().id());
    }

    // 4. 维护新分组未满索引
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, slot.grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;
    if (group_size == 0 || slot.entities.size() < group_size) {
      unfilled_season_grade_groups_[{season_type, slot.grade}].insert(slot.group_id);
    } else {
      unfilled_season_grade_groups_[{season_type, slot.grade}].erase(slot.group_id);
    }

    // 5. 维护受影响的旧分组未满索引
    for (const auto& [old_group_id, old_grade] : affected_old_groups) {
      auto tickets_it = group_to_tickets_.find(old_group_id);
      if (tickets_it == group_to_tickets_.end()) {
        continue;
      }
      const config::GradeInfo* old_grade_info =
          FindGradeInfo(season_info, old_grade);
      std::uint32_t old_group_size =
          old_grade_info != nullptr ? old_grade_info->group_size() : 0;
      if (old_group_size == 0 || tickets_it->second.size() < old_group_size) {
        unfilled_season_grade_groups_[{season_type, old_grade}].insert(old_group_id);
      } else {
        unfilled_season_grade_groups_[{season_type, old_grade}].erase(old_group_id);
      }
    }
  }
}
```

> 说明：
> - `WriteSeasonGroups` 是维护分组索引的唯一入口。所有对 `Ticket.seasons()` 的写入必须经过它，以保证索引同步。
> - 调用方（`Initialize`、`NextSeason`、`AddTicket`）不得在外部手动修改 `season_grade_to_groups_`、`group_to_tickets_`、`unfilled_season_grade_groups_`。
> - 步骤 1 与步骤 5 共同处理旧分组变空或人数变化后的索引清理/更新，避免僵尸分组。
> - `WriteSeasonGroups` 签名由 `(slots, season_type, season_time)` 扩展为 `(slots, season_type, season_info, season_time)`，调用处同步更新。
> - **边界情况：**
>   - 凭据数据损坏（同一 `group_id` 对应不同 `grade`）：以最后扫描到的凭据为准；重建索引至少保证索引与当前数据结构一致。
>   - 旧分组变空：从 `season_grade_to_groups_` 与 `unfilled_season_grade_groups_` 中清理该 `group_id`。`group_id` 不再复用，由分配器继续生成新 ID。
>   - `slot.entities.empty()`：直接跳过，不更新索引。
>   - 复用旧 `group_id` 填充：先移除凭据原旧索引，再写入同一 `group_id`，结果正确。
>   - `group_size == 0`：表示该段位分组大小无限制，视为未满，新凭据持续加入同一分组。

- [ ] **Step 5.4: 在 `Initialize` 开始时重建全量索引**

`Initialize` 调用前，凭据数据可能已存在旧的赛季分组。为确保索引与凭据数据结构一致，需在 `Initialize` 开头扫描所有凭据的 `seasons()`，重建 `group_to_tickets_`、`season_grade_to_groups_` 与 `unfilled_season_grade_groups_`。

> **边界情况：**
> - 配置缺失（`config.GetInfo` 失败）：无法判断该 `season_type` 下分组是否已满，对应分组不进入 `unfilled_season_grade_groups_`。
> - `group_size == 0`：表示该段位分组大小无限制，视为未满，新凭据持续加入同一分组。
> - 空分组：扫描凭据时不会生成空的 `group_to_tickets_` 条目，因此空分组不会进入 `unfilled_season_grade_groups_`。

```cpp
void TicketManager::RebuildGroupIndex(
    const SeasonConfigInterface& config) {
  season_grade_to_groups_.clear();
  group_to_tickets_.clear();
  unfilled_season_grade_groups_.clear();

  // 1. 扫描所有凭据，建立 season/grade -> group_ids 与 group_id -> ticket_ids
  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    const auto& ticket = entity->GetData();
    for (const auto& [season_type, group] : ticket.seasons()) {
      season_grade_to_groups_[{season_type, group.grade()}].insert(
          group.group_id());
      group_to_tickets_[group.group_id()].insert(ticket.id());
    }
  }

  // 2. 根据配置计算哪些分组未满
  for (const auto& [key, groups] : season_grade_to_groups_) {
    const auto& [season_type, grade] = key;
    config::SeasonInfo season_info;
    if (!config.GetInfo(season_type, season_info)) {
      continue;
    }
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;
    for (std::uint64_t group_id : groups) {
      auto it = group_to_tickets_.find(group_id);
      if (it == group_to_tickets_.end()) {
        continue;
      }
      if (group_size == 0 || it->second.size() < group_size) {
        unfilled_season_grade_groups_[key].insert(group_id);
      }
    }
  }
}
```

在 `Initialize` 中调用：

```cpp
void TicketManager::Initialize(const SeasonConfigInterface& config,
                               std::uint32_t now_time, std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());
  RebuildGroupIndex(config);

  for (std::uint32_t season_type : config.GetTypes()) {
    InitializeSeasonType(config, season_type, now_time, rng);
  }
}
```

- [ ] **Step 5.5: 实现 `GetGroupTicketIds`**

```cpp
std::vector<std::uint64_t> TicketManager::GetGroupTicketIds(
    std::uint32_t /*season_type*/, std::uint64_t group_id) const {
  auto it = group_to_tickets_.find(group_id);
  if (it == group_to_tickets_.end()) {
    return {};
  }
  return std::vector<std::uint64_t>(it->second.begin(), it->second.end());
}
```

- [ ] **Step 5.6: 新增索引维护测试**

验证调用 `Initialize` 后，`GetGroupTicketIds` 返回正确的凭据 ID 集合。

```cpp
TEST(TicketManagerTest, GroupIndexReturnsMembers) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  MockSeasonConfig config(info);
  config.SetTime(time);
  std::mt19937 rng(12345);

  for (std::uint64_t id = 1; id <= 4; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
  }

  tm.Initialize(config, 50, rng);

  std::unordered_map<std::uint64_t, std::size_t> group_sizes;
  for (std::uint64_t id = 1; id <= 4; ++id) {
    const auto& ticket = manager.GetEntity(id)->GetData();
    const std::uint64_t group_id = ticket.seasons().at(1).group_id();
    auto members = tm.GetGroupTicketIds(1, group_id);
    EXPECT_NE(std::find(members.begin(), members.end(), id), members.end());
    group_sizes[group_id] = members.size();
  }

  EXPECT_EQ(group_sizes.size(), 2);
  for (const auto& [group_id, size] : group_sizes) {
    (void)group_id;
    EXPECT_EQ(size, 2);
  }
}
```

- [ ] **Step 5.7: 构建、测试并提交**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

```bash
git add src/cmatch/ticket_manager.h src/cmatch/ticket_manager.cpp \
  tests/cmatch/test_ticket_manager.cpp
git commit -m "feat(ticket_manager): 建立分组索引并提供按 group_id 查询凭据接口"
```

---

## Task 6: 利用分组索引统一填充未满分组并创建新分组

**Files:**
- 修改：`src/cmatch/ticket_manager.cpp`
- 测试：`tests/cmatch/test_ticket_manager.cpp`

**Interfaces:**
- 修改 `InitializeSeasonType` 中处理"不在当前赛季内"凭据与新凭据的分组逻辑（原 `ticket_manager.cpp:126-161`）。
- 利用 `unfilled_season_grade_groups_` 索引快速定位同段位未满的旧分组，优先填充；剩余凭据统一创建新 `GroupSlot`。
- 保证索引与凭据数据结构同步：`WriteSeasonGroups` 在写入时统一更新 `season_grade_to_groups_`、`group_to_tickets_`、`unfilled_season_grade_groups_`。

- [ ] **Step 6.1: 统一收集待分组凭据**

将以下三类凭据按段位收集到 `pending_by_grade`，避免多次遍历：
1. 不在当前赛季内、已结算并计算新段位后的凭据。
2. 无赛季数据的新凭据，使用 `season_info.initial_grade()`。
3. （可选）当前赛季内需要重新分配的旧凭据；根据 Task 4，当前赛季内凭据保留原分组，因此本项为空。

```cpp
std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>> pending_by_grade;

// 不在当前赛季内的凭据：按原分组结算并计算新段位
for (const auto& [group_id, group] : out_of_season_by_group) {
  (void)group_id;
  if (group.empty()) {
    continue;
  }
  std::uint32_t grade =
      group.front()->GetData().seasons().at(season_type).grade();
  SettleGroup(group, season_type, season_time, grade,
              season_info.score_attr_id());
}

for (const auto& [group_id, group] : out_of_season_by_group) {
  (void)group_id;
  for (const auto& entity : group) {
    const auto& ticket = entity->GetData();
    std::uint32_t old_grade = ticket.seasons().at(season_type).grade();
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, old_grade);
    std::uint32_t new_grade = old_grade;
    auto settlement_it = ticket.settlements().find(season_type);
    if (settlement_it != ticket.settlements().end() && grade_info != nullptr) {
      new_grade = ComputeNextGrade(*grade_info, settlement_it->second.rank(),
                                   settlement_it->second.rank_percent());
    }
    pending_by_grade[new_grade].push_back(entity);
  }
}

// 无赛季数据的凭据
for (const auto& entity : new_entities) {
  std::uint32_t grade = season_info.initial_grade();
  pending_by_grade[grade].push_back(entity);
}
```

- [ ] **Step 6.2: 按段位统一填充未满分组并创建新分组**

对每个段位的待分组凭据，优先填充同段位未满的旧分组，剩余凭据创建新 `GroupSlot`。

```cpp
std::unordered_map<std::uint32_t, std::vector<GroupSlot>> grade_slots;

for (auto& [grade, pending] : pending_by_grade) {
  if (pending.empty()) {
    continue;
  }

  // 新加入的待分组凭据先洗牌，再统一填充/建组
  std::shuffle(pending.begin(), pending.end(), rng);

  const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
  std::uint32_t group_size =
      grade_info != nullptr ? grade_info->group_size() : 0;

  // 1. 优先填充同段位未满的旧分组（利用索引）
  auto unfilled_it = unfilled_season_grade_groups_.find({season_type, grade});
  if (unfilled_it != unfilled_season_grade_groups_.end()) {
    // WriteSeasonGroups 会修改 unfilled_season_grade_groups_，先复制 ID 避免迭代器失效
    std::vector<std::uint64_t> unfilled_groups(
        unfilled_it->second.begin(), unfilled_it->second.end());
    for (std::uint64_t existing_group_id : unfilled_groups) {
      auto tickets_it = group_to_tickets_.find(existing_group_id);
      if (tickets_it == group_to_tickets_.end()) {
        continue;
      }
      std::size_t current_size = tickets_it->second.size();
      if (group_size > 0 && current_size >= group_size) {
        continue;
      }

      // 创建一个 slot 复用旧 group_id
      GroupSlot slot;
      slot.group_id = existing_group_id;
      slot.grade = grade;
      while (!pending.empty() &&
             (group_size == 0 || current_size + slot.entities.size() < group_size)) {
        slot.entities.push_back(pending.back());
        pending.pop_back();
      }
      if (!slot.entities.empty()) {
        grade_slots[grade].push_back(std::move(slot));
      }
      if (pending.empty()) {
        break;
      }
    }
  }

  // 2. 剩余凭据统一创建新分组
  if (!pending.empty()) {
    if (group_size == 0) {
      GroupSlot slot;
      slot.grade = grade;
      slot.group_id =
          group_id_allocator_.Allocate(pending.front()->GetData().zone_id());
      slot.entities = std::move(pending);
      grade_slots[grade].push_back(std::move(slot));
    } else {
      for (std::size_t i = 0; i < pending.size(); i += group_size) {
        std::size_t end = std::min(i + group_size, pending.size());
        GroupSlot slot;
        slot.grade = grade;
        slot.group_id =
            group_id_allocator_.Allocate(pending[i]->GetData().zone_id());
        slot.entities.assign(
            std::next(pending.begin(), static_cast<std::ptrdiff_t>(i)),
            std::next(pending.begin(), static_cast<std::ptrdiff_t>(end)));
        grade_slots[grade].push_back(std::move(slot));
      }
    }
  }
}
```

> **边界情况：**
> - **洗牌策略：** 只有新加入的待分组凭据（不在当前赛季内结算后、无赛季数据）需要洗牌，当前赛季内保留原分组的凭据不参与洗牌。
> - **填充顺序非确定性：** `unfilled_season_grade_groups_` 使用 `std::unordered_set`，遍历顺序不确定。新加入凭据已整体洗牌，因此填充结果满足随机性要求。
> - **`group_size == 0`：** 该段位分组大小无限制，所有待分组凭据放入一个 `GroupSlot`。
> - **待分组不足以填满旧分组：** 旧分组只填充部分，`WriteSeasonGroups` 会维护 `unfilled_season_grade_groups_`。
> - **待分组填满旧分组后仍有剩余：** `WriteSeasonGroups` 会从未满索引中移除已填满的旧分组，剩余凭据创建新分组。
> - **当前赛季内凭据所在分组已满：** 这些分组不在 `unfilled_season_grade_groups_` 中，不会被用于填充，保证当前赛季内凭据不被挤出原分组。

- [ ] **Step 6.3: 统一写入并维护索引**

通过 `WriteSeasonGroups` 将所有 `grade_slots` 写入凭据，由 `WriteSeasonGroups` 统一维护索引与数据一致性。

```cpp
for (auto& [grade, slots] : grade_slots) {
  (void)grade;
  WriteSeasonGroups(slots, season_type, season_info, season_time);
}
```

- [ ] **Step 6.4: 更新测试**

新增或调整测试，验证：
- 当前赛季内未满的旧分组会被来自"不在当前赛季内"结算后的凭据优先填满。
- 填满后剩余凭据创建新分组。
- 新加入的待分组凭据会经过洗牌，填充/建组结果具有随机性。
- 当前赛季内凭据保留原分组，不会被挤出。
- 索引 `GetGroupTicketIds` 与凭据数据结构一致。
- 分组被清空后，`GetGroupTicketIds` 返回空。
- 跨多个 `season_type` 时索引互不干扰。

```cpp
TEST(TicketManagerTest, PendingTicketsFillUnfilledGroupsFirst) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  (*info.mutable_grades())[1].set_group_size(3);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  MockSeasonConfig config(info);
  config.SetTime(time);
  std::mt19937 rng(12345);

  // 当前赛季内已有一个未满分组（size=1，容量=3）
  auto in_season = manager.GetOrCreateEntity(1, 1);
  SetScore(in_season, info.score_attr_id(), 100);
  {
    auto& group = (*in_season->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  // 不在当前赛季内的两个凭据，结算后应回到 grade 1
  for (std::uint64_t id = 2; id <= 3; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(50);  // 不在当前赛季 [0, 100) 内
    group.set_grade(1);
    group.set_group_id(2000 + id);
  }

  tm.Initialize(config, 75, rng);

  // 凭据 2、3 应优先填入 group 1000，与凭据 1 同组
  EXPECT_EQ(manager.GetEntity(1)->GetData().seasons().at(1).group_id(), 1000);
  EXPECT_EQ(manager.GetEntity(2)->GetData().seasons().at(1).group_id(), 1000);
  EXPECT_EQ(manager.GetEntity(3)->GetData().seasons().at(1).group_id(), 1000);

  auto members = tm.GetGroupTicketIds(1, 1000);
  EXPECT_EQ(members.size(), 3);
}
```

- [ ] **Step 6.5: 构建、测试并提交**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

```bash
git add src/cmatch/ticket_manager.cpp tests/cmatch/test_ticket_manager.cpp
git commit -m "perf(ticket_manager): 利用分组索引统一填充未满分组并创建新分组"
```

---

## Task 7: 改造 `NextSeason` 与 `AddTicket` 使用统一填充/索引

**Files:**
- 修改：`src/cmatch/ticket_manager.h`
- 修改：`src/cmatch/ticket_manager.cpp`
- 测试：`tests/cmatch/test_ticket_manager.cpp`

**Interfaces:**
- `NextSeason` 和 `AddTicket` 均使用更新后的 `WriteSeasonGroups` 签名，确保索引同步。
- `AddTicket` 参数与 `Initialize` 对齐：
  - `void AddTicket(const SeasonConfigInterface& config, std::uint32_t now_time, std::uint64_t ticket_id, std::mt19937& rng);`
  - 内部按 `config.GetTypes()` 遍历，调用私有方法 `AddTicketToSeason` 逐个类型处理。
- `AddTicketToSeason` 优先通过 `unfilled_season_grade_groups_` 查找同段位未满旧分组并填入；无未满旧分组时创建新分组。
- `NextSeason` 虽不需要填充旧分组（新赛季全部重新分组），但必须通过 `WriteSeasonGroups` 维护索引。

- [ ] **Step 7.1: 修改 `NextSeason` 调用 `WriteSeasonGroups` 新签名**

将 `NextSeason` 中所有 `WriteSeasonGroups(slots, season_type, season_time)` 改为：

```cpp
WriteSeasonGroups(slots, season_type, season_info, season_time);
```

- [ ] **Step 7.2: 修改 `AddTicket` 头文件签名**

```cpp
/// @brief 新加入的凭据，由外部调用一次
///
/// 内部遍历所有已配置的赛事类型，为凭据分配初始段位并优先填入同段位未满旧分组；
/// 若已存在赛季分组且凭据上的时间位于当前赛季内，则修复其时间为配置值。
/// @param[in] config 赛季配置接口
/// @param[in] now_time 当前时间
/// @param[in] ticket_id 新凭据 ID
/// @param[in,out] rng 随机数引擎
void AddTicket(const SeasonConfigInterface& config, std::uint32_t now_time,
               std::uint64_t ticket_id, std::mt19937& rng);

private:
/// @brief 为单个赛事类型处理新凭据
/// @param[in] entity 凭据实体
/// @param[in] season_type 赛季类型
/// @param[in] season_info 赛季信息
/// @param[in] season_time 赛季时间
/// @param[in] now_time 当前时间
/// @param[in,out] rng 随机数引擎
void AddTicketToSeason(const TicketEntityPtr& entity,
                       std::uint32_t season_type,
                       const config::SeasonInfo& season_info,
                       const config::SeasonTime& season_time,
                       std::uint32_t now_time, std::mt19937& rng);
```

- [ ] **Step 7.3: 重构 `AddTicket` 实现**

```cpp
void TicketManager::AddTicket(const SeasonConfigInterface& config,
                              std::uint32_t now_time,
                              std::uint64_t ticket_id, std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());

  TicketEntityPtr entity = entity_manager_.GetEntity(ticket_id);
  if (entity == nullptr) {
    return;
  }

  for (std::uint32_t season_type : config.GetTypes()) {
    config::SeasonInfo season_info;
    config::SeasonTime season_time;
    if (!config.GetInfo(season_type, season_info) ||
        !config.GetTime(season_type, season_time)) {
      continue;
    }

    AddTicketToSeason(entity, season_type, season_info, season_time, now_time,
                      rng);
  }
}

void TicketManager::AddTicketToSeason(
    const TicketEntityPtr& entity, std::uint32_t season_type,
    const config::SeasonInfo& season_info,
    const config::SeasonTime& season_time, std::uint32_t now_time,
    std::mt19937& rng) {
  auto& ticket = entity->GetData();
  auto it = ticket.seasons().find(season_type);
  if (it == ticket.seasons().end()) {
    std::uint32_t grade = season_info.initial_grade();
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;

    // 优先填入同段位未满旧分组
    auto unfilled_it = unfilled_season_grade_groups_.find({season_type, grade});
    if (unfilled_it != unfilled_season_grade_groups_.end() &&
        !unfilled_it->second.empty()) {
      std::uint64_t existing_group_id = *unfilled_it->second.begin();
      auto tickets_it = group_to_tickets_.find(existing_group_id);
      if (tickets_it != group_to_tickets_.end() &&
          (group_size == 0 || tickets_it->second.size() < group_size)) {
        std::vector<GroupSlot> slots;
        GroupSlot slot;
        slot.group_id = existing_group_id;
        slot.grade = grade;
        slot.entities.push_back(entity);
        slots.push_back(std::move(slot));
        WriteSeasonGroups(slots, season_type, season_info, season_time);
        return;
      }
    }

    // 无未满旧分组，创建新分组
    std::vector<GroupSlot> slots;
    FormGroupSlots({entity}, group_size, grade, rng, slots);
    WriteSeasonGroups(slots, season_type, season_info, season_time);
    return;
  }

  // 已存在赛季分组时，仅在当前赛季内修复时间
  config::SeasonTime group_time;
  group_time.set_begin_time(it->second.begin_time());
  group_time.set_end_time(it->second.end_time());
  if (IsInSeason(now_time, group_time)) {
    table::SeasonGroup& group = (*ticket.mutable_seasons())[season_type];
    if (group.begin_time() != season_time.begin_time() ||
        group.end_time() != season_time.end_time()) {
      group.set_begin_time(season_time.begin_time());
      group.set_end_time(season_time.end_time());
      entity_manager_.SetDirty(ticket.id());
    }
  }
}
```

- [ ] **Step 7.4: 更新测试**

更新 `test_ticket_manager.cpp` 中所有 `tm.AddTicket(info, time, ...)` 调用为 `tm.AddTicket(config, now_time, ...)`。

新增测试验证：
- 新凭据加入时优先填入同段位未满旧分组。
- 凭据已存在赛季分组且 `now_time` 位于当前赛季内时，`AddTicket` 会修复时间为配置值。
- 凭据已存在赛季分组但 `now_time` 不在当前赛季内时，`AddTicket` 不会修改时间。

```cpp
TEST(TicketManagerTest, AddTicketFillsUnfilledGroupFirst) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  (*info.mutable_grades())[1].set_group_size(3);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  MockSeasonConfig config(info);
  config.SetTime(time);
  std::mt19937 rng(12345);

  // 当前赛季内有一个未满分组（size=1，容量=3）
  auto existing = manager.GetOrCreateEntity(1, 1);
  SetScore(existing, info.score_attr_id(), 100);
  {
    auto& group = (*existing->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  // 重建索引（Initialize 会执行）
  tm.Initialize(config, 50, rng);

  // 新凭据应加入 group 1000
  auto new_entity = manager.GetOrCreateEntity(2, 1);
  SetScore(new_entity, info.score_attr_id(), 200);
  tm.AddTicket(config, 50, 2, rng);

  EXPECT_EQ(new_entity->GetData().seasons().at(1).group_id(), 1000);
}
```

- [ ] **Step 7.5: 构建、测试并提交**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

```bash
git add src/cmatch/ticket_manager.h src/cmatch/ticket_manager.cpp \
  tests/cmatch/test_ticket_manager.cpp
git commit -m "refactor(ticket_manager): NextSeason 与 AddTicket 使用统一索引填充，AddTicket 参数与 Initialize 对齐"
```

---

## Task 8: `MatchGroupServiceImpl::GetGroupMembers` 使用索引接口

**Files:**
- 修改：`src/cmatch/match_group_service_impl.cpp`
- 测试：`tests/cmatch/test_match_group_service.cpp`

**Interfaces:**
- `MatchGroupServiceImpl` 调用 `ticket_manager_.GetGroupTicketIds(season_type, group_id)` 获取成员。
- `GetGroupMembersResp.members` 格式不变：`map<uint64, uint32>`，key 为凭据 ID，value 为 `zone_id`。

- [ ] **Step 8.1: 修改 `GetGroupMembers` 实现**

```cpp
void MatchGroupServiceImpl::GetGroupMembers(
    const std::shared_ptr<lib::GetGroupMembersReq>& request,
    const std::function<void(const lib::GetGroupMembersResp&)>& done) {
  lib::GetGroupMembersResp resp;
  resp.set_result(lib::OK);

  const std::uint32_t season_type = request->type();
  const std::uint64_t group_id = request->group_id();

  for (std::uint64_t ticket_id :
       ticket_manager_.GetGroupTicketIds(season_type, group_id)) {
    TicketEntityPtr entity = entity_manager_.GetEntity(ticket_id);
    if (entity == nullptr) {
      continue;
    }
    const table::Ticket& ticket = entity->GetData();
    (*resp.mutable_members())[ticket.id()] = ticket.zone_id();
  }

  done(resp);
}
```

- [ ] **Step 8.2: 验证测试无需修改或仅做小调整**

现有 `GetGroupMembersReturnsMap` 测试应继续通过，因为功能行为不变。

- [ ] **Step 8.3: 构建、测试并提交**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

```bash
git add src/cmatch/match_group_service_impl.cpp
git commit -m "refactor(service): GetGroupMembers 使用 TicketManager 分组索引查询"
```

---

## Task 9: 同步更新设计文档

**Files:**
- 修改：`docs/specs/cmatch_library/design/config.md`
- 修改：`docs/specs/cmatch_library/design/user_interface.md`
- 修改：`docs/specs/cmatch_library/design/matching_algorithm.md`

- [ ] **Step 9.1: 更新 `config.md`**

删除 `SeasonInfo` 中 `min_score` 字段的定义与注释。更新后：

```protobuf
// 赛事信息
message SeasonInfo {
  // 赛事类型
  uint32 type = 1;
  // 段位信息列表，key 为段位编号
  map<uint32, GradeInfo> grades = 2;
  // 初始积分
  uint64 initial_score = 3;
  // 积分属性 ID
  uint32 score_attr_id = 5;
  // 切换赛季后是否重置积分
  bool reset_score = 6;
  // 初始段位
  uint32 initial_grade = 7;
}
```

> 注意：字段编号 `4`（原 `min_score`）已删除并保留，禁止复用，避免与旧数据冲突。

- [ ] **Step 9.2: 更新 `user_interface.md`**

1. `RemoveSettlementListReq.settlement_ids` 类型改为 `repeated uint32`。
2. `TicketManager::BuildSeason` 改为 `TicketManager::Initialize`，签名改为：

```cpp
void Initialize(const SeasonConfigInterface& config,
                std::uint32_t now_time,
                std::mt19937& rng);
```

3. `TicketManager::AddTicket` 签名改为：

```cpp
void AddTicket(const SeasonConfigInterface& config,
               std::uint32_t now_time,
               std::uint64_t ticket_id,
               std::mt19937& rng);
```

4. 补充 `GetGroupTicketIds` 接口说明：

```cpp
/// @brief 获取指定分组内的凭据 ID 列表
/// @param[in] season_type 赛季类型
/// @param[in] group_id 分组 ID
/// @return 凭据 ID 列表（顺序不做保证）
std::vector<std::uint64_t> GetGroupTicketIds(
    std::uint32_t season_type, std::uint64_t group_id) const;
```

5. 时间修复语义：
   - `Initialize` 仅在凭据上的时间位于当前赛季内时，才将其修复为配置的赛季时间。
   - `AddTicket` 对已存在赛季分组的凭据，同样仅在当前赛季内修复时间。

- [ ] **Step 9.3: 更新 `matching_algorithm.md`**

补充索引维护与统一分组说明：

```markdown
## 分组索引

`TicketManager` 维护以下索引以加速查询与统一分组：

- `<season_type, grade> -> group_ids`：按赛季类型与段位快速定位所有分组。
- `group_id -> ticket_ids`：按分组 ID 快速获取成员凭据。
- `<season_type, grade> -> group_ids`（未满）：快速定位未满的分组，用于优先填充。

`WriteSeasonGroups` 是维护上述索引的唯一入口，所有对 `Ticket.seasons()` 的写入必须经过它。
`GetGroupMembers` 接口通过 `group_id -> ticket_ids` 索引直接返回成员，无需遍历全部凭据。

## 统一分组

`Initialize` 与 `AddTicket` 对不在当前赛季内或新加入的凭据，优先填入同段位未满的旧分组；
剩余凭据再统一创建新分组，从而减少分组碎片并提升效率。
新加入的待分组凭据在填充/建组前会统一洗牌，当前赛季内保留原分组的凭据不参与洗牌。
`NextSeason` 切换赛季时全部重新分组，不填充旧分组，但仍通过 `WriteSeasonGroups` 维护索引。

## 分组大小

`GradeInfo.group_size == 0` 表示该段位分组大小无限制，所有凭据加入同一分组。
```

- [ ] **Step 9.4: 提交文档更新**

```bash
git add docs/specs/cmatch_library/design/config.md \
  docs/specs/cmatch_library/design/user_interface.md \
  docs/specs/cmatch_library/design/matching_algorithm.md
git commit -m "docs: 同步更新设计文档以反映 TicketManager 重构"
```

---

## Task 10: 最终验证

- [ ] **Step 10.1: 完整构建**

```bash
cmake --build --preset ninja-debug
```

- [ ] **Step 10.2: 完整测试**

```bash
ctest --preset ninja-debug
```

- [ ] **Step 10.3: 格式化检查**

```bash
find src/cmatch tests/cmatch proto/cmatch \
  \( -name "*.cpp" -o -name "*.h" -o -name "*.proto" \) \
  -exec clang-format -i {} +
```

- [ ] **Step 10.4: 静态分析检查**

确保 `clangd` 在 `src/cmatch/ticket_manager.h`、`src/cmatch/ticket_manager.cpp`、`src/cmatch/match_group_service_impl.cpp` 中无新增 `ClangTidy` 警告。

- [ ] **Step 10.5: 最终提交（如有格式化差异）**

```bash
git commit -m "style: 应用 clang-format 格式化"
```

---

## Self-Review Checklist

- [ ] 所有用户列出的重构项均有对应 Task。
- [ ] 计划中没有 "TBD"、"TODO"、"实现 later" 等占位符。
- [ ] 方法签名在 Task 3、Task 4、Task 5、Task 6、Task 7 之间保持一致。
- [ ] `RebuildGroupIndex`、`WriteSeasonGroups` 与统一填充的边界情况已在 Task 5、Task 6 中覆盖。
- [ ] `NextSeason` 与 `AddTicket` 的统一填充/索引策略已在 Task 7 中明确。
- [ ] 设计文档 `config.md`、`user_interface.md`、`matching_algorithm.md` 均有明确更新步骤。
- [ ] 评审中确认的 `PairHash`、索引统一维护、`group_id` 不复用、`group_size == 0` 语义、洗牌策略、时间修复条件均已同步到计划。
- [ ] 每个 Task 结束时均包含构建与测试命令。
