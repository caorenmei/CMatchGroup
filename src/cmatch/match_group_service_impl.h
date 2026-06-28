// 匹配分组服务接口

#ifndef CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
#define CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_

#include <functional>
#include <memory>

#include "cmatch/lib.pb.h"

namespace cmatch {

/// @brief 匹配分组服务接口
///
/// 定义 CMatch Library 对外暴露的 8 个 RPC 方法。
/// 使用 std::shared_ptr 承载请求，通过 std::function 回调返回响应，
/// 便于接入各类 RPC 框架。
class MatchGroupServiceImpl {
 public:
  virtual ~MatchGroupServiceImpl() = default;

  /// @brief 获取赛事列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void GetSeasonList(
      const std::shared_ptr<lib::GetSeasonListReq>& request,
      std::function<void(const lib::GetSeasonListResp&)> done) = 0;

  /// @brief 更新凭据
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void SubmitTicket(
      const std::shared_ptr<lib::SubmitTicketReq>& request,
      std::function<void(const lib::SubmitTicketResp&)> done) = 0;

  /// @brief 获取凭据
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void GetTicket(
      const std::shared_ptr<lib::GetTicketReq>& request,
      std::function<void(const lib::GetTicketResp&)> done) = 0;

  /// @brief 报名赛事
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void RegisterSeason(
      const std::shared_ptr<lib::RegisterSeasonReq>& request,
      std::function<void(const lib::RegisterSeasonResp&)> done) = 0;

  /// @brief 获取凭据列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void GetTicketList(
      const std::shared_ptr<lib::GetTicketListReq>& request,
      std::function<void(const lib::GetTicketListResp&)> done) = 0;

  /// @brief 获取分组成员列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void GetGroupMembers(
      const std::shared_ptr<lib::GetGroupMembersReq>& request,
      std::function<void(const lib::GetGroupMembersResp&)> done) = 0;

  /// @brief 获取结算列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void GetSettlementList(
      const std::shared_ptr<lib::GetSettlementListReq>& request,
      std::function<void(const lib::GetSettlementListResp&)> done) = 0;

  /// @brief 删除结算列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  virtual void RemoveSettlementList(
      const std::shared_ptr<lib::RemoveSettlementListReq>& request,
      std::function<void(const lib::RemoveSettlementListResp&)> done) = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
