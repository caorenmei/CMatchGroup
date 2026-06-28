# 步骤 3：TicketManager 匹配算法

## 目标

根据 `matching_algorithm.md` 与 `user_interface.md` 实现 `TicketManager` 类，覆盖赛季判断、结算、分组、段位升降以及分组 ID 分配等核心算法。

## 原始规格引用

- `matching_algorithm.md`：
  - 同一赛季判断（闭开区间）。
  - 赛季结算（按积分降序，ID 升序作为第二键）。
  - 分组 ID 分配（高 32 位 `zone_id`，低 32 位自增编号）。
  - 段位升降规则。
  - 赛季分组（`std::shuffle` + 随机数引擎，`group_size` 控制）。
- `user_interface.md`：
  - `TicketManager` 构造函数、`BuildSeason`、`NextSeason`、`AddTicket` 签名。

## 当前代码库基线

- 步骤 1 完成后，已存在 Protobuf 生成类型。
- 步骤 2 完成后，已存在 `SeasonConfigInterface`、`TicketEntityInterface`、`TicketEntityManagerInterface` 及内存 Mock。
- 尚无 `TicketManager` 实现。

## 开始前的校准清单

1. **随机数引擎来源**：规格要求“外部传入的随机数引擎”。确认 `TicketManager` 的方法是否接收 `std::mt19937&` 或更通用的 `std::random_device` 引用。当前建议由调用方传入 `std::mt19937&`，以便测试可复现。
2. **分组 ID 自增编号持久化**：规格要求“实体加载完成后，获取所有凭据上的分组 ID，包括结算数据里的分组 ID，计算最大的自增编号”。确认该逻辑放在 `BuildSeason` 中是否合理，是否需要额外的 `InitGroupIdAllocator` 方法。
3. **段位升降配置与默认值**：`promote_rank_percent` / `promote_rank` 同时为 0 表示不升段；`demote_rank_percent` / `demote_rank` 同时为 0 表示不降段。需确认 Protobuf 中 `float` 未设置时默认值为 0.0，与规格一致。
4. **`min_score` 的处理**：规格中 `SeasonInfo` 包含 `min_score`，但 `matching_algorithm.md` 未明确说明积分低于最小值时的处理。当前需求中暂不实现积分下限裁剪，留待步骤 5 或后续需求明确。
5. **分组算法中的“同一段位”**：确认是按当前段位（`SeasonGroup.grade`）还是结算后新段位分组。根据 `matching_algorithm.md` 的“赛季分组”描述，应为同一段位。

## 需求

### R3.1 实现 `TicketManager` 类框架

在 `src/cmatch/ticket_manager.h` 中声明：

