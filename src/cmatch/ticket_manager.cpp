// 凭据管理器实现

#include "cmatch/ticket_manager.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cmatch/season_config_interface.h"
#include "cmatch/table.pb.h"
#include "cmatch/ticket_entity_interface.h"
#include "cmatch/ticket_entity_manager_interface.h"

namespace cmatch {

namespace {

// 判断当前时间是否位于赛季时间范围内（闭开区间）
bool IsInSeason(std::uint32_t now_time, const config::SeasonTime& season_time) {
  return now_time >= season_time.begin_time() &&
         now_time < season_time.end_time();
}

// 获取凭据在指定属性上的积分，缺失时返回 0
std::uint64_t GetScore(const table::Ticket& ticket,
                       std::uint32_t score_attr_id) {
  auto it = ticket.attributes().find(score_attr_id);
  if (it == ticket.attributes().end()) {
    return 0;
  }
  return it->second;
}

// 查找段位配置，不存在返回 nullptr
const config::GradeInfo* FindGradeInfo(const config::SeasonInfo& season_info,
                                       std::uint32_t grade) {
  auto it = season_info.grades().find(grade);
  if (it == season_info.grades().end()) {
    return nullptr;
  }
  return &it->second;
}

// 分组槽位，延迟写入 SeasonGroup
struct GroupSlot {
  std::uint64_t group_id = 0;
  std::uint32_t grade = 0;
  std::vector<TicketEntityPtr> entities;
};

}  // namespace

TicketManager::TicketManager(TicketEntityManagerInterface& entity_manager)
    : entity_manager_(entity_manager) {}

void TicketManager::Initialize(const SeasonConfigInterface& config,
                               std::uint32_t now_time, std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());
  RebuildGroupIndex(config);

  for (std::uint32_t season_type : config.GetTypes()) {
    InitializeSeasonType(config, season_type, now_time, rng);
  }
}

void TicketManager::InitializeSeasonType(
    const SeasonConfigInterface& config, std::uint32_t season_type,
    std::uint32_t now_time, std::mt19937& rng) {
  config::SeasonInfo season_info;
  config::SeasonTime season_time;
  if (!config.GetInfo(season_type, season_info) ||
      !config.GetTime(season_type, season_time)) {
    return;
  }

  std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>
      in_season_by_grade;
  std::unordered_map<std::uint64_t, std::vector<TicketEntityPtr>>
      out_of_season_by_group;
  std::vector<TicketEntityPtr> new_entities;

  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    auto& ticket = entity->GetData();
    auto it = ticket.seasons().find(season_type);
    if (it == ticket.seasons().end()) {
      new_entities.push_back(entity);
      continue;
    }

    // 读取凭据上的赛季时间与段位
    config::SeasonTime group_time;
    group_time.set_begin_time(it->second.begin_time());
    group_time.set_end_time(it->second.end_time());
    const std::uint32_t grade = it->second.grade();
    const std::uint64_t group_id = it->second.group_id();

    if (IsInSeason(now_time, group_time)) {
      // 修复赛季时间：以配置为准
      table::SeasonGroup& mutable_group =
          (*ticket.mutable_seasons())[season_type];
      if (mutable_group.begin_time() != season_time.begin_time() ||
          mutable_group.end_time() != season_time.end_time()) {
        mutable_group.set_begin_time(season_time.begin_time());
        mutable_group.set_end_time(season_time.end_time());
        entity_manager_.SetDirty(ticket.id());
      }
      in_season_by_grade[grade].push_back(entity);
    } else {
      out_of_season_by_group[group_id].push_back(entity);
    }
  }

  // 当前赛季分组槽位：按段位维护若干分组，便于后续填入不在赛季内的凭据
  std::unordered_map<std::uint32_t, std::vector<GroupSlot>> grade_slots;

  // 在当前赛季内的凭据：保留原分组，仅按段位收集
  for (auto& [grade, tickets] : in_season_by_grade) {
    (void)grade;
    (void)tickets;
  }

  // 不在当前赛季内的凭据：按原分组结算并计算新段位
  for (const auto& [group_id, group] : out_of_season_by_group) {
    (void)group_id;
    if (group.empty()) {
      continue;
    }
    std::uint32_t grade =
        group.front()->GetData().seasons().at(season_type).grade();
    SettleGroup(group, season_type, season_time, grade,
                season_info.score_attr_id());
  }

  for (const auto& [group_id, group] : out_of_season_by_group) {
    (void)group_id;
    for (const auto& entity : group) {
      const auto& ticket = entity->GetData();
      std::uint32_t old_grade = ticket.seasons().at(season_type).grade();
      const config::GradeInfo* grade_info =
          FindGradeInfo(season_info, old_grade);
      std::uint32_t new_grade = old_grade;
      auto settlement_it = ticket.settlements().find(season_type);
      if (settlement_it != ticket.settlements().end() &&
          grade_info != nullptr) {
        new_grade = ComputeNextGrade(*grade_info, settlement_it->second.rank(),
                                     settlement_it->second.rank_percent());
      }
      const config::GradeInfo* new_grade_info =
          FindGradeInfo(season_info, new_grade);
      std::uint32_t group_size =
          new_grade_info != nullptr ? new_grade_info->group_size() : 0;
      AddToGroupSlots(entity, group_size, new_grade, grade_slots[new_grade]);
    }
  }

  // 无赛季数据的凭据：分配初始段位并尝试填入现有分组
  for (const auto& entity : new_entities) {
    std::uint32_t grade = season_info.initial_grade();
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;
    AddToGroupSlots(entity, group_size, grade, grade_slots[grade]);
  }

  // 将最终的分组槽位写入凭据
  for (auto& [grade, slots] : grade_slots) {
    (void)grade;
    WriteSeasonGroups(slots, season_type, season_info, season_time);
  }
}

