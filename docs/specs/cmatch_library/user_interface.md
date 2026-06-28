# 用户接口

## 服务定义

使用 Protobuf 进行定义用户接口。方便接入各类RPC框架。
这里只生成 Message 的定义，服务的实现由用户自己完成。用户可以使用任何 RPC 框架来实现服务。

```protobuf
syntax = "proto3";

package cmatch.lib;

// 匹配分组服务
service MatchGroupService
{
    // 获取赛事列表
    rpc GetSeasonList(GetSeasonListReq) returns (GetSeasonListResp);
    // 更新凭据
    rpc SubmitTicket(SubmitTicketReq) returns (SubmitTicketResp);
    // 获取凭据
    rpc GetTicket(GetTicketReq) returns (GetTicketResp);
    // 报名赛事
    rpc RegisterSeason(RegisterSeasonReq) returns (RegisterSeasonResp);
    // 获取凭据列表
    rpc GetTicketList(GetTicketListReq) returns (GetTicketListResp);
    // 获取分组成员列表
    rpc GetGroupMembers(GetGroupMembersReq) returns (GetGroupMembersResp);
    // 获取结算列表
    rpc GetSettlementList(GetSettlementListReq) returns (GetSettlementListResp);
    // 删除结算列表
    rpc RemoveSettlementList(RemoveSettlementListReq) returns (RemoveSettlementListResp);
}

// 返回结果码
enum Results
{
    // 成功
    OK = 0; 
    // 服务繁忙
    SYSTEM_BUSY = 1;
}

// 赛事
message Season
{
    // 赛事类型
    uint32 type = 1;
    // 赛季开始时间
    uint64 begin_time = 2;
    // 赛季结束时间
    uint64 end_time = 3;
}

// 凭据
message Ticket
{
    // 凭据ID
    uint64 id = 1;
    // 区域ID
    uint32 zone_id = 2;
    // 属性列表
    map<uint32, uint64> attributes = 3;
}

// 赛季分组
message SeasonGroup
{
    // 赛季类型
    uint32 type = 1;
    // 赛季开始时间
    uint64 begin_time = 2;
    // 赛季结束时间
    uint64 end_time = 3;
    // 段位编号
    uint32 grade = 4;
    // 分组编号
    uint64 group_id = 5;
}

// 结算信息
message SeasonSettlement
{
    // 赛季类型
    uint32 type = 1;
    // 赛季开始时间
    uint64 begin_time = 2;
    // 赛季结束时间
    uint64 end_time = 3;
    // 段位编号
    uint32 grade = 4;
    // 分组编号
    uint64 group_id = 5;
    // 结算排名
    uint32 rank = 6;
    // 结算排名百分比
    float rank_percent = 7;
    // 结算积分
    uint64 score = 8;
}

// 获取赛事列表请求
message GetSeasonListReq
{
}

// 获取赛事列表响应
message GetSeasonListResp
{
    // 返回结果码
    Results result = 1;
    // 赛事列表
    repeated Season seasons = 2;
}

// 更新凭据请求
message SubmitTicketReq
{
    // 凭据
    Ticket ticket = 1;
    // 参与的赛事类型列表（新增）
    repeated uint32 registrations = 2;
}

// 更新凭据响应
message SubmitTicketResp
{
    // 返回结果码
    Results result = 1;
}

// 获取凭据请求
message GetTicketReq
{
    // 凭据ID
    uint64 id = 1;
}

// 获取凭据响应
message GetTicketResp
{
    // 返回结果码
    Results result = 1;
    // 凭据
    Ticket ticket = 2;
    // 参与的赛事类型列表（查询）
    repeated uint32 registrations = 3;
    // 当前分组的赛事列表（查询）
    map<uint32, SeasonGroup> seasons = 4;
}

// 报名赛事请求
message RegisterSeasonReq
{
    // 凭据ID
    uint64 id = 1;
    // 赛事类型（新增）
    repeated uint32 types = 2;
}

// 报名赛事响应
message RegisterSeasonResp
{
    // 返回结果码
    Results result = 1;
}

// 获取凭据列表请求
message GetTicketListReq
{
    // 凭据ID列表
    repeated uint64 ids = 1;
}

// 获取凭据列表响应
message GetTicketListResp
{
    // 返回结果码
    Results result = 1;
    // 凭据列表
    repeated Ticket tickets = 2;
}

// 获取分组成员列表请求
message GetGroupMembersReq
{
    // 赛季类型
    uint32 type = 1;
    // 分组编号
    uint64 group_id = 2;
}

// 获取分组成员列表响应
message GetGroupMembersResp
{
    // 返回结果码
    Results result = 1;
    // 分组成员列表: key为凭据ID, value为凭据的区域ID
    map<uint64, uint32> members = 2;
}

// 获取结算列表请求
message GetSettlementListReq
{
    // 凭据ID
    uint64 id = 1;
}

// 获取结算列表响应
message GetSettlementListResp
{
    // 返回结果码
    Results result = 1;
    // 结算列表
    repeated SeasonSettlement settlements = 2;
}

// 删除结算列表请求
message RemoveSettlementListReq
{
    // 凭据ID
    uint64 id = 1;
    // 结算ID列表
    repeated uint64 settlement_ids = 2;
}

// 删除结算列表响应
message RemoveSettlementListResp
{
    // 返回结果码
    Results result = 1;
}
```

