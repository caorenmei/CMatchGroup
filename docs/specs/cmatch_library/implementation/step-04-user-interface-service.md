# 步骤 4：用户接口服务实现

## 目标

根据 `user_interface.md` 实现 `MatchGroupServiceImpl` 接口及全部 RPC 方法，向上层提供完整的匹配分组服务能力。

## 原始规格引用

- `user_interface.md`：
  - `service MatchGroupService` 与全部请求/响应消息。
  - `MatchGroupServiceImpl` 纯虚接口（8 个方法）。
  - `TicketManager` 构造依赖 `TicketEntityManagerInterface&`。

## 当前代码库基线

- 步骤 1 完成后，已存在 Protobuf 生成类型。
- 步骤 2 完成后，已存在核心接口与 Mock。
- 步骤 3 完成后，已存在 `TicketManager` 实现。
- 尚无服务接口的 `.proto` 文件与服务实现。

## 开始前的校准清单

1. **服务 `.proto` 文件位置**：`user_interface.md` 中的 `MatchGroupService` 请求/响应消息应放在 `proto/cmatch/lib.proto` 中，包名为 `cmatch.lib`。是否同时生成 gRPC 服务骨架？规格说明“只生成 Message 的定义，服务的实现由用户自己完成”，因此 `.proto` 中可只定义 `service` 与消息，但不依赖 gRPC 编译。
2. **回调风格**：规格中使用 `std::function<void(const Resp&)> done` 作为异步回调。确认是否保持该风格，还是简化为同步返回值。当前建议保持规格风格，便于接入各类 RPC 框架。
3. **`Results` 枚举的默认值**：`OK = 0` 是 Protobuf 枚举的默认返回值，需确保未显式设置时不会误报成功。
4. **`MatchGroupServiceImpl` 是否持有 `TicketManager`**：规格中 `TicketManager` 独立存在，服务实现可能通过引用或指针使用它。当前建议 `MatchGroupServiceImpl` 作为纯虚基类，具体实现类（如 `MatchGroupServiceDefault`）持有 `TicketManager` 与 `SeasonConfigInterface`。
5. **错误处理细化**：规格仅定义了 `OK` 与 `SYSTEM_BUSY`，但服务实现中会出现凭据不存在、赛事不存在、分组不存在等典型错误。实施时应在 `proto/cmatch/lib.proto` 的 `Results` 枚举中增加更丰富的错误码（如 `TICKET_NOT_FOUND`、`SEASON_NOT_FOUND` 等），以便调用方精确处理。原始规格文档保持只读，扩展通过实施文档落地。

## 需求

### R4.1 创建服务 Protobuf 文件

在 `proto/cmatch/lib.proto` 中定义：

```protobuf
syntax = "proto3";

package cmatch.lib;

// 返回结果码
enum Results {
    // 成功
    OK = 0;
    // 服务繁忙
    SYSTEM_BUSY = 1;
    // 凭据不存在
    TICKET_NOT_FOUND = 2;
    // 赛事不存在
    SEASON_NOT_FOUND = 3;
    // 分组不存在
    GROUP_NOT_FOUND = 4;
    // 结算不存在
    SETTLEMENT_NOT_FOUND = 5;
    // 请求参数无效
    INVALID_PARAMETER = 6;
    // 已报名该赛事
    ALREADY_REGISTERED = 7;
    // 内部错误
    INTERNAL_ERROR = 8;
}

message Season {
    uint32 type = 1;
    uint64 begin_time = 2;
    uint64 end_time = 3;
}

message Ticket {
    uint64 id = 1;
    uint32 zone_id = 2;
    map<uint32, uint64> attributes = 3;
}

message SeasonGroup {
    uint32 type = 1;
    uint64 begin_time = 2;
    uint64 end_time = 3;
    uint32 grade = 4;
    uint64 group_id = 5;
}

message SeasonSettlement {
    uint32 type = 1;
    uint64 begin_time = 2;
    uint64 end_time = 3;
    uint32 grade = 4;
    uint64 group_id = 5;
    uint32 rank = 6;
    float rank_percent = 7;
    uint64 score = 8;
}

message GetSeasonListReq {}
message GetSeasonListResp {
    Results result = 1;
    repeated Season seasons = 2;
}

message SubmitTicketReq {
    Ticket ticket = 1;
    repeated uint32 registrations = 2;
}
message SubmitTicketResp {
    Results result = 1;
}

message GetTicketReq {
    uint64 id = 1;
}
message GetTicketResp {
    Results result = 1;
    Ticket ticket = 2;
    repeated uint32 registrations = 3;
    map<uint32, SeasonGroup> seasons = 4;
}

message RegisterSeasonReq {
    uint64 id = 1;
    repeated uint32 types = 2;
}
message RegisterSeasonResp {
    Results result = 1;
}

message GetTicketListReq {
    repeated uint64 ids = 1;
}
message GetTicketListResp {
    Results result = 1;
    repeated Ticket tickets = 2;
}

message GetGroupMembersReq {
    uint32 type = 1;
    uint64 group_id = 2;
}
message GetGroupMembersResp {
    Results result = 1;
    map<uint64, uint32> members = 2;
}

message GetSettlementListReq {
    uint64 id = 1;
}
message GetSettlementListResp {
    Results result = 1;
    repeated SeasonSettlement settlements = 2;
}

message RemoveSettlementListReq {
    uint64 id = 1;
    repeated uint64 settlement_ids = 2;
}
message RemoveSettlementListResp {
    Results result = 1;
}

service MatchGroupService {
    rpc GetSeasonList(GetSeasonListReq) returns (GetSeasonListResp);
    rpc SubmitTicket(SubmitTicketReq) returns (SubmitTicketResp);
    rpc GetTicket(GetTicketReq) returns (GetTicketResp);
    rpc RegisterSeason(RegisterSeasonReq) returns (RegisterSeasonResp);
    rpc GetTicketList(GetTicketListReq) returns (GetTicketListResp);
    rpc GetGroupMembers(GetGroupMembersReq) returns (GetGroupMembersResp);
    rpc GetSettlementList(GetSettlementListReq) returns (GetSettlementListResp);
    rpc RemoveSettlementList(RemoveSettlementListReq) returns (RemoveSettlementListResp);
}
```

