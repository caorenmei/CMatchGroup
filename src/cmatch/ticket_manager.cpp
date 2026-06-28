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

  for (auto season_type : config.GetTypes()) {
    InitializeSeasonType(config, season_type, now_time, rng);
  }
}

void TicketManager::InitializeSeasonType(const SeasonConfigInterface& config,
                                         std::uint32_t season_type,
                                         std::uint32_t now_time,
                                         std::mt19937& rng) {
  auto season_info = config::SeasonInfo{};
  auto season_time = config::SeasonTime{};
  if (!config.GetInfo(season_type, season_info) ||
      !config.GetTime(season_type, season_time)) {
    return;
  }

  auto in_season_by_grade =
      std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>{};
  auto out_of_season_by_group =
      std::unordered_map<std::uint64_t, std::vector<TicketEntityPtr>>{};
  auto new_entities = std::vector<TicketEntityPtr>{};

  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    auto& ticket = entity->GetData();
    auto& seasons = *ticket.mutable_seasons();
    auto it = seasons.find(season_type);
    if (it == seasons.end()) {
      new_entities.push_back(entity);
      continue;
    }

    // 读取凭据上的赛季时间与段位
    auto group_time = config::SeasonTime{};
    auto& group = it->second;
    group_time.set_begin_time(group.begin_time());
    group_time.set_end_time(group.end_time());
    const auto grade = group.grade();
    const auto group_id = group.group_id();

    if (IsInSeason(now_time, group_time)) {
      // 修复赛季时间：以配置为准
      if (group.begin_time() != season_time.begin_time() ||
          group.end_time() != season_time.end_time()) {
        group.set_begin_time(season_time.begin_time());
        group.set_end_time(season_time.end_time());
        entity_manager_.SetDirty(ticket.id());
      }
      in_season_by_grade[grade].push_back(entity);
    } else {
      out_of_season_by_group[group_id].push_back(entity);
    }
  }

  // 待分组凭据按段位收集：不在当前赛季内的凭据结算并计算新段位后进入；
  // 无赛季数据的新凭据使用初始段位进入。
  auto pending_by_grade =
      std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>{};

  // 不在当前赛季内的凭据：按原分组结算并计算新段位
  for (const auto& [group_id, group] : out_of_season_by_group) {
    (void)group_id;
    if (group.empty()) {
      continue;
    }
    auto grade = group.front()->GetData().seasons().at(season_type).grade();
    SettleGroup(group, season_type, season_time, grade,
                season_info.score_attr_id());
  }

  for (const auto& [group_id, group] : out_of_season_by_group) {
    (void)group_id;
    for (const auto& entity : group) {
      const auto& ticket = entity->GetData();
      auto old_grade = ticket.seasons().at(season_type).grade();
      const auto* grade_info = FindGradeInfo(season_info, old_grade);
      auto new_grade = old_grade;
      auto settlement_it = ticket.settlements().find(season_type);
      if (settlement_it != ticket.settlements().end() &&
          grade_info != nullptr) {
        new_grade = ComputeNextGrade(*grade_info, settlement_it->second.rank(),
                                     settlement_it->second.rank_percent());
      }
      pending_by_grade[new_grade].push_back(entity);
    }
  }

  // 无赛季数据的凭据
  for (const auto& entity : new_entities) {
    auto grade = season_info.initial_grade();
    pending_by_grade[grade].push_back(entity);
  }

  // 按段位统一填充未满分组并创建新分组
  auto grade_slots =
      std::unordered_map<std::uint32_t, std::vector<GroupSlot>>{};

  for (auto& [grade, pending] : pending_by_grade) {
    if (pending.empty()) {
      continue;
    }

    // 新加入的待分组凭据先洗牌，再统一填充/建组
    std::shuffle(pending.begin(), pending.end(), rng);

    const auto* grade_info = FindGradeInfo(season_info, grade);
    auto group_size = grade_info != nullptr ? grade_info->group_size() : 0;

    // 1. 优先填充同段位未满的旧分组（利用索引）
    auto unfilled_it = unfilled_season_grade_groups_.find({season_type, grade});
    if (unfilled_it != unfilled_season_grade_groups_.end()) {
      // WriteSeasonGroups 会修改 unfilled_season_grade_groups_，先复制 ID
      // 避免迭代器失效
      auto unfilled_groups = std::vector<std::uint64_t>(
          unfilled_it->second.begin(), unfilled_it->second.end());
      for (auto existing_group_id : unfilled_groups) {
        auto tickets_it = group_to_tickets_.find(existing_group_id);
        if (tickets_it == group_to_tickets_.end()) {
          continue;
        }
        auto current_size = tickets_it->second.size();
        if (group_size > 0 && current_size >= group_size) {
          continue;
        }

        // 创建一个 slot 复用旧 group_id
        auto slot = GroupSlot{};
        slot.group_id = existing_group_id;
        slot.grade = grade;
        while (!pending.empty() &&
               (group_size == 0 ||
                current_size + slot.entities.size() < group_size)) {
          slot.entities.push_back(pending.back());
          pending.pop_back();
        }
        if (!slot.entities.empty()) {
          grade_slots[grade].push_back(std::move(slot));
        }
        if (pending.empty()) {
          break;
        }
      }
    }

    // 2. 剩余凭据统一创建新分组
    if (!pending.empty()) {
      if (group_size == 0) {
        auto slot = GroupSlot{};
        slot.grade = grade;
        slot.group_id =
            group_id_allocator_.Allocate(pending.front()->GetData().zone_id());
        slot.entities = std::move(pending);
        grade_slots[grade].push_back(std::move(slot));
      } else {
        for (auto i = std::size_t{0}; i < pending.size(); i += group_size) {
          auto end = std::min(i + group_size, pending.size());
          auto slot = GroupSlot{};
          slot.grade = grade;
          slot.group_id =
              group_id_allocator_.Allocate(pending[i]->GetData().zone_id());
          slot.entities.assign(
              std::next(pending.begin(), static_cast<std::ptrdiff_t>(i)),
              std::next(pending.begin(), static_cast<std::ptrdiff_t>(end)));
          grade_slots[grade].push_back(std::move(slot));
        }
      }
    }
  }

  // 将最终的分组槽位写入凭据，由 WriteSeasonGroups 统一维护索引
  for (auto& [grade, slots] : grade_slots) {
    (void)grade;
    WriteSeasonGroups(slots, season_type, season_info, season_time);
  }
}

