# 配置

## 赛季配置

使用 Protobuf 进行定义配置。

```protobuf
syntax = "proto3";

package cmatch.config;

// 段位信息
message GradeInfo
{
    // 段位编号
    uint32 grade = 1;
    // 上一个段位编号
    uint32 prev_grade = 2;
    // 下一个段位编号
    uint32 next_grade = 3;
    // 分组大小，等于0表示该段为只有一个分组
    uint32 group_size = 4;
    // 升段的排名，配置为0表示不升段
    uint32 promote_rank = 5;
    // 升段排名百分比，配置为0表示不升段
    float promote_rank_percent = 6;
    // 降段的排名，配置为0表示不降段
    uint32 demote_rank = 7;
    // 降段排名百分比，配置为0表示不降段
    float demote_rank_percent = 8;
}

// 赛事信息
message SeasonInfo
{
    // 赛事类型
    uint32 type = 1;
    // 段位信息列表
    map<uint32, GradeInfo> grades = 2;
    // 初始积分
    uint64 initial_score = 3;
    // 最小积分
    uint64 min_score = 4;
    // 积分的属性ID
    uint32 score_attr_id = 5;
    // 切换赛季后是否重置积分
    bool reset_score = 6;
    // 初始段位
    uint32 initial_grade = 7;
}

// 赛季时间
message SeasonTime
{
    // 赛季开始时间
    uint64 begin_time = 1;
    // 赛季结束时间
    uint64 end_time = 2;
}
```

## 配置接口

```c++

// 赛季配置接口
class SeasonConfigInterface
{
public:
    virtual ~SeasonConfigInterface() = default;

    // 获取所有赛季类型
    virtual std::vector<std::uint32_t> GetTypes() const = 0;
  
    // 获取赛季信息，返回是否找到该类型
    virtual bool GetInfo(std::uint32_t type, SeasonInfo& info) const = 0;

    // 获取赛季时间，返回是否找到该类型
    virtual bool GetTime(std::uint32_t type, SeasonTime& time) const = 0;
};
```

## 相关文档

- [返回总览](README.md)
- [数据结构](data_structures.md)
- [用户接口](user_interface.md)