void TicketManager::NextSeason(const config::SeasonInfo& season_info,
                               const config::SeasonTime& season_time,
                               std::uint32_t /*now_time*/, std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());

  const std::uint32_t season_type = season_info.type();
  std::unordered_map<std::uint64_t, std::vector<TicketEntityPtr>> groups;

  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    const auto& ticket = entity->GetData();
    auto it = ticket.seasons().find(season_type);
    if (it == ticket.seasons().end()) {
      continue;
    }
    groups[it->second.group_id()].push_back(entity);
  }

  // 按当前分组结算
  for (const auto& [group_id, group] : groups) {
    (void)group_id;
    if (group.empty()) {
      continue;
    }
    std::uint32_t grade =
        group.front()->GetData().seasons().at(season_type).grade();
    SettleGroup(group, season_type, season_time, grade,
                season_info.score_attr_id());
  }

  // 计算新段位并按新段位分组
  std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>> grade_groups;
  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    auto& ticket = entity->GetData();
    auto settlement_it = ticket.settlements().find(season_type);
    if (settlement_it == ticket.settlements().end()) {
      continue;
    }
    std::uint32_t old_grade = ticket.seasons().at(season_type).grade();
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, old_grade);
    std::uint32_t new_grade = old_grade;
    if (grade_info != nullptr) {
      new_grade = ComputeNextGrade(*grade_info, settlement_it->second.rank(),
                                   settlement_it->second.rank_percent());
    }
    grade_groups[new_grade].push_back(entity);
  }

  // 按配置重置积分
  if (season_info.reset_score()) {
    for (const auto& [id, entity] : entity_manager_.GetEntities()) {
      auto& ticket = entity->GetData();
      if (ticket.seasons().count(season_type) == 0) {
        continue;
      }
      (*ticket.mutable_attributes())[season_info.score_attr_id()] =
          season_info.initial_score();
      entity_manager_.SetDirty(ticket.id());
    }
  }

  for (auto& [grade, tickets] : grade_groups) {
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;
    std::vector<GroupSlot> slots;
    FormGroupSlots(std::move(tickets), group_size, grade, rng, slots);
    WriteSeasonGroups(slots, season_type, season_info, season_time);
  }
}

void TicketManager::AddTicket(const config::SeasonInfo& season_info,
                              const config::SeasonTime& season_time,
                              std::uint32_t /*now_time*/,
                              std::uint64_t ticket_id, std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());

  const std::uint32_t season_type = season_info.type();
  TicketEntityPtr entity = entity_manager_.GetEntity(ticket_id);
  if (entity == nullptr) {
    return;
  }

  auto& ticket = entity->GetData();
  auto it = ticket.seasons().find(season_type);
  if (it == ticket.seasons().end()) {
    std::uint32_t grade = season_info.initial_grade();
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;
    std::vector<GroupSlot> slots;
    FormGroupSlots({entity}, group_size, grade, rng, slots);
    WriteSeasonGroups(slots, season_type, season_info, season_time);
    return;
  }

  // 已存在赛季分组时，确保时间与配置一致
  table::SeasonGroup& group = (*ticket.mutable_seasons())[season_type];
  if (group.begin_time() != season_time.begin_time() ||
      group.end_time() != season_time.end_time()) {
    group.set_begin_time(season_time.begin_time());
    group.set_end_time(season_time.end_time());
    entity_manager_.SetDirty(ticket.id());
  }
}

