// 赛季配置接口

#ifndef CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_
#define CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_

#include <cstdint>
#include <vector>

#include "cmatch/config.pb.h"

namespace cmatch {

// 赛季配置接口，提供赛事类型、赛季信息与赛季时间的查询能力
class SeasonConfigInterface {
 public:
  virtual ~SeasonConfigInterface() = default;

  // 获取所有赛季类型
  virtual std::vector<std::uint32_t> GetTypes() const = 0;

  // 获取赛季信息，返回是否找到该类型
  virtual bool GetInfo(std::uint32_t type, config::SeasonInfo& info) const = 0;

  // 获取赛季时间，返回是否找到该类型
  virtual bool GetTime(std::uint32_t type, config::SeasonTime& time) const = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_