```cpp
#ifndef CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
#define CMATCH_SRC_CMATCH_TICKET_MANAGER_H_

#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>

#include "cmatch/config.pb.h"
#include "cmatch/ticket_entity_manager_interface.h"

namespace cmatch {

// 凭据管理器
class TicketManager {
 public:
  explicit TicketManager(TicketEntityManagerInterface& entity_manager);

  // 实体加载完成后，遍历每个赛事类型调用一次
  void BuildSeason(const config::SeasonInfo& season_info,
                   const config::SeasonTime& season_time,
                   std::uint32_t now_time, std::mt19937& rng);

  // 切换赛季
  void NextSeason(const config::SeasonInfo& season_info,
                  const config::SeasonTime& season_time,
                  std::uint32_t now_time, std::mt19937& rng);

  // 新加入的凭据，遍历每个赛事类型调用一次
  void AddTicket(const config::SeasonInfo& season_info,
                 const config::SeasonTime& season_time,
                 std::uint32_t now_time, std::uint64_t ticket_id,
                 std::mt19937& rng);

 private:
  struct GroupIdAllocator {
    std::uint32_t next_sequence_ = 1;

    void Initialize(
        const std::unordered_map<std::uint64_t, TicketEntityPtr>& entities) {
      std::uint32_t max_sequence = 0;
      for (const auto& [id, entity] : entities) {
        (void)id;
        const auto& ticket = entity->GetData();
        for (const auto& [type, group] : ticket.seasons()) {
          (void)type;
          max_sequence = std::max(
              max_sequence, static_cast<std::uint32_t>(group.group_id()));
        }
        for (const auto& [type, settlement] : ticket.settlements()) {
          (void)type;
          max_sequence = std::max(
              max_sequence, static_cast<std::uint32_t>(settlement.group_id()));
        }
      }
      next_sequence_ = max_sequence + 1;
    }

    std::uint64_t Allocate(std::uint32_t zone_id) {
      std::uint64_t group_id =
          (static_cast<std::uint64_t>(zone_id) << 32) | next_sequence_;
      ++next_sequence_;
      return group_id;
    }
  };

  struct GroupSlot {
    std::uint64_t group_id = 0;
    std::uint32_t grade = 0;
    std::vector<TicketEntityPtr> entities;
  };

  static std::uint32_t ComputeNextGrade(const config::GradeInfo& grade_info,
                                        std::uint32_t rank,
                                        float rank_percent);
  void SettleGroup(const std::vector<TicketEntityPtr>& group,
                   std::uint32_t season_type,
                   const config::SeasonTime& season_time, std::uint32_t grade,
                   std::uint32_t score_attr_id);
  void FormGroupSlots(std::vector<TicketEntityPtr> tickets,
                      std::uint32_t group_size, std::uint32_t grade,
                      std::mt19937& rng, std::vector<GroupSlot>& slots);
  void AddToGroupSlots(const TicketEntityPtr& entity, std::uint32_t group_size,
                       std::uint32_t grade, std::vector<GroupSlot>& slots);
  void WriteSeasonGroups(std::vector<GroupSlot>& slots,
                         std::uint32_t season_type,
                         const config::SeasonTime& season_time);

  TicketEntityManagerInterface& entity_manager_;
  GroupIdAllocator group_id_allocator_;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
```

### R3.2 同一赛季判断

实现私有辅助函数：

```cpp
bool IsInSeason(std::uint32_t now_time, const config::SeasonTime& season_time);
```

- `now_time >= begin_time` 且 `now_time < end_time` 时返回 `true`。
- 否则返回 `false`。

### R3.3 分组 ID 分配器

实现 `GroupIdAllocator`：

- 维护下一个自增编号 `next_sequence_`（`std::uint32_t`）。
- 提供 `void Initialize(const std::unordered_map<...TicketEntityPtr>& entities)`：遍历所有凭据的 `SeasonGroup` 与 `SeasonSettlement` 中的 `group_id`，提取低 32 位，取最大值加 1 作为 `next_sequence_`。
- 提供 `std::uint64_t Allocate(std::uint32_t zone_id)`：返回 `(static_cast<std::uint64_t>(zone_id) << 32) | next_sequence_`，然后 `++next_sequence_`。

### R3.4 赛季结算

实现 `SettleGroup(const std::vector<TicketEntityPtr>& group, std::uint32_t season_type, const config::SeasonTime& season_time)`：

- 将分组内凭据按 `attributes[score_attr_id]` 降序排列，积分相同则按 `id` 升序。
- 排名从 1 开始。
- 排名百分比 = 排名 / 分组内凭据总数（`float`）。
- 为每个凭据新增 `SeasonSettlement`，保存赛季类型、时间、段位、分组编号、排名、排名百分比、积分。
- 调用 `entity_manager_.SetDirty(ticket_id)`。

### R3.5 段位升降

实现 `ComputeNextGrade(const config::GradeInfo& grade_info, std::uint32_t rank, float rank_percent)`：

- 先检查升段：
  - 若 `promote_rank_percent > 0` 且 `rank_percent <= promote_rank_percent`，升段到 `next_grade`。
  - 否则若 `promote_rank > 0` 且 `rank <= promote_rank`，升段到 `next_grade`。
- 若未升段，检查降段：
  - 若 `demote_rank_percent > 0` 且 `rank_percent > demote_rank_percent`，降段到 `prev_grade`。
  - 否则若 `demote_rank > 0` 且 `rank > demote_rank`，降段到 `prev_grade`。
- 返回最终段位。

### R3.6 赛季分组

