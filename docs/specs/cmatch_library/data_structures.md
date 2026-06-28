# 数据结构

## 匹配凭据

使用 Protobuf 进行定义匹配凭据。

```protobuf
syntax = "proto3";

package cmatch.table;

// 凭据信息
message Ticket
{
    // 凭据ID
    uint64 id = 1;
    // 区域ID
    uint32 zone_id = 2;
    // 凭据的自增编号, 凭据内部唯一
    uint32 auto_id = 3;
    // 属性列表
    map<uint32, uint64> attributes = 4;
    // 报名的赛事类型列表
    repeated uint32 registrations = 5;
    // 赛季分组
    map<uint32, SeasonGroup> seasons = 6;
    // 结算信息
    map<uint32, SeasonSettlement> settlements = 7;
}

// 赛季分组
message SeasonGroup
{
    // 赛季类型
    uint32 type = 1;
    // 赛事开始时间
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
```

## 相关文档

- [返回总览](README.md)
- [配置](config.md)
- [用户接口](user_interface.md)

