// 匹配分组服务接口

#ifndef CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
#define CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_

#include <functional>
#include <memory>

#include "cmatch/lib.pb.h"

namespace cmatch {

// 匹配分组服务接口
class MatchGroupServiceImpl {
 public:
  virtual ~MatchGroupServiceImpl() = default;

  // 获取赛事列表
  virtual void GetSeasonList(
      const std::shared_ptr<lib::GetSeasonListReq>& request,
      std::function<void(const lib::GetSeasonListResp&)> done) = 0;

  // 更新凭据
  virtual void SubmitTicket(
      const std::shared_ptr<lib::SubmitTicketReq>& request,
      std::function<void(const lib::SubmitTicketResp&)> done) = 0;

  // 获取凭据
  virtual void GetTicket(
      const std::shared_ptr<lib::GetTicketReq>& request,
      std::function<void(const lib::GetTicketResp&)> done) = 0;

  // 报名赛事
  virtual void RegisterSeason(
      const std::shared_ptr<lib::RegisterSeasonReq>& request,
      std::function<void(const lib::RegisterSeasonResp&)> done) = 0;

  // 获取凭据列表
  virtual void GetTicketList(
      const std::shared_ptr<lib::GetTicketListReq>& request,
      std::function<void(const lib::GetTicketListResp&)> done) = 0;

  // 获取分组成员列表
  virtual void GetGroupMembers(
      const std::shared_ptr<lib::GetGroupMembersReq>& request,
      std::function<void(const lib::GetGroupMembersResp&)> done) = 0;

  // 获取结算列表
  virtual void GetSettlementList(
      const std::shared_ptr<lib::GetSettlementListReq>& request,
      std::function<void(const lib::GetSettlementListResp&)> done) = 0;

  // 删除结算列表
  virtual void RemoveSettlementList(
      const std::shared_ptr<lib::RemoveSettlementListReq>& request,
      std::function<void(const lib::RemoveSettlementListResp&)> done) = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
