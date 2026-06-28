// 凭据管理器

#ifndef CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
#define CMATCH_SRC_CMATCH_TICKET_MANAGER_H_

#include <cstdint>
#include <memory>
#include <random>

#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_entity_manager_interface.h"

namespace cmatch {

// 凭据管理器，负责赛季分组、结算与段位升降
class TicketManager {
 public:
  explicit TicketManager(TicketEntityManagerInterface& entity_manager);
  ~TicketManager();

  // 实体加载完成后，遍历每个赛事类型调用一次
  void BuildSeason(const config::SeasonInfo& season_info,
                   const config::SeasonTime& season_time,
                   std::uint32_t now_time, std::mt19937& rng);

  // 切换赛季
  void NextSeason(const config::SeasonInfo& season_info,
                  const config::SeasonTime& season_time, std::uint32_t now_time,
                  std::mt19937& rng);

  // 新加入的凭据，遍历每个赛事类型调用一次
  void AddTicket(const config::SeasonInfo& season_info,
                 const config::SeasonTime& season_time, std::uint32_t now_time,
                 std::uint64_t ticket_id, std::mt19937& rng);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_MANAGER_H_
