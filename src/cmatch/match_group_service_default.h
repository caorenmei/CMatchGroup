// 匹配分组服务默认实现

#ifndef CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_DEFAULT_H_
#define CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_DEFAULT_H_

#include <functional>
#include <memory>

#include "cmatch/lib.pb.h"
#include "cmatch/match_group_service_impl.h"
#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_entity_manager_interface.h"
#include "cmatch/ticket_manager.h"

namespace cmatch {

// 匹配分组服务默认实现
class MatchGroupServiceDefault : public MatchGroupServiceImpl {
 public:
  MatchGroupServiceDefault(SeasonConfigInterface& config,
                           TicketEntityManagerInterface& entity_manager,
                           TicketManager& ticket_manager);

  void GetSeasonList(
      const std::shared_ptr<lib::GetSeasonListReq>& request,
      std::function<void(const lib::GetSeasonListResp&)> done) override;

  void SubmitTicket(
      const std::shared_ptr<lib::SubmitTicketReq>& request,
      std::function<void(const lib::SubmitTicketResp&)> done) override;

  void GetTicket(const std::shared_ptr<lib::GetTicketReq>& request,
                 std::function<void(const lib::GetTicketResp&)> done) override;

  void RegisterSeason(
      const std::shared_ptr<lib::RegisterSeasonReq>& request,
      std::function<void(const lib::RegisterSeasonResp&)> done) override;

  void GetTicketList(
      const std::shared_ptr<lib::GetTicketListReq>& request,
      std::function<void(const lib::GetTicketListResp&)> done) override;

  void GetGroupMembers(
      const std::shared_ptr<lib::GetGroupMembersReq>& request,
      std::function<void(const lib::GetGroupMembersResp&)> done) override;

  void GetSettlementList(
      const std::shared_ptr<lib::GetSettlementListReq>& request,
      std::function<void(const lib::GetSettlementListResp&)> done) override;

  void RemoveSettlementList(
      const std::shared_ptr<lib::RemoveSettlementListReq>& request,
      std::function<void(const lib::RemoveSettlementListResp&)> done) override;

 private:
  SeasonConfigInterface& config_;
  TicketEntityManagerInterface& entity_manager_;
  TicketManager& ticket_manager_;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_DEFAULT_H_