## 服务实现

```c++
// 匹配分组服务接口
class MatchGroupServiceImpl
{
public:
    virtual ~MatchGroupServiceImpl() = default;

    // 获取赛事列表
    void GetSeasonList(const std::shared_ptr<GetSeasonListReq>& request, std::function<void(const GetSeasonListResp&)> done) = 0;
    // 更新凭据
    void SubmitTicket(const std::shared_ptr<SubmitTicketReq>& request, std::function<void(const SubmitTicketResp&)> done) = 0;
    // 获取凭据
    void GetTicket(const std::shared_ptr<GetTicketReq>& request, std::function<void(const GetTicketResp&)> done) = 0;
    // 报名赛事
    void RegisterSeason(const std::shared_ptr<RegisterSeasonReq>& request, std::function<void(const RegisterSeasonResp&)> done) = 0;
    // 获取凭据列表
    void GetTicketList(const std::shared_ptr<GetTicketListReq>& request, std::function<void(const GetTicketListResp&)> done) = 0;
    // 获取分组成员列表
    void GetGroupMembers(const std::shared_ptr<GetGroupMembersReq>& request, std::function<void(const GetGroupMembersResp&)> done) = 0;
    // 获取结算列表
    void GetSettlementList(const std::shared_ptr<GetSettlementListReq>& request, std::function<void(const GetSettlementListResp&)> done) = 0;
    // 删除结算列表
    void RemoveSettlementList(const std::shared_ptr<RemoveSettlementListReq>& request, std::function<void(const RemoveSettlementListResp&)> done) = 0;
}

### 实体和实体管理器

```c++

// 凭据实体
class TicketEntityInterface
{
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

// 凭据实体管理器
class TicketEntityManagerInterface
{
public:
    virtual ~TicketEntityManagerInterface() = default;

    // 获取凭据实体
    virtual TicketEntityPtr GetEntity(std::uint64_t id) const = 0;
    virtual TicketEntityPtr GetOrCreateEntity(std::uint64_t id, std::uint32_t zone_id) = 0;
    virtual const std::unordered_map<std::uint64_t, TicketEntityPtr>& GetEntities() const = 0;

    // 设置凭据实体为脏数据，由实体管理器负责保存脏数据到数据库
    virtual void SetDirty(std::uint64_t id) = 0;

    // 判断实体管理器是否已加载
    virtual bool IsLoaded() const = 0;
};

// 凭据管理器
class TicketManager
{
public:
    explicit TicketManager(TicketEntityManagerInterface& entity_manager);
    ~TicketManager();

    // 当 TicketEntityManagerInterface 加载完成时调用。由外部根据 SeasonConfigInterface::GetTypes() 遍历每个赛事类型调用一次。
    void BuildSeason(const config::SeasonInfo& season_info, 
        const config::SeasonTime& season_time, 
        std::uint32_t now_time);
    // 切换赛季
    void NextSeason(const config::SeasonInfo& season_info, 
        const config::SeasonTime& season_time, 
        std::uint32_t now_time);
    // 新加入的凭据，需要在赛季中进行分组。由外部根据 SeasonConfigInterface::GetTypes() 遍历每个赛事类型调用一次。
    void AddTicket(const config::SeasonInfo& season_info, 
        const config::SeasonTime& season_time, 
        std::uint32_t now_time,
        std::uint64_t ticket_id);
}
```

## 相关文档

- [返回总览](README.md)
- [数据结构](data_structures.md)
- [匹配算法](matching_algorithm.md)