std::uint32_t TicketManager::ComputeNextGrade(
    const config::GradeInfo& grade_info, std::uint32_t rank,
    float rank_percent) {
  if (grade_info.promote_rank_percent() > 0.0F &&
      rank_percent <= grade_info.promote_rank_percent()) {
    return grade_info.next_grade();
  }
  if (grade_info.promote_rank() > 0 && rank <= grade_info.promote_rank()) {
    return grade_info.next_grade();
  }
  if (grade_info.demote_rank_percent() > 0.0F &&
      rank_percent > grade_info.demote_rank_percent()) {
    return grade_info.prev_grade();
  }
  if (grade_info.demote_rank() > 0 && rank > grade_info.demote_rank()) {
    return grade_info.prev_grade();
  }
  return grade_info.grade();
}

void TicketManager::SettleGroup(const std::vector<TicketEntityPtr>& group,
                                std::uint32_t season_type,
                                const config::SeasonTime& season_time,
                                std::uint32_t grade,
                                std::uint32_t score_attr_id) {
  std::vector<TicketEntityPtr> sorted = group;
  std::sort(
      sorted.begin(), sorted.end(),
      [score_attr_id](const TicketEntityPtr& a, const TicketEntityPtr& b) {
        std::uint64_t score_a = GetScore(a->GetData(), score_attr_id);
        std::uint64_t score_b = GetScore(b->GetData(), score_attr_id);
        if (score_a != score_b) {
          return score_a > score_b;
        }
        return a->GetKey() < b->GetKey();
      });

  const std::uint32_t total = static_cast<std::uint32_t>(sorted.size());
  for (std::uint32_t i = 0; i < total; ++i) {
    const auto& entity = sorted[i];
    auto& ticket = entity->GetData();

    // 已结算凭据不修改其数据，但仍参与排名计算
    if (ticket.settlements().count(season_type) > 0) {
      continue;
    }

    const std::uint32_t rank = i + 1;
    const float rank_percent =
        static_cast<float>(rank) / static_cast<float>(total);

    table::SeasonSettlement& settlement =
        (*ticket.mutable_settlements())[season_type];
    settlement.set_type(season_type);
    settlement.set_begin_time(season_time.begin_time());
    settlement.set_end_time(season_time.end_time());
    settlement.set_grade(grade);
    settlement.set_group_id(ticket.seasons().at(season_type).group_id());
    settlement.set_rank(rank);
    settlement.set_rank_percent(rank_percent);
    settlement.set_score(GetScore(ticket, score_attr_id));

    entity_manager_.SetDirty(ticket.id());
  }
}

void TicketManager::FormGroupSlots(std::vector<TicketEntityPtr> tickets,
                                   std::uint32_t group_size,
                                   std::uint32_t grade, std::mt19937& rng,
                                   std::vector<GroupSlot>& slots) {
  if (tickets.empty()) {
    return;
  }

  std::shuffle(tickets.begin(), tickets.end(), rng);

  if (group_size == 0) {
    GroupSlot slot;
    slot.grade = grade;
    slot.group_id =
        group_id_allocator_.Allocate(tickets.front()->GetData().zone_id());
    slot.entities = std::move(tickets);
    slots.push_back(std::move(slot));
    return;
  }

  for (std::size_t i = 0; i < tickets.size(); i += group_size) {
    std::size_t end = std::min(i + group_size, tickets.size());
    GroupSlot slot;
    slot.grade = grade;
    slot.group_id =
        group_id_allocator_.Allocate(tickets[i]->GetData().zone_id());
    slot.entities.assign(
        std::next(tickets.begin(), static_cast<std::ptrdiff_t>(i)),
        std::next(tickets.begin(), static_cast<std::ptrdiff_t>(end)));
    slots.push_back(std::move(slot));
  }
}

void TicketManager::AddToGroupSlots(const TicketEntityPtr& entity,
                                    std::uint32_t group_size,
                                    std::uint32_t grade,
                                    std::vector<GroupSlot>& slots) {
  if (group_size == 0) {
    if (!slots.empty()) {
      slots.front().entities.push_back(entity);
      return;
    }
  } else {
    for (auto& slot : slots) {
      if (slot.entities.size() < group_size) {
        slot.entities.push_back(entity);
        return;
      }
    }
  }

  GroupSlot slot;
  slot.grade = grade;
  slot.group_id = group_id_allocator_.Allocate(entity->GetData().zone_id());
  slot.entities.push_back(entity);
  slots.push_back(std::move(slot));
}

