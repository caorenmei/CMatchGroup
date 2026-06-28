// 匹配分组服务实现

#include "cmatch/match_group_service_impl.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>

#include "cmatch/config.pb.h"
#include "cmatch/table.pb.h"
#include "cmatch/ticket_entity_manager_interface.h"

namespace cmatch {

MatchGroupServiceImpl::MatchGroupServiceImpl(
    SeasonConfigInterface& config, TicketEntityManagerInterface& entity_manager,
    TicketManager& ticket_manager)
    : config_(config),
      entity_manager_(entity_manager),
      ticket_manager_(ticket_manager) {}

void MatchGroupServiceImpl::GetSeasonList(
    const std::shared_ptr<lib::GetSeasonListReq>& /*request*/,
    const std::function<void(const lib::GetSeasonListResp&)>& done) {
  auto resp = lib::GetSeasonListResp{};
  resp.set_result(lib::OK);

  for (auto type : config_.GetTypes()) {
    auto time = config::SeasonTime{};
    if (!config_.GetTime(type, time)) {
      continue;
    }
    auto* season = resp.add_seasons();
    season->set_type(type);
    season->set_begin_time(time.begin_time());
    season->set_end_time(time.end_time());
  }

  done(resp);
}

void MatchGroupServiceImpl::SubmitTicket(
    const std::shared_ptr<lib::SubmitTicketReq>& request,
    const std::function<void(const lib::SubmitTicketResp&)>& done) {
  auto resp = lib::SubmitTicketResp{};

  const auto id = request->ticket().id();
  const auto zone_id = request->ticket().zone_id();
  if (id == 0 || zone_id == 0) {
    resp.set_result(lib::INVALID_PARAMETER);
    done(resp);
    return;
  }

  auto entity = entity_manager_.GetOrCreateEntity(id, zone_id);
  auto& ticket = entity->GetData();

  for (const auto& attr : request->ticket().attributes()) {
    (*ticket.mutable_attributes())[attr.first] = attr.second;
  }

  auto existing_regs = std::unordered_set<std::uint32_t>(
      ticket.registrations().begin(), ticket.registrations().end());
  for (auto reg : request->registrations()) {
    if (existing_regs.insert(reg).second) {
      ticket.add_registrations(reg);
    }
  }

  entity_manager_.SetDirty(id);
  resp.set_result(lib::OK);
  done(resp);
}

void MatchGroupServiceImpl::GetTicket(
    const std::shared_ptr<lib::GetTicketReq>& request,
    const std::function<void(const lib::GetTicketResp&)>& done) {
  auto resp = lib::GetTicketResp{};
  const auto id = request->id();

  auto entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  const auto& ticket = entity->GetData();
  resp.set_result(lib::OK);
  resp.mutable_ticket()->set_id(ticket.id());
  resp.mutable_ticket()->set_zone_id(ticket.zone_id());
  for (const auto& attr : ticket.attributes()) {
    (*resp.mutable_ticket()->mutable_attributes())[attr.first] = attr.second;
  }

  for (auto reg : ticket.registrations()) {
    resp.add_registrations(reg);
  }

  for (const auto& season : ticket.seasons()) {
    auto* out = &(*resp.mutable_seasons())[season.first];
    out->set_type(season.second.type());
    out->set_begin_time(season.second.begin_time());
    out->set_end_time(season.second.end_time());
    out->set_grade(season.second.grade());
    out->set_group_id(season.second.group_id());
  }

  done(resp);
}

void MatchGroupServiceImpl::RegisterSeason(
    const std::shared_ptr<lib::RegisterSeasonReq>& request,
    const std::function<void(const lib::RegisterSeasonResp&)>& done) {
  auto resp = lib::RegisterSeasonResp{};
  const auto id = request->id();

  auto entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  auto& ticket = entity->GetData();
  auto existing_regs = std::unordered_set<std::uint32_t>(
      ticket.registrations().begin(), ticket.registrations().end());

  for (auto type : request->types()) {
    auto info = config::SeasonInfo{};
    auto time = config::SeasonTime{};
    if (!config_.GetInfo(type, info) || !config_.GetTime(type, time)) {
      resp.set_result(lib::SEASON_NOT_FOUND);
      done(resp);
      return;
    }
    if (existing_regs.insert(type).second) {
      ticket.add_registrations(type);
    }
  }

  entity_manager_.SetDirty(id);
  resp.set_result(lib::OK);
  done(resp);
}

void MatchGroupServiceImpl::GetTicketList(
    const std::shared_ptr<lib::GetTicketListReq>& request,
    const std::function<void(const lib::GetTicketListResp&)>& done) {
  auto resp = lib::GetTicketListResp{};
  resp.set_result(lib::OK);

  for (auto id : request->ids()) {
    auto entity = entity_manager_.GetEntity(id);
    if (entity == nullptr) {
      continue;
    }
    const auto& ticket = entity->GetData();
    auto* out = resp.add_tickets();
    out->set_id(ticket.id());
    out->set_zone_id(ticket.zone_id());
    for (const auto& attr : ticket.attributes()) {
      (*out->mutable_attributes())[attr.first] = attr.second;
    }
  }

  done(resp);
}

void MatchGroupServiceImpl::GetGroupMembers(
    const std::shared_ptr<lib::GetGroupMembersReq>& request,
    const std::function<void(const lib::GetGroupMembersResp&)>& done) {
  auto resp = lib::GetGroupMembersResp{};
  resp.set_result(lib::OK);

  const auto season_type = request->type();
  const auto group_id = request->group_id();

  for (auto ticket_id :
       ticket_manager_.GetGroupTicketIds(season_type, group_id)) {
    auto entity = entity_manager_.GetEntity(ticket_id);
    if (entity == nullptr) {
      continue;
    }
    const auto& ticket = entity->GetData();
    (*resp.mutable_members())[ticket.id()] = ticket.zone_id();
  }

  done(resp);
}

void MatchGroupServiceImpl::GetSettlementList(
    const std::shared_ptr<lib::GetSettlementListReq>& request,
    const std::function<void(const lib::GetSettlementListResp&)>& done) {
  auto resp = lib::GetSettlementListResp{};
  const auto id = request->id();

  auto entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  resp.set_result(lib::OK);
  const auto& ticket = entity->GetData();
  for (const auto& settlement : ticket.settlements()) {
    auto* out = resp.add_settlements();
    out->set_type(settlement.second.type());
    out->set_begin_time(settlement.second.begin_time());
    out->set_end_time(settlement.second.end_time());
    out->set_grade(settlement.second.grade());
    out->set_group_id(settlement.second.group_id());
    out->set_rank(settlement.second.rank());
    out->set_rank_percent(settlement.second.rank_percent());
    out->set_score(settlement.second.score());
  }

  done(resp);
}

void MatchGroupServiceImpl::RemoveSettlementList(
    const std::shared_ptr<lib::RemoveSettlementListReq>& request,
    const std::function<void(const lib::RemoveSettlementListResp&)>& done) {
  auto resp = lib::RemoveSettlementListResp{};
  const auto id = request->id();

  auto entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  auto& ticket = entity->GetData();
  bool changed = false;
  for (auto settlement_id : request->settlement_ids()) {
    if (settlement_id > std::numeric_limits<std::uint32_t>::max()) {
      continue;
    }
    const auto season_type = static_cast<std::uint32_t>(settlement_id);
    if (ticket.mutable_settlements()->erase(season_type) > 0) {
      changed = true;
    }
  }

  if (changed) {
    entity_manager_.SetDirty(id);
  }

  resp.set_result(lib::OK);
  done(resp);
}

}  // namespace cmatch
