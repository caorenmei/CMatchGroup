# 步骤 2：核心接口定义

## 目标

根据 `config.md` 与 `user_interface.md` 定义 `SeasonConfigInterface`、`TicketEntityInterface` 和 `TicketEntityManagerInterface`，并提供可用于单元测试的内存 mock 实现。

## 原始规格引用

- `config.md`：
  - `SeasonConfigInterface`
    - `GetTypes() const`
    - `GetInfo(std::uint32_t type, SeasonInfo& info) const`
    - `GetTime(std::uint32_t type, SeasonTime& time) const`
- `user_interface.md`：
  - `TicketEntityInterface`
    - `GetKey() const`
    - `GetData()` / `GetData() const`
    - `IsFull() const`
  - `TicketEntityManagerInterface`
    - `GetEntity(std::uint64_t id) const`
    - `GetOrCreateEntity(std::uint64_t id, std::uint32_t zone_id)`
    - `GetEntities() const`
    - `SetDirty(std::uint64_t id)`
    - `IsLoaded() const`

## 当前代码库基线

- 步骤 1 完成后，应已存在 `cmatch::config::SeasonInfo`、`cmatch::config::SeasonTime`、`cmatch::table::Ticket` 等 Protobuf 生成类型。
- `src/cmatch/` 目录是库的公共头文件与实现目录。
- 当前尚无业务接口定义。

## 开始前的校准清单

1. **Protobuf 类型的实际命名空间**：确认生成代码中 `SeasonInfo` 与 `SeasonTime` 是否确实在 `cmatch::config` 命名空间下，`Ticket` 是否在 `cmatch::table` 下。若不是，调整接口中的类型引用。
2. **引用还是值传递**：规格中 `GetInfo` 与 `GetTime` 使用输出参数（`SeasonInfo& info`）。请确认是否需要改为返回值（`std::optional<SeasonInfo>`）以更符合现代 C++ 风格。当前建议保留规格中的输出参数签名，以维持与原文档一致。
3. **线程安全**：当前规格未明确要求线程安全。若后续服务实现需要并发访问实体管理器，应在本步骤的接口中预留锁的扩展点（如文档注释中说明）。
4. **`TicketEntityPtr` 类型**：规格定义 `using TicketEntityPtr = std::shared_ptr<TicketEntityInterface>;`。确认是否使用 `std::shared_ptr` 还是 `std::unique_ptr`。当前建议按规格使用 `std::shared_ptr`。

## 需求

### R2.1 定义配置接口

在 `src/cmatch/season_config_interface.h` 中定义纯虚接口：

```cpp
#ifndef CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_
#define CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_

#include <cstdint>
#include <vector>

#include "cmatch/config.pb.h"

namespace cmatch {

// 赛季配置接口
class SeasonConfigInterface {
 public:
  virtual ~SeasonConfigInterface() = default;

  // 获取所有赛季类型
  virtual std::vector<std::uint32_t> GetTypes() const = 0;

  // 获取赛季信息，返回是否找到该类型
  virtual bool GetInfo(std::uint32_t type,
                       config::SeasonInfo& info) const = 0;

  // 获取赛季时间，返回是否找到该类型
  virtual bool GetTime(std::uint32_t type,
                       config::SeasonTime& time) const = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_
```

### R2.2 定义凭据实体接口

在 `src/cmatch/ticket_entity_interface.h` 中定义：

```cpp
#ifndef CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_
#define CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_

#include <cstdint>
#include <memory>

#include "cmatch/table.pb.h"

namespace cmatch {

// 凭据实体接口
class TicketEntityInterface {
 public:
  virtual ~TicketEntityInterface() = default;

  // 获取凭据ID
  virtual std::uint64_t GetKey() const = 0;

  // 获取凭据数据
  virtual table::Ticket& GetData() = 0;
  virtual const table::Ticket& GetData() const = 0;

  // 判断凭据是否有效
  virtual bool IsFull() const = 0;
};

using TicketEntityPtr = std::shared_ptr<TicketEntityInterface>;

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_
```

