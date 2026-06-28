// 凭据管理器

#ifndef CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
#define CMATCH_SRC_CMATCH_TICKET_MANAGER_H_

#include <algorithm>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cmatch/config.pb.h"
#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_entity_manager_interface.h"

namespace cmatch {

/// @brief 凭据管理器
///
/// 负责赛季分组、结算、段位升降等核心匹配算法。
/// 所有公共方法均为线程不安全，需由调用方保证同步。
class TicketManager {
 public:
  /// @brief 构造函数
  /// @param[in] entity_manager 凭据实体管理器引用
  explicit TicketManager(TicketEntityManagerInterface& entity_manager);

  ~TicketManager() = default;

  /// @brief 实体加载完成后，遍历每个赛事类型调用一次
  ///
  /// 根据当前时间将凭据分为“在当前赛季内”与“不在当前赛季内”两类：
  /// - 在当前赛季内：按段位重新随机分组。
  /// -
  /// 不在当前赛季内：按原分组结算、计算新段位、优先填入当前赛季同段位未满分组。
  /// 同时修复凭据上的赛季时间与配置不一致的问题。
  /// @param[in] config 赛季配置接口
  /// @param[in] now_time 当前时间
  /// @param[in,out] rng 随机数引擎
  void Initialize(const SeasonConfigInterface& config, std::uint32_t now_time,
                  std::mt19937& rng);

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

  /// @brief 新加入的凭据，由外部调用一次
  ///
  /// 内部遍历所有已配置的赛事类型，为凭据分配初始段位并优先填入同段位未满旧分组；
  /// 断言新凭据上不存在任何赛季分组数据。
  /// @param[in] config 赛季配置接口
  /// @param[in] ticket_id 新凭据 ID
  /// @param[in,out] rng 随机数引擎
  void AddTicket(const SeasonConfigInterface& config, std::uint64_t ticket_id,
                 std::mt19937& rng);

  /// @brief 获取指定分组内的凭据 ID 列表
  /// @param[in] season_type 赛季类型
  /// @param[in] group_id 分组 ID
  /// @return 凭据 ID 列表（顺序不做保证）
  std::vector<std::uint64_t> GetGroupTicketIds(std::uint32_t season_type,
                                               std::uint64_t group_id) const;

 private:
  // 分组 ID 分配器，高 32 位为 zone_id，低 32 位为自增编号
  struct GroupIdAllocator {
    std::uint32_t next_sequence_ = 1;

    void Initialize(
        const std::unordered_map<std::uint64_t, TicketEntityPtr>& entities) {
      auto max_sequence = std::uint32_t{0};
      for (const auto& [id, entity] : entities) {
        (void)id;
        const auto& ticket = entity->GetData();
        for (const auto& [type, group] : ticket.seasons()) {
          (void)type;
          max_sequence = std::max(max_sequence,
                                  static_cast<std::uint32_t>(group.group_id()));
        }
        for (const auto& [type, settlement] : ticket.settlements()) {
          (void)type;
          max_sequence = std::max(
              max_sequence, static_cast<std::uint32_t>(settlement.group_id()));
        }
      }
      next_sequence_ = max_sequence + 1;
    }

    std::uint64_t Allocate(std::uint32_t zone_id) {
      auto group_id =
          (static_cast<std::uint64_t>(zone_id) << 32) | next_sequence_;
      ++next_sequence_;
      return group_id;
    }
  };

  // 分组槽位，延迟写入 SeasonGroup
  struct GroupSlot {
    std::uint64_t group_id = 0;
    std::uint32_t grade = 0;
    std::vector<TicketEntityPtr> entities;
  };

  static std::uint32_t ComputeNextGrade(const config::GradeInfo& grade_info,
                                        std::uint32_t rank, float rank_percent);

  void SettleGroup(const std::vector<TicketEntityPtr>& group,
                   std::uint32_t season_type,
                   const config::SeasonTime& season_time, std::uint32_t grade,
                   std::uint32_t score_attr_id);

  void FormGroupSlots(std::vector<TicketEntityPtr> tickets,
                      std::uint32_t group_size, std::uint32_t grade,
                      std::mt19937& rng, std::vector<GroupSlot>& slots);

  void WriteSeasonGroups(std::vector<GroupSlot>& slots,
                         std::uint32_t season_type,
                         const config::SeasonInfo& season_info,
                         const config::SeasonTime& season_time);

  /// @brief 逐个赛事类型完成初始化
  /// @param[in] config 赛季配置接口
  /// @param[in] season_type 赛季类型
  /// @param[in] now_time 当前时间
  /// @param[in,out] rng 随机数引擎
  void InitializeSeasonType(const SeasonConfigInterface& config,
                            std::uint32_t season_type, std::uint32_t now_time,
                            std::mt19937& rng);

  /// @brief 根据当前凭据数据重建分组索引
  /// @param[in] config 赛季配置接口
  void RebuildGroupIndex(const SeasonConfigInterface& config);

  /// @brief 为单个赛事类型处理新凭据
  /// @param[in] entity 凭据实体
  /// @param[in] season_type 赛季类型
  /// @param[in] season_info 赛季信息
  /// @param[in] season_time 赛季时间
  /// @param[in,out] rng 随机数引擎
  void AddTicketToSeason(const TicketEntityPtr& entity,
                         std::uint32_t season_type,
                         const config::SeasonInfo& season_info,
                         const config::SeasonTime& season_time,
                         std::mt19937& rng);

  /// @brief 将单个凭据按指定段位分配到同段位未满旧分组，或创建新分组
  /// @param[in] entity 凭据实体
  /// @param[in] season_type 赛季类型
  /// @param[in] grade 目标段位
  /// @param[in] season_info 赛季信息
  /// @param[in] season_time 新赛季时间
  /// @param[in,out] rng 随机数引擎
  void AssignTicketToGradeGroup(const TicketEntityPtr& entity,
                                std::uint32_t season_type, std::uint32_t grade,
                                const config::SeasonInfo& season_info,
                                const config::SeasonTime& season_time,
                                std::mt19937& rng);

  // 用于 pair 键的哈希辅助结构
  struct PairHash {
    std::size_t operator()(
        const std::pair<std::uint32_t, std::uint32_t>& p) const noexcept {
      return (std::hash<std::uint32_t>{}(p.first) * 31) +
             std::hash<std::uint32_t>{}(p.second);
    }
  };

  // 分组索引
  std::unordered_map<std::pair<std::uint32_t, std::uint32_t>,
                     std::unordered_set<std::uint64_t>, PairHash>
      season_grade_to_groups_;
  std::unordered_map<std::uint64_t, std::unordered_set<std::uint64_t>>
      group_to_tickets_;
  std::unordered_map<std::pair<std::uint32_t, std::uint32_t>,
                     std::unordered_set<std::uint64_t>, PairHash>
      unfilled_season_grade_groups_;

  TicketEntityManagerInterface& entity_manager_;
  GroupIdAllocator group_id_allocator_;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
