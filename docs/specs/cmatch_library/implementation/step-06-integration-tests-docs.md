# 步骤 6：集成测试与文档完善

## 目标

通过端到端集成测试验证 CMatch Library 全部规格行为，完善项目文档与公共 API 注释。

## 原始规格引用

- `README.md`：核心概念与文档结构。
- `config.md`、`data_structures.md`、`user_interface.md`、`matching_algorithm.md`、`exception_handling.md`：全部规格内容。
- 项目根 `README.md`：当前为模板说明，需补充业务功能描述。

## 当前代码库基线

- 步骤 5 完成后，`TicketManager`、服务实现、异常处理均已落地。
- 单元测试已覆盖各模块核心行为。
- 项目根 `README.md` 仍为模板，未反映 CMatch Library 功能。
- 部分新增公共头文件可能缺少 Doxygen 注释或注释不完整。

## 开始前的校准清单

1. **集成测试范围**：确认需要覆盖的端到端场景。建议至少包括：
   - 多赛季配置加载。
   - 凭据提交、报名、分组。
   - 赛季切换与结算。
   - 跨赛季凭据修复。
2. **文档更新范围**：
   - 项目根 `README.md` 是否需要重写为 CMatch Library 的业务说明？
   - 是否新增 `docs/api.md` 或 `docs/usage.md`？
   - 公共头文件是否全部补充 Doxygen 注释？
3. **性能与压力测试**：是否在本步骤加入？当前建议不加入，作为后续优化项。
4. **Protobuf 生成代码的文档**：Doxygen 通常不扫描生成代码，无需处理。

## 需求

### R6.1 端到端集成测试

创建 `tests/cmatch/test_cmatch_integration.cpp`，构建一个完整的业务流程：

1. 准备内存配置实现（Mock `SeasonConfigInterface`）。
2. 准备内存实体管理器（Mock `TicketEntityManagerInterface`）。
3. 构造 `TicketManager` 与 `MatchGroupServiceDefault`。
4. 调用 `GetSeasonList` 验证赛季列表。
5. 调用 `SubmitTicket` 提交多个凭据。
6. 调用 `RegisterSeason` 为凭据报名赛事。
7. 调用 `BuildSeason` 构建分组。
8. 调用 `GetGroupMembers` 验证分组成员。
9. 模拟时间推进，调用 `NextSeason` 切换赛季。
10. 调用 `GetSettlementList` 验证结算数据。
11. 验证段位升降与积分重置行为。

### R6.2 边界场景集成测试

- 空配置：无任何赛季类型时，接口行为正确。
- 无凭据：`BuildSeason` 不崩溃。
- 合服场景：不同 `zone_id` 的凭据分组 ID 唯一。
- 跨赛季修复：凭据时间不一致时修复后行为正确。

### R6.3 更新项目根 README

将 `README.md` 从模板更新为 CMatch Library 项目说明，至少包含：

- 项目简介：CMatch Library 是什么、适用场景。
- 构建要求：CMake、Ninja、g++、Google Test、Protobuf、clang-format、clang-tidy。
- 快速开始：配置、构建、测试命令。
- 项目结构说明。
- 指向规格文档与实施路线的链接。

### R6.4 补充公共 API 注释

为以下公共头文件补充 Doxygen 风格简体中文注释：

- `src/cmatch/season_config_interface.h`
- `src/cmatch/ticket_entity_interface.h`
- `src/cmatch/ticket_entity_manager_interface.h`
- `src/cmatch/ticket_manager.h`
- `src/cmatch/match_group_service_impl.h`
- `src/cmatch/match_group_service_default.h`

每个公共类、方法、参数、返回值均需注释。

### R6.5 更新实施路线文档

- 在 `docs/specs/cmatch_library/implementation/README.md` 中记录实际完成日期与主要变更。
- 在 `todolist.md` 中将所有步骤标记为完成。

## 验收条件

- [ ] `cmake --build --preset ninja-debug` 成功。
- [ ] `ctest --preset ninja-debug` 全部测试通过（包括单元测试与集成测试）。
- [ ] `clang-format` 执行后无文件内容变更。
- [ ] `clang-tidy` 对全部 C++ 源文件无新增警告。
- [ ] 项目根 `README.md` 已更新，内容反映 CMatch Library 功能。
- [ ] 所有新增公共头文件均包含 Doxygen 风格简体中文注释。
- [ ] `todolist.md` 中所有步骤状态更新为 `[x]`。

## 预计交付文件

- 新增：
  - `tests/cmatch/test_cmatch_integration.cpp`
- 修改：
  - `README.md`
  - `src/cmatch/*.h`（补充注释）
  - `tests/CMakeLists.txt`（新增 `test_cmatch_integration` 可执行文件，源文件路径为 `cmatch/test_cmatch_integration.cpp`）
  - `docs/specs/cmatch_library/implementation/README.md`
  - `docs/specs/cmatch_library/implementation/todolist.md`

## 风险与注意事项

- **集成测试的稳定性**：集成测试依赖多个组件协同，任何模块改动都可能导致测试失败。需确保测试用例独立、可复现。
- **文档与代码同步**：README 与 API 注释必须与实际代码行为一致，避免文档过时。
- **全量 clang-tidy 检查**：本步骤应对整个 `src/` 运行 clang-tidy，可能暴露之前步骤遗漏的警告。需逐条修复。

## 完成后的状态

CMatch Library 全部规格已落地，文档完善，测试通过，实施路线完成。
