// 匹配分组服务实现

#ifndef CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
#define CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_

#include <functional>
#include <memory>

#include "cmatch/lib.pb.h"
#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_entity_manager_interface.h"
#include "cmatch/ticket_manager.h"

namespace cmatch {

/// @brief 匹配分组服务实现
///
/// 实现 CMatch Library 对外暴露的 8 个 RPC 方法。
/// 使用 std::shared_ptr 承载请求，通过 std::function 回调返回响应，
/// 便于接入各类 RPC 框架。
/// 匹配算法由 TicketManager 负责，服务层不直接实现算法逻辑。
class MatchGroupServiceImpl {
 public:
  /// @brief 构造函数
  /// @param[in] config 赛季配置接口
  /// @param[in] entity_manager 凭据实体管理器接口
  /// @param[in] ticket_manager 凭据管理器
  MatchGroupServiceImpl(SeasonConfigInterface& config,
                        TicketEntityManagerInterface& entity_manager,
                        TicketManager& ticket_manager);

  /// @brief 获取赛事列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void GetSeasonList(
      const std::shared_ptr<lib::GetSeasonListReq>& request,
      const std::function<void(const lib::GetSeasonListResp&)>& done);

  /// @brief 更新凭据
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void SubmitTicket(
      const std::shared_ptr<lib::SubmitTicketReq>& request,
      const std::function<void(const lib::SubmitTicketResp&)>& done);

  /// @brief 获取凭据
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void GetTicket(const std::shared_ptr<lib::GetTicketReq>& request,
                 const std::function<void(const lib::GetTicketResp&)>& done);

  /// @brief 报名赛事
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void RegisterSeason(
      const std::shared_ptr<lib::RegisterSeasonReq>& request,
      const std::function<void(const lib::RegisterSeasonResp&)>& done);

  /// @brief 获取凭据列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void GetTicketList(
      const std::shared_ptr<lib::GetTicketListReq>& request,
      const std::function<void(const lib::GetTicketListResp&)>& done);

  /// @brief 获取分组成员列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void GetGroupMembers(
      const std::shared_ptr<lib::GetGroupMembersReq>& request,
      const std::function<void(const lib::GetGroupMembersResp&)>& done);

  /// @brief 获取结算列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void GetSettlementList(
      const std::shared_ptr<lib::GetSettlementListReq>& request,
      const std::function<void(const lib::GetSettlementListResp&)>& done);

  /// @brief 删除结算列表
  /// @param[in] request 请求
  /// @param[in] done 响应回调
  void RemoveSettlementList(
      const std::shared_ptr<lib::RemoveSettlementListReq>& request,
      const std::function<void(const lib::RemoveSettlementListResp&)>& done);

 private:
  SeasonConfigInterface& config_;
  TicketEntityManagerInterface& entity_manager_;
  TicketManager& ticket_manager_;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_MATCH_GROUP_SERVICE_IMPL_H_
