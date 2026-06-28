// 凭据管理器

#ifndef CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
#define CMATCH_SRC_CMATCH_TICKET_MANAGER_H_

#include <cstdint>
#include <memory>
#include <random>

#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_entity_manager_interface.h"

namespace cmatch {

/// @brief 凭据管理器
///
/// 负责赛季分组、结算、段位升降等核心匹配算法。
/// 采用 Pimpl
/// 模式隐藏实现细节，所有公共方法均为线程不安全，需由调用方保证同步。
class TicketManager {
 public:
  /// @brief 构造函数
  /// @param[in] entity_manager 凭据实体管理器引用
  explicit TicketManager(TicketEntityManagerInterface& entity_manager);

  ~TicketManager();

  /// @brief 实体加载完成后，遍历每个赛事类型调用一次
  ///
  /// 根据当前时间将凭据分为“在当前赛季内”与“不在当前赛季内”两类：
  /// - 在当前赛季内：按段位重新随机分组。
  /// -
  /// 不在当前赛季内：按原分组结算、计算新段位、优先填入当前赛季同段位未满分组。
  /// 同时修复凭据上的赛季时间与配置不一致的问题。
  /// @param[in] season_info 赛季信息
  /// @param[in] season_time 赛季时间
  /// @param[in] now_time 当前时间
  /// @param[in,out] rng 随机数引擎
  void BuildSeason(const config::SeasonInfo& season_info,
                   const config::SeasonTime& season_time,
                   std::uint32_t now_time, std::mt19937& rng);

  /// @brief 切换赛季
  ///
  /// 对指定赛季类型的所有凭据按当前分组结算，计算新段位后重新分组。
  /// 若配置 reset_score 为 true，则将凭据积分重置为 initial_score。
  /// @param[in] season_info 赛季信息
  /// @param[in] season_time 新赛季时间
  /// @param[in] now_time 当前时间
  /// @param[in,out] rng 随机数引擎
  void NextSeason(const config::SeasonInfo& season_info,
                  const config::SeasonTime& season_time, std::uint32_t now_time,
                  std::mt19937& rng);

  /// @brief 新加入的凭据，遍历每个赛事类型调用一次
  ///
  /// 若凭据无对应赛季分组，则分配初始段位并创建新的 SeasonGroup；
  /// 若已存在赛季分组，则确保其时间与配置一致。
  /// @param[in] season_info 赛季信息
  /// @param[in] season_time 赛季时间
  /// @param[in] now_time 当前时间
  /// @param[in] ticket_id 新凭据 ID
  /// @param[in,out] rng 随机数引擎
  void AddTicket(const config::SeasonInfo& season_info,
                 const config::SeasonTime& season_time, std::uint32_t now_time,
                 std::uint64_t ticket_id, std::mt19937& rng);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