### R4.2 定义服务接口

在 `src/cmatch/match_group_service_impl.h` 中定义纯虚接口：

```cpp
#ifndef CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
#define CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_

#include <functional>
#include <memory>

#include "cmatch/lib.pb.h"

namespace cmatch {

class MatchGroupServiceImpl {
 public:
  virtual ~MatchGroupServiceImpl() = default;

  virtual void GetSeasonList(
      const std::shared_ptr<lib::GetSeasonListReq>& request,
      std::function<void(const lib::GetSeasonListResp&)> done) = 0;

  virtual void SubmitTicket(
      const std::shared_ptr<lib::SubmitTicketReq>& request,
      std::function<void(const lib::SubmitTicketResp&)> done) = 0;

  virtual void GetTicket(
      const std::shared_ptr<lib::GetTicketReq>& request,
      std::function<void(const lib::GetTicketResp&)> done) = 0;

  virtual void RegisterSeason(
      const std::shared_ptr<lib::RegisterSeasonReq>& request,
      std::function<void(const lib::RegisterSeasonResp&)> done) = 0;

  virtual void GetTicketList(
      const std::shared_ptr<lib::GetTicketListReq>& request,
      std::function<void(const lib::GetTicketListResp&)> done) = 0;

  virtual void GetGroupMembers(
      const std::shared_ptr<lib::GetGroupMembersReq>& request,
      std::function<void(const lib::GetGroupMembersResp&)> done) = 0;

  virtual void GetSettlementList(
      const std::shared_ptr<lib::GetSettlementListReq>& request,
      std::function<void(const lib::GetSettlementListResp&)> done) = 0;

  virtual void RemoveSettlementList(
      const std::shared_ptr<lib::RemoveSettlementListReq>& request,
      std::function<void(const lib::RemoveSettlementListResp&)> done) = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
```

### R4.3 实现默认服务类

在 `src/cmatch/match_group_service_default.h` 与 `.cpp` 中实现具体服务类：

```cpp
class MatchGroupServiceDefault : public MatchGroupServiceImpl {
 public:
  MatchGroupServiceDefault(SeasonConfigInterface& config,
                           TicketEntityManagerInterface& entity_manager,
                           TicketManager& ticket_manager);
  // ... 实现 8 个 RPC 方法
};
```

各方法行为：