### R2.3 定义凭据实体管理器接口

在 `src/cmatch/ticket_entity_manager_interface.h` 中定义：

```cpp
#ifndef CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_
#define CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_

#include <cstdint>
#include <unordered_map>

#include "cmatch/ticket_entity_interface.h"

namespace cmatch {

// 凭据实体管理器接口
class TicketEntityManagerInterface {
 public:
  virtual ~TicketEntityManagerInterface() = default;

  // 获取凭据实体
  virtual TicketEntityPtr GetEntity(std::uint64_t id) const = 0;
  virtual TicketEntityPtr GetOrCreateEntity(std::uint64_t id,
                                            std::uint32_t zone_id) = 0;
  virtual const std::unordered_map<std::uint64_t, TicketEntityPtr>&
  GetEntities() const = 0;

  // 设置凭据实体为脏数据，由实体管理器负责保存脏数据到数据库
  virtual void SetDirty(std::uint64_t id) = 0;

  // 判断实体管理器是否已加载
  virtual bool IsLoaded() const = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_
```

### R2.4 提供内存 Mock 实现

为支持单元测试，在 `tests/cmatch/mock_ticket_entity_manager.h` 中提供基于 `std::unordered_map` 的内存实现：

- `MockTicketEntity`：实现 `TicketEntityInterface`，内部持有 `table::Ticket`。
- `MockTicketEntityManager`：实现 `TicketEntityManagerInterface`，内部持有 `std::unordered_map<std::uint64_t, TicketEntityPtr>`。
- 支持 `SetDirty` 记录脏数据集合（如 `std::unordered_set<std::uint64_t>`）。

## 验收条件

- [ ] `cmake --build --preset ninja-debug` 成功，接口头文件可被 `src/` 与 `tests/` 正常 include。
- [ ] `ctest --preset ninja-debug` 中新增测试全部通过，至少覆盖：
  - `MockTicketEntity` 的 `GetKey`、`GetData`、`IsFull` 行为。
  - `MockTicketEntityManager` 的 `GetOrCreateEntity`、`GetEntities`、`SetDirty`、`IsLoaded` 行为。
  - `SeasonConfigInterface` 的纯虚 mock 可正常派生。
- [ ] `clang-format` 执行后无文件内容变更。
- [ ] `clang-tidy` 对新增 C++ 文件无新增警告。
- [ ] 所有新增公共头文件包含 Doxygen 风格简体中文注释。

## 预计交付文件

- 新增：
  - `src/cmatch/season_config_interface.h`
  - `src/cmatch/ticket_entity_interface.h`
  - `src/cmatch/ticket_entity_manager_interface.h`
  - `tests/cmatch/mock_ticket_entity_manager.h`
  - `tests/cmatch/test_core_interfaces.cpp`
- 修改：
  - `src/cmatch/CMakeLists.txt`（若接口需要源文件实现；若全为头文件，可能无需修改）
  - `tests/cmatch/CMakeLists.txt`（新增 `test_core_interfaces` 可执行文件，源文件路径为 `test_core_interfaces.cpp`）

## 风险与注意事项

- **接口签名的兼容性**：步骤 3 将直接依赖这些接口签名。任何签名变更都必须在步骤 3 开始前完成。
- **`IsFull` 的语义**：规格未详细说明“有效”的判断标准。Mock 实现可先默认返回 `true`；步骤 3 中根据业务需求补充。
- **实体管理器生命周期**：`TicketManager` 将持有 `TicketEntityManagerInterface&` 引用（见 `user_interface.md`）。Mock 实现应确保引用有效性。
- **`GetOrCreateEntity` 的 zone_id 参数**：创建新实体时，应将 `zone_id` 写入 `table::Ticket::set_zone_id(id)`。

## 完成后的下一步

进入 [步骤 3：TicketManager 匹配算法](step-03-ticket-manager-algorithm.md)。
