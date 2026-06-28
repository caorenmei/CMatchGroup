# 步骤 5：异常处理与数据修复

## 目标

根据 `exception_handling.md` 在 `BuildSeason` 阶段实现实体数据修复与赛季时间修复，确保多个实体之间的数据一致性。

## 原始规格引用

- `exception_handling.md`：
  - 实体数据修复：单个实体数据一致，但多个实体可能不一致；`BuildSeason` 阶段需修复。
  - 对不在当前赛季的凭据：按分组结算、计算新段位、加入当前赛季分组。
  - 赛季时间修复：配置时间与凭据时间不一致时，以配置时间为准并修复凭据。

## 当前代码库基线

- 步骤 4 完成后，已存在完整的服务实现与 `TicketManager`。
- `BuildSeason` 在步骤 3 中已实现基础版本。
- 异常处理与数据修复的细节尚未实现。

## 开始前的校准清单

1. **结算时是否修改已结算凭据**：规格明确“不修改其数据”。在 `BuildSeason` 阶段对不在当前赛季的凭据按分组结算时，已结算的凭据只参与排名计算，不写入新的 `SeasonSettlement`。
2. **分组大小不够时的处理**：规格要求“如果分组大小不够，创建新的分组”。需确认“不够”是指当前赛季该段位已有分组人数达到 `group_size` 时，创建新分组。
3. **赛季时间修复的触发时机**：规格要求“判断是在同一赛季时，使用配置的赛季时间为准，并且修复凭据上的赛季时间”。这应在 `BuildSeason` 中统一处理，而不是分散在各个方法中。
4. **段位升降与结算的交互**：不在当前赛季的凭据结算后，需要根据 `GradeInfo` 计算新段位，然后按新段位分组。
5. **是否重置积分**：`SeasonInfo.reset_score` 与 `initial_score` 在赛季切换时的处理是否在本步骤实现？规格在 `config.md` 中定义了这些字段，但 `exception_handling.md` 未明确说明。当前建议：
   - 若 `reset_score == true`，切换赛季时将所有凭据积分重置为 `initial_score`。
   - 若 `reset_score == false`，保留当前积分。
   - 该逻辑放在 `NextSeason` 中实现。

## 需求

### R5.1 赛季时间修复

在 `BuildSeason` 中，对所有处理到的凭据：

- 如果凭据的 `SeasonGroup`（对应类型）存在，且其 `begin_time` / `end_time` 与配置不一致：
  - 以 `season_time` 为准，覆盖凭据上的 `begin_time` / `end_time`。
  - 调用 `entity_manager_.SetDirty(ticket_id)`。

### R5.2 不在当前赛季凭据的数据修复

在 `BuildSeason` 中，对当前时间不在赛季内的凭据：

1. **按原分组聚合**：按凭据上已有的 `SeasonGroup.group_id` 分组。
2. **结算（不修改已结算凭据）**：
   - 对每个分组，将组内凭据按积分降序、ID 升序排序。
   - 计算排名与百分比。
   - 对尚未结算的凭据，写入 `SeasonSettlement`。
   - 已结算的凭据不修改其 `settlements`。
3. **计算新段位**：对每个凭据，根据当前段位的 `GradeInfo` 与结算结果，调用 `ComputeNextGrade`。
4. **加入当前赛季分组**：
   - 按新段位将凭据聚合。
   - 对每个段位的凭据，打乱顺序后按 `group_size` 分组。
   - 若当前赛季该段位已有分组且人数不足 `group_size`，优先填入；否则创建新分组。
   - 为每个分组分配新的 `group_id`，写入 `SeasonGroup`。
   - 调用 `SetDirty`。

### R5.3 在当前赛季凭据的数据修复

在 `BuildSeason` 中，对当前时间在赛季内的凭据：

- 按当前段位分组。
- 对每个段位的凭据重新随机分组（步骤 3 已实现基础版本）。
- 此步骤应确保：
  - 赛季时间已修复为配置时间。
  - 分组 ID 唯一。
  - 每个凭据的 `SeasonGroup` 反映当前赛季信息。

### R5.4 `NextSeason` 中的积分重置

在 `NextSeason` 中，结算完成后、重新分组前：

- 若 `season_info.reset_score() == true`，将每个凭据的 `attributes[score_attr_id]` 设置为 `season_info.initial_score()`。
- 若 `reset_score == false`，不修改积分。
- 调用 `SetDirty`。

### R5.5 测试

创建/扩展 `tests/cmatch/test_exception_handling.cpp`，覆盖：

- 配置赛季时间与凭据不一致时，`BuildSeason` 修复凭据时间。
- 不在当前赛季的凭据被正确结算、升降段、并加入当前赛季分组。
- 已结算凭据在结算时不被重复修改。
- 切换赛季时 `reset_score` 为 `true` 则积分重置，为 `false` 则保留。
- 合服场景下，不同 `zone_id` 的凭据分组 ID 不冲突。

## 验收条件

- [ ] `cmake --build --preset ninja-debug` 成功。
- [ ] `ctest --preset ninja-debug` 中新增异常处理测试全部通过。
- [ ] 步骤 3 的现有测试不因本步骤改动而失败（回归测试）。
- [ ] `clang-format` 执行后无文件内容变更。
- [ ] `clang-tidy` 对修改后的 C++ 文件无新增警告。
- [ ] 异常处理逻辑有清晰的代码注释说明修复规则。

## 预计交付文件

- 修改：
  - `src/cmatch/ticket_manager.cpp`（核心修复逻辑）
  - `tests/CMakeLists.txt`（新增 `test_exception_handling` 可执行文件，源文件路径为 `cmatch/test_exception_handling.cpp`）
- 新增：
  - `tests/cmatch/test_exception_handling.cpp`

## 风险与注意事项

- **重复结算**：必须确保已结算凭据不参与写入，否则会导致历史结算数据被覆盖。
- **分组填充策略**：“分组大小不够，创建新的分组”可能存在多种解释。当前策略为：优先填入当前赛季同段位已有未满分组，否则创建新分组。需在文档和代码注释中明确。
- **跨段位聚合**：不在当前赛季的凭据结算后可能升降段，必须按新段位而非旧段位分组。
- **性能**：`BuildSeason` 可能涉及全量实体扫描与多次排序。当前步骤以保证正确性为主，性能优化留给后续。

## 完成后的下一步

进入 [步骤 6：集成测试与文档完善](step-06-integration-tests-docs.md)。
