// 匹配分组服务实现

#include "cmatch/match_group_service_impl.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>

#include "cmatch/config.pb.h"
#include "cmatch/table.pb.h"
#include "cmatch/ticket_entity_interface.h"
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
  lib::GetSeasonListResp resp;
  resp.set_result(lib::OK);

  for (std::uint32_t type : config_.GetTypes()) {
    config::SeasonTime time;
    if (!config_.GetTime(type, time)) {
      continue;
    }
    lib::Season* season = resp.add_seasons();
    season->set_type(type);
    season->set_begin_time(time.begin_time());
    season->set_end_time(time.end_time());
  }

  done(resp);
}

void MatchGroupServiceImpl::SubmitTicket(
    const std::shared_ptr<lib::SubmitTicketReq>& request,
    const std::function<void(const lib::SubmitTicketResp&)>& done) {
  lib::SubmitTicketResp resp;

  const std::uint64_t id = request->ticket().id();
  const std::uint32_t zone_id = request->ticket().zone_id();
  if (id == 0 || zone_id == 0) {
    resp.set_result(lib::INVALID_PARAMETER);
    done(resp);
    return;
  }

  TicketEntityPtr entity = entity_manager_.GetOrCreateEntity(id, zone_id);
  table::Ticket& ticket = entity->GetData();

  for (const auto& attr : request->ticket().attributes()) {
    (*ticket.mutable_attributes())[attr.first] = attr.second;
  }

  std::unordered_set<std::uint32_t> existing_regs(
      ticket.registrations().begin(), ticket.registrations().end());
  for (std::uint32_t reg : request->registrations()) {
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
  lib::GetTicketResp resp;
  const std::uint64_t id = request->id();

  TicketEntityPtr entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  const table::Ticket& ticket = entity->GetData();
  resp.set_result(lib::OK);
  resp.mutable_ticket()->set_id(ticket.id());
  resp.mutable_ticket()->set_zone_id(ticket.zone_id());
  for (const auto& attr : ticket.attributes()) {
    (*resp.mutable_ticket()->mutable_attributes())[attr.first] = attr.second;
  }

  for (std::uint32_t reg : ticket.registrations()) {
    resp.add_registrations(reg);
  }

  for (const auto& season : ticket.seasons()) {
    lib::SeasonGroup* out = &(*resp.mutable_seasons())[season.first];
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
  lib::RegisterSeasonResp resp;
  const std::uint64_t id = request->id();

  TicketEntityPtr entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  table::Ticket& ticket = entity->GetData();
  std::unordered_set<std::uint32_t> existing_regs(
      ticket.registrations().begin(), ticket.registrations().end());

  for (std::uint32_t type : request->types()) {
    config::SeasonInfo info;
    config::SeasonTime time;
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
  lib::GetTicketListResp resp;
  resp.set_result(lib::OK);

  for (std::uint64_t id : request->ids()) {
    TicketEntityPtr entity = entity_manager_.GetEntity(id);
    if (entity == nullptr) {
      continue;
    }
    const table::Ticket& ticket = entity->GetData();
    lib::Ticket* out = resp.add_tickets();
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
  lib::GetGroupMembersResp resp;
  resp.set_result(lib::OK);

  const std::uint32_t season_type = request->type();
  const std::uint64_t group_id = request->group_id();

  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    const table::Ticket& ticket = entity->GetData();
    auto it = ticket.seasons().find(season_type);
    if (it == ticket.seasons().end() || it->second.group_id() != group_id) {
      continue;
    }
    (*resp.mutable_members())[ticket.id()] = ticket.zone_id();
  }

  done(resp);
}

void MatchGroupServiceImpl::GetSettlementList(
    const std::shared_ptr<lib::GetSettlementListReq>& request,
    const std::function<void(const lib::GetSettlementListResp&)>& done) {
  lib::GetSettlementListResp resp;
  const std::uint64_t id = request->id();

  TicketEntityPtr entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  resp.set_result(lib::OK);
  const table::Ticket& ticket = entity->GetData();
  for (const auto& settlement : ticket.settlements()) {
    lib::SeasonSettlement* out = resp.add_settlements();
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
  lib::RemoveSettlementListResp resp;
  const std::uint64_t id = request->id();

  TicketEntityPtr entity = entity_manager_.GetEntity(id);
  if (entity == nullptr) {
    resp.set_result(lib::TICKET_NOT_FOUND);
    done(resp);
    return;
  }

  table::Ticket& ticket = entity->GetData();
  bool changed = false;
  for (std::uint64_t settlement_id : request->settlement_ids()) {
    if (settlement_id > std::numeric_limits<std::uint32_t>::max()) {
      continue;
    }
    const std::uint32_t season_type = static_cast<std::uint32_t>(settlement_id);
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
