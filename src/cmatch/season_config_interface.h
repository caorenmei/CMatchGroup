// 赛季配置接口

#ifndef CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_
#define CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_

#include <cstdint>
#include <vector>

#include "cmatch/config.pb.h"

namespace cmatch {

/// @brief 赛季配置接口
///
/// 提供赛事类型、赛季信息与赛季时间的查询能力。
/// 实现类负责从配置中心或本地配置加载并维护赛季数据。
class SeasonConfigInterface {
 public:
  virtual ~SeasonConfigInterface() = default;

  /// @brief 获取所有已配置的赛季类型
  /// @return 赛季类型列表
  virtual std::vector<std::uint32_t> GetTypes() const = 0;

  /// @brief 获取指定类型的赛季信息
  /// @param[in] type 赛季类型
  /// @param[out] info 赛季信息输出参数
  /// @return 是否找到该类型
  virtual bool GetInfo(std::uint32_t type, config::SeasonInfo& info) const = 0;

  /// @brief 获取指定类型的赛季时间
  /// @param[in] type 赛季类型
  /// @param[out] time 赛季时间输出参数
  /// @return 是否找到该类型
  virtual bool GetTime(std::uint32_t type, config::SeasonTime& time) const = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_SEASON_CONFIG_INTERFACE_H_