实现 `FormGroups(const std::vector<TicketEntityPtr>& tickets, std::uint32_t group_size, std::mt19937& rng)`：

- 使用 `std::shuffle` 打乱顺序。
- 若 `group_size == 0`，所有凭据作为一个分组。
- 否则按 `group_size` 切分，不足部分也作为一个分组。
- 为每个分组调用 `group_id_allocator_.Allocate(zone_id)` 分配分组 ID，其中 `zone_id` 取分组内第一个凭据的 `zone_id`。
- 将分组信息写入各凭据的 `table::Ticket::mutable_seasons()` 中对应 `season_type` 的 `SeasonGroup`。
- 调用 `entity_manager_.SetDirty(ticket_id)`。

### R3.7 `BuildSeason` 行为

- 校准 `GroupIdAllocator`。
- 对指定赛季类型，将所有实体按“是否在当前赛季内”分为两类。
- **在当前赛季内的凭据**：按段位分组，重新分组并写入 `SeasonGroup`。
- **不在当前赛季内的凭据**：按原有分组结算，计算段位，加入当前赛季分组（不足则创建新分组）。

注意：异常处理中更复杂的数据修复逻辑（不在当前赛季的处理）在步骤 5 中细化；本步骤实现基础版本即可。

### R3.8 `NextSeason` 行为

- 对指定赛季类型的所有凭据，按当前 `SeasonGroup` 分组结算。
- 根据结算结果计算新段位。
- 按新段位重新分组并写入新的 `SeasonGroup`。

### R3.9 `AddTicket` 行为

- 从实体管理器获取凭据实体。
- 若凭据不在当前赛季中（无对应 `SeasonGroup`），为其分配初始段位 `initial_grade`，并创建新的 `SeasonGroup`（可能单独成组）。
- 若凭据已在当前赛季中，仅确保其 `SeasonGroup` 与配置时间一致。

## 验收条件

- [ ] `cmake --build --preset ninja-debug` 成功。
- [ ] `ctest --preset ninja-debug` 中新增测试全部通过，至少覆盖：
  - 赛季判断边界（`now_time == begin_time`、`now_time == end_time - 1`、`now_time == end_time`）。
  - 结算排名与百分比计算（含积分相同按 ID 排序）。
  - 分组 ID 唯一性（跨 zone_id、跨结算数据）。
  - 段位升降规则（百分比优先、rank 次之、不升不降）。
  - 随机分组（`group_size` 生效、剩余凭据成组、`group_size == 0` 时单组）。
  - `BuildSeason` 对在当前赛季内凭据的基础分组。
  - `NextSeason` 的结算与段位升降。
  - `AddTicket` 为新凭据创建初始分组。
- [ ] `clang-format` 执行后无文件内容变更。
- [ ] `clang-tidy` 对新增 C++ 文件无新增警告。

## 预计交付文件

- 新增：
  - `src/cmatch/ticket_manager.h`
  - `src/cmatch/ticket_manager.cpp`
  - `tests/cmatch/test_ticket_manager.cpp`
- 修改：
  - `src/cmatch/CMakeLists.txt`（将 `ticket_manager.cpp` 加入库源文件列表）
  - `tests/cmatch/CMakeLists.txt`（新增 `test_ticket_manager` 可执行文件，源文件路径为 `test_ticket_manager.cpp`）

## 风险与注意事项

- **随机性测试**：使用固定种子的 `std::mt19937` 使测试结果可复现。
- **浮点比较**：排名百分比比较时使用 `std::numeric_limits<float>::epsilon()` 相关容差，或按规格直接比较。
- **分组 ID 溢出**：自增编号为 32 位，理论上可能溢出。可在文档中注明不考虑溢出场景，或实现环绕检测。
- **`BuildSeason` 与异常处理的边界**：本步骤仅实现基础修复，复杂的多实体一致性修复交给步骤 5。
- **积分属性不存在**：若凭据的 `attributes` 中不存在 `score_attr_id`，应视为 0 分。

## 完成后的下一步

进入 [步骤 4：用户接口服务实现](step-04-user-interface-service.md)。