void TicketManager::NextSeason(const config::SeasonInfo& season_info,
                               const config::SeasonTime& season_time,
                               std::uint32_t /*now_time*/, std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());

  const auto season_type = season_info.type();
  auto groups =
      std::unordered_map<std::uint64_t, std::vector<TicketEntityPtr>>{};

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
    auto grade = group.front()->GetData().seasons().at(season_type).grade();
    SettleGroup(group, season_type, season_time, grade,
                season_info.score_attr_id());
  }

  // 计算新段位并按新段位分组
  auto grade_groups =
      std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>{};
  for (const auto& [id, entity] : entity_manager_.GetEntities()) {
    (void)id;
    auto& ticket = entity->GetData();
    auto settlement_it = ticket.settlements().find(season_type);
    if (settlement_it == ticket.settlements().end()) {
      continue;
    }
    auto old_grade = ticket.seasons().at(season_type).grade();
    const auto* grade_info = FindGradeInfo(season_info, old_grade);
    auto new_grade = old_grade;
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
    const auto* grade_info = FindGradeInfo(season_info, grade);
    auto group_size = grade_info != nullptr ? grade_info->group_size() : 0;
    auto slots = std::vector<GroupSlot>{};
    FormGroupSlots(std::move(tickets), group_size, grade, rng, slots);
    WriteSeasonGroups(slots, season_type, season_info, season_time);
  }
}

void TicketManager::AddTicket(const SeasonConfigInterface& config,
                              std::uint32_t now_time, std::uint64_t ticket_id,
                              std::mt19937& rng) {
  group_id_allocator_.Initialize(entity_manager_.GetEntities());

  auto entity = entity_manager_.GetEntity(ticket_id);
  if (entity == nullptr) {
    return;
  }

  for (auto season_type : config.GetTypes()) {
    auto season_info = config::SeasonInfo{};
    auto season_time = config::SeasonTime{};
    if (!config.GetInfo(season_type, season_info) ||
        !config.GetTime(season_type, season_time)) {
      continue;
    }

    AddTicketToSeason(entity, season_type, season_info, season_time, now_time,
                      rng);
  }
}