- **GetSeasonList**：遍历 `SeasonConfigInterface::GetTypes()`，对每个类型调用 `GetTime`，填充 `Season` 列表。无配置时返回空列表与 `OK`。
- **SubmitTicket**：校验请求中的 `ticket.id` 与 `zone_id` 有效性；通过 `entity_manager.GetOrCreateEntity(id, zone_id)` 获取/创建实体，更新 `attributes`、`registrations`，调用 `SetDirty`。参数无效时返回 `INVALID_PARAMETER`。
- **GetTicket**：通过 `entity_manager.GetEntity(id)` 获取实体；若不存在返回 `TICKET_NOT_FOUND`。
- **RegisterSeason**：获取实体，不存在返回 `TICKET_NOT_FOUND`；校验每个赛事类型是否有效，无效返回 `SEASON_NOT_FOUND`；跳过已报名的类型，若全部已报名可返回 `ALREADY_REGISTERED`（或 `OK`，需在实现前明确）。更新 `registrations` 后调用 `SetDirty`。
- **GetTicketList**：批量查询 `Ticket`（不包含 `registrations` 与 `seasons`，按规格）。不存在的 ID 跳过，整体返回 `OK`。
- **GetGroupMembers**：遍历实体管理器中所有实体，收集指定 `type` 与 `group_id` 的凭据，返回 `members` map。若没有任何成员，返回 `OK` 与空 map（或 `GROUP_NOT_FOUND`，需在实现前明确）。
- **GetSettlementList**：获取实体，不存在返回 `TICKET_NOT_FOUND`；返回指定凭据的所有 `settlements`。
- **RemoveSettlementList**：获取实体，不存在返回 `TICKET_NOT_FOUND`；从凭据的 `settlements` map 中删除指定 `settlement_id`（key）。不存在的 key 可忽略。调用 `SetDirty`。返回 `OK`。

### R4.4 测试

创建 `tests/cmatch/test_match_group_service.cpp`，覆盖：

- `GetSeasonList` 返回所有配置的赛季。
- `SubmitTicket` 创建凭据并设置属性。
- `SubmitTicket` 参数无效时返回 `INVALID_PARAMETER`。
- `GetTicket` 查询凭据，验证 `registrations` 与 `seasons`。
- `GetTicket` 凭据不存在时返回 `TICKET_NOT_FOUND`。
- `RegisterSeason` 为凭据报名赛事。
- `RegisterSeason` 凭据不存在时返回 `TICKET_NOT_FOUND`。
- `RegisterSeason` 赛事类型无效时返回 `SEASON_NOT_FOUND`。
- `GetTicketList` 批量查询，跳过不存在的 ID。
- `GetGroupMembers` 返回正确成员 map。
- `GetSettlementList` 与 `RemoveSettlementList` 操作结算数据。
- `GetSettlementList` 凭据不存在时返回 `TICKET_NOT_FOUND`。

## 验收条件

- [ ] `cmake --build --preset ninja-debug` 成功，新增 Protobuf 消息编译通过。
- [ ] `ctest --preset ninja-debug` 中新增服务测试全部通过。
- [ ] `clang-format` 执行后无文件内容变更。
- [ ] `clang-tidy` 对新增 C++ 文件无新增警告。
- [ ] 所有新增公共头文件包含 Doxygen 风格简体中文注释。

## 预计交付文件

- 新增：
  - `proto/cmatch/lib.proto`
  - `src/cmatch/match_group_service_impl.h`
  - `src/cmatch/match_group_service_default.h`
  - `src/cmatch/match_group_service_default.cpp`
  - `tests/cmatch/test_match_group_service.cpp`
- 修改：
  - `src/CMakeLists.txt`（加入 `match_group_service_default.cpp`）
  - `tests/CMakeLists.txt`（新增 `test_match_group_service` 可执行文件，源文件路径为 `cmatch/test_match_group_service.cpp`）

## 风险与注意事项

- **服务层与算法层职责划分**：服务层只负责请求/响应转换与实体管理调用，不直接实现匹配算法；算法由 `TicketManager` 负责。
- **`GetGroupMembers` 性能**：当前实现为全量扫描实体。若实体数量大，后续可能需要按 `group_id` 建立索引，但本步骤不实现。
- **`RemoveSettlementList` 的语义**：规格中 `settlement_ids` 对应 `SeasonSettlement` 的 key。由于 `SeasonSettlement` 没有独立 ID 字段，应使用 `map<uint32, SeasonSettlement>` 的 key（即赛季类型）作为删除标识。
- **错误码设计**：本步骤在规格基础上扩展了 `Results` 枚举。实现前应最终确认每个 RPC 方法在各类错误场景下返回的具体错误码（如 `GetGroupMembers` 空结果是否返回 `GROUP_NOT_FOUND`），避免调用方语义歧义。

## 完成后的下一步

进入 [步骤 5：异常处理与数据修复](step-05-exception-handling-repair.md)。