void TicketManager::WriteSeasonGroups(std::vector<GroupSlot>& slots,
                                      std::uint32_t season_type,
                                      const config::SeasonInfo& season_info,
                                      const config::SeasonTime& season_time) {
  for (auto& slot : slots) {
    if (slot.entities.empty()) {
      continue;
    }

    // 1. 从旧分组索引中移除这些凭据，并记录受影响的旧分组
    std::unordered_map<std::uint64_t, std::uint32_t> affected_old_groups;
    for (const auto& entity : slot.entities) {
      const auto& ticket = entity->GetData();
      auto it = ticket.seasons().find(season_type);
      if (it != ticket.seasons().end()) {
        std::uint64_t old_group_id = it->second.group_id();
        std::uint32_t old_grade = it->second.grade();
        group_to_tickets_[old_group_id].erase(ticket.id());
        if (group_to_tickets_[old_group_id].empty()) {
          group_to_tickets_.erase(old_group_id);
          season_grade_to_groups_[{season_type, old_grade}].erase(old_group_id);
          unfilled_season_grade_groups_[{season_type, old_grade}].erase(
              old_group_id);
        } else {
          affected_old_groups[old_group_id] = old_grade;
        }
      }
    }

    // 2. 写入新的 SeasonGroup
    for (const auto& entity : slot.entities) {
      auto& ticket = entity->GetData();
      table::SeasonGroup& group = (*ticket.mutable_seasons())[season_type];
      group.set_type(season_type);
      group.set_begin_time(season_time.begin_time());
      group.set_end_time(season_time.end_time());
      group.set_grade(slot.grade);
      group.set_group_id(slot.group_id);
      entity_manager_.SetDirty(ticket.id());
    }

    // 3. 建立新分组索引
    season_grade_to_groups_[{season_type, slot.grade}].insert(slot.group_id);
    for (const auto& entity : slot.entities) {
      group_to_tickets_[slot.group_id].insert(entity->GetData().id());
    }

    // 4. 维护新分组未满索引
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, slot.grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;
    if (group_size == 0 || slot.entities.size() < group_size) {
      unfilled_season_grade_groups_[{season_type, slot.grade}].insert(
          slot.group_id);
    } else {
      unfilled_season_grade_groups_[{season_type, slot.grade}].erase(
          slot.group_id);
    }

    // 5. 维护受影响的旧分组未满索引
    for (const auto& [old_group_id, old_grade] : affected_old_groups) {
      auto tickets_it = group_to_tickets_.find(old_group_id);
      if (tickets_it == group_to_tickets_.end()) {
        continue;
      }
      const config::GradeInfo* old_grade_info =
          FindGradeInfo(season_info, old_grade);
      std::uint32_t old_group_size =
          old_grade_info != nullptr ? old_grade_info->group_size() : 0;
      if (old_group_size == 0 || tickets_it->second.size() < old_group_size) {
        unfilled_season_grade_groups_[{season_type, old_grade}].insert(
            old_group_id);
      } else {
        unfilled_season_grade_groups_[{season_type, old_grade}].erase(
            old_group_id);
      }
    }
  }
}

std::vector<std::uint64_t> TicketManager::GetGroupTicketIds(
    std::uint32_t /*season_type*/, std::uint64_t group_id) const {
  auto it = group_to_tickets_.find(group_id);
  if (it == group_to_tickets_.end()) {
    return {};
  }
  return std::vector<std::uint64_t>(it->second.begin(), it->second.end());
}

void TicketManager::RebuildGroupIndex(const SeasonConfigInterface& config) {
  season_grade_to_groups_.clear();
  group_to_tickets_.clear();
  unfilled_season_grade_groups_.clear();

  // 1. 扫描所有凭据，建立 season/grade -> group_ids 与 group_id -> ticket_ids
  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    const auto& ticket = entity->GetData();
    for (const auto& [season_type, group] : ticket.seasons()) {
      season_grade_to_groups_[{season_type, group.grade()}].insert(
          group.group_id());
      group_to_tickets_[group.group_id()].insert(ticket.id());
    }
  }

  // 2. 根据配置计算哪些分组未满
  for (const auto& [key, groups] : season_grade_to_groups_) {
    const auto& [season_type, grade] = key;
    config::SeasonInfo season_info;
    if (!config.GetInfo(season_type, season_info)) {
      continue;
    }
    const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
    std::uint32_t group_size =
        grade_info != nullptr ? grade_info->group_size() : 0;
    for (std::uint64_t group_id : groups) {
      auto it = group_to_tickets_.find(group_id);
      if (it == group_to_tickets_.end()) {
        continue;
      }
      if (group_size == 0 || it->second.size() < group_size) {
        unfilled_season_grade_groups_[key].insert(group_id);
      }
    }
  }
}

}  // namespace cmatch