void TicketManager::AddTicketToSeason(const TicketEntityPtr& entity,
                                      std::uint32_t season_type,
                                      const config::SeasonInfo& season_info,
                                      const config::SeasonTime& season_time,
                                      std::uint32_t now_time,
                                      std::mt19937& rng) {
  auto& ticket = entity->GetData();
  auto it = ticket.seasons().find(season_type);
  if (it == ticket.seasons().end()) {
    auto grade = season_info.initial_grade();
    const auto* grade_info = FindGradeInfo(season_info, grade);
    auto group_size = grade_info != nullptr ? grade_info->group_size() : 0;

    // 优先填入同段位未满旧分组
    auto unfilled_it = unfilled_season_grade_groups_.find({season_type, grade});
    if (unfilled_it != unfilled_season_grade_groups_.end() &&
        !unfilled_it->second.empty()) {
      auto existing_group_id = *unfilled_it->second.begin();
      auto tickets_it = group_to_tickets_.find(existing_group_id);
      if (tickets_it != group_to_tickets_.end() &&
          (group_size == 0 || tickets_it->second.size() < group_size)) {
        auto slots = std::vector<GroupSlot>{};
        auto slot = GroupSlot{};
        slot.group_id = existing_group_id;
        slot.grade = grade;
        slot.entities.push_back(entity);
        slots.push_back(std::move(slot));
        WriteSeasonGroups(slots, season_type, season_info, season_time);
        return;
      }
    }

    // 无未满旧分组，创建新分组
    auto slots = std::vector<GroupSlot>{};
    FormGroupSlots({entity}, group_size, grade, rng, slots);
    WriteSeasonGroups(slots, season_type, season_info, season_time);
    return;
  }

  // 已存在赛季分组时，仅在当前赛季内修复时间
  auto group_time = config::SeasonTime{};
  group_time.set_begin_time(it->second.begin_time());
  group_time.set_end_time(it->second.end_time());
  if (IsInSeason(now_time, group_time)) {
    auto& group = (*ticket.mutable_seasons())[season_type];
    if (group.begin_time() != season_time.begin_time() ||
        group.end_time() != season_time.end_time()) {
      group.set_begin_time(season_time.begin_time());
      group.set_end_time(season_time.end_time());
      entity_manager_.SetDirty(ticket.id());
    }
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
  auto sorted = group;
  std::sort(sorted.begin(), sorted.end(),
            [score_attr_id](const auto& a, const auto& b) {
              auto score_a = GetScore(a->GetData(), score_attr_id);
              auto score_b = GetScore(b->GetData(), score_attr_id);
              if (score_a != score_b) {
                return score_a > score_b;
              }
              return a->GetKey() < b->GetKey();
            });

  const auto total = static_cast<std::uint32_t>(sorted.size());
  for (auto i = std::uint32_t{0}; i < total; ++i) {
    const auto& entity = sorted[i];
    auto& ticket = entity->GetData();

    // 已结算凭据不修改其数据，但仍参与排名计算
    if (ticket.settlements().count(season_type) > 0) {
      continue;
    }

    const auto rank = i + 1;
    const auto rank_percent =
        static_cast<float>(rank) / static_cast<float>(total);

    auto& settlement = (*ticket.mutable_settlements())[season_type];
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
    auto slot = GroupSlot{};
    slot.grade = grade;
    slot.group_id =
        group_id_allocator_.Allocate(tickets.front()->GetData().zone_id());
    slot.entities = std::move(tickets);
    slots.push_back(std::move(slot));
    return;
  }

  for (auto i = std::size_t{0}; i < tickets.size(); i += group_size) {
    auto end = std::min(i + group_size, tickets.size());
    auto slot = GroupSlot{};
    slot.grade = grade;
    slot.group_id =
        group_id_allocator_.Allocate(tickets[i]->GetData().zone_id());
    slot.entities.assign(
        std::next(tickets.begin(), static_cast<std::ptrdiff_t>(i)),
        std::next(tickets.begin(), static_cast<std::ptrdiff_t>(end)));
    slots.push_back(std::move(slot));
  }
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
    auto affected_old_groups =
        std::unordered_map<std::uint64_t, std::uint32_t>{};
    for (const auto& entity : slot.entities) {
      const auto& ticket = entity->GetData();
      auto it = ticket.seasons().find(season_type);
      if (it != ticket.seasons().end()) {
        auto old_group_id = it->second.group_id();
        auto old_grade = it->second.grade();
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
      auto& group = (*ticket.mutable_seasons())[season_type];
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
    const auto* grade_info = FindGradeInfo(season_info, slot.grade);
    auto group_size = grade_info != nullptr ? grade_info->group_size() : 0;
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
      const auto* old_grade_info = FindGradeInfo(season_info, old_grade);
      auto old_group_size =
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
    auto season_info = config::SeasonInfo{};
    if (!config.GetInfo(season_type, season_info)) {
      continue;
    }
    const auto* grade_info = FindGradeInfo(season_info, grade);
    auto group_size = grade_info != nullptr ? grade_info->group_size() : 0;
    for (auto group_id : groups) {
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
