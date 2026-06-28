# CMatch Library

## 核心概念与名词

- 赛季（Season）：由开始时间和结束时间组成。
- 凭据（Ticket）：参与匹配的数据结构。
- 积分（Score）：凭据的分数，用于衡量实力。
- 段位（Grade）：参与匹配的等级，相同段位在一起分组匹配。
- 分组（Group）：同一段位的凭据会被分组，分组内的凭据会进行匹配。
- 排行（Rank）：同一分组内的凭据根据积分进行排序。
- 时间（Time）：时间戳，单位为秒。

## 文档结构

本文档按照功能模块将 CMatch Library 的详细规范拆分为以下子文档：

- [配置](config.md)：赛季配置 Protobuf 定义与配置接口。
- [数据结构](data_structures.md)：匹配凭据的 Protobuf 定义。
- [用户接口](user_interface.md)：服务定义、服务实现与实体管理器接口。
- [匹配算法](matching_algorithm.md)：赛季判断、结算、分组与段位升降规则。
- [异常处理](exception_handling.md)：实体数据修复与赛季时间修复。
