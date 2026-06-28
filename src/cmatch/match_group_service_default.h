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

/// @brief 匹配分组服务默认实现
///
/// 实现 MatchGroupServiceImpl 接口，负责请求/响应转换与实体管理调用。
/// 匹配算法由 TicketManager 负责，服务层不直接实现算法逻辑。
class MatchGroupServiceDefault : public MatchGroupServiceImpl {
 public:
  /// @brief 构造函数
  /// @param[in] config 赛季配置接口
  /// @param[in] entity_manager 凭据实体管理器接口
  /// @param[in] ticket_manager 凭据管理器
  MatchGroupServiceDefault(SeasonConfigInterface& config,
                           TicketEntityManagerInterface& entity_manager,
                           TicketManager& ticket_manager);

  /// @copydoc MatchGroupServiceImpl::GetSeasonList
  void GetSeasonList(
      const std::shared_ptr<lib::GetSeasonListReq>& request,
      std::function<void(const lib::GetSeasonListResp&)> done) override;

  /// @copydoc MatchGroupServiceImpl::SubmitTicket
  void SubmitTicket(
      const std::shared_ptr<lib::SubmitTicketReq>& request,
      std::function<void(const lib::SubmitTicketResp&)> done) override;

  /// @copydoc MatchGroupServiceImpl::GetTicket
  void GetTicket(const std::shared_ptr<lib::GetTicketReq>& request,
                 std::function<void(const lib::GetTicketResp&)> done) override;

  /// @copydoc MatchGroupServiceImpl::RegisterSeason
  void RegisterSeason(
      const std::shared_ptr<lib::RegisterSeasonReq>& request,
      std::function<void(const lib::RegisterSeasonResp&)> done) override;

  /// @copydoc MatchGroupServiceImpl::GetTicketList
  void GetTicketList(
      const std::shared_ptr<lib::GetTicketListReq>& request,
      std::function<void(const lib::GetTicketListResp&)> done) override;

  /// @copydoc MatchGroupServiceImpl::GetGroupMembers
  void GetGroupMembers(
      const std::shared_ptr<lib::GetGroupMembersReq>& request,
      std::function<void(const lib::GetGroupMembersResp&)> done) override;

  /// @copydoc MatchGroupServiceImpl::GetSettlementList
  void GetSettlementList(
      const std::shared_ptr<lib::GetSettlementListReq>& request,
      std::function<void(const lib::GetSettlementListResp&)> done) override;

  /// @copydoc MatchGroupServiceImpl::RemoveSettlementList
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
