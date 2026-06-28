// 凭据管理器实现

#include "cmatch/ticket_manager.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

}  // namespace

class TicketManager::Impl {
 public:
  explicit Impl(TicketEntityManagerInterface& entity_manager)
      : entity_manager_(entity_manager) {}

  void BuildSeason(const config::SeasonInfo& season_info,
                   const config::SeasonTime& season_time,
                   std::uint32_t now_time, std::mt19937& rng) {
    group_id_allocator_.Initialize(entity_manager_.GetEntities());

    const std::uint32_t season_type = season_info.type();
    std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>
        in_season_by_grade;
    std::unordered_map<std::uint64_t, std::vector<TicketEntityPtr>>
        out_of_season_by_group;
    std::vector<TicketEntityPtr> new_entities;

    for (const auto& [id, entity] : entity_manager_.GetEntities()) {
      (void)id;
      const auto& ticket = entity->GetData();
      auto it = ticket.seasons().find(season_type);
      if (it == ticket.seasons().end()) {
        new_entities.push_back(entity);
        continue;
      }
      config::SeasonTime group_time;
      group_time.set_begin_time(it->second.begin_time());
      group_time.set_end_time(it->second.end_time());
      if (IsInSeason(now_time, group_time)) {
        in_season_by_grade[it->second.grade()].push_back(entity);
      } else {
        out_of_season_by_group[it->second.group_id()].push_back(entity);
      }
    }

    // 在当前赛季内的凭据：按当前段位重新分组
    for (auto& [grade, tickets] : in_season_by_grade) {
      const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
      std::uint32_t group_size =
          grade_info != nullptr ? grade_info->group_size() : 0;
      FormGroups(std::move(tickets), group_size, season_type, season_time,
                 grade, rng);
    }

    // 不在当前赛季内的凭据：按原分组结算并计算新段位
    std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>
        out_next_grade_groups;
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
          new_grade =
              ComputeNextGrade(*grade_info, settlement_it->second.rank(),
                               settlement_it->second.rank_percent());
        }
        out_next_grade_groups[new_grade].push_back(entity);
      }
    }

    for (auto& [grade, tickets] : out_next_grade_groups) {
      const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
      std::uint32_t group_size =
          grade_info != nullptr ? grade_info->group_size() : 0;
      FormGroups(std::move(tickets), group_size, season_type, season_time,
                 grade, rng);
    }

    // 无赛季数据的凭据：分配初始段位并分组
    std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>
        new_by_grade;
    for (const auto& entity : new_entities) {
      std::uint32_t grade = season_info.initial_grade();
      new_by_grade[grade].push_back(entity);
    }

    for (auto& [grade, tickets] : new_by_grade) {
      const config::GradeInfo* grade_info = FindGradeInfo(season_info, grade);
      std::uint32_t group_size =
          grade_info != nullptr ? grade_info->group_size() : 0;
      FormGroups(std::move(tickets), group_size, season_type, season_time,
                 grade, rng);
    }
  }

  void NextSeason(const config::SeasonInfo& season_info,
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
    std::unordered_map<std::uint32_t, std::vector<TicketEntityPtr>>
        grade_groups;
    for (const auto& [id, entity] : entity_manager_.GetEntities()) {
      (void)id;
      auto& ticket = entity->GetData();
      auto settlement_it = ticket.settlements().find(season_type);
      if (settlement_it == ticket.settlements().end()) {
        continue;
      }
      std::uint32_t old_grade = ticket.seasons().at(season_type).grade();
      const config::GradeInfo* grade_info =
          FindGradeInfo(season_info, old_grade);
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
      FormGroups(std::move(tickets), group_size, season_type, season_time,
                 grade, rng);
    }
  }

  void AddTicket(const config::SeasonInfo& season_info,
                 const config::SeasonTime& season_time,
                 std::uint32_t /*now_time*/, std::uint64_t ticket_id,
                 std::mt19937& rng) {
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
      FormGroups({entity}, group_size, season_type, season_time, grade, rng);
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

 private:
  // 分组 ID 分配器，高 32 位为 zone_id，低 32 位为自增编号
  struct GroupIdAllocator {
    std::uint32_t next_sequence_ = 1;

    void Initialize(
        const std::unordered_map<std::uint64_t, TicketEntityPtr>& entities) {
      std::uint32_t max_sequence = 0;
      for (const auto& [id, entity] : entities) {
        (void)id;
        const auto& ticket = entity->GetData();
        for (const auto& [type, group] : ticket.seasons()) {
          (void)type;
          max_sequence = std::max(max_sequence,
                                  static_cast<std::uint32_t>(group.group_id()));
        }
        for (const auto& [type, settlement] : ticket.settlements()) {
          (void)type;
          max_sequence = std::max(
              max_sequence, static_cast<std::uint32_t>(settlement.group_id()));
        }
      }
      next_sequence_ = max_sequence + 1;
    }

    std::uint64_t Allocate(std::uint32_t zone_id) {
      std::uint64_t group_id =
          (static_cast<std::uint64_t>(zone_id) << 32) | next_sequence_;
      ++next_sequence_;
      return group_id;
    }
  };

  void SettleGroup(const std::vector<TicketEntityPtr>& group,
                   std::uint32_t season_type,
                   const config::SeasonTime& season_time, std::uint32_t grade,
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

  static std::uint32_t ComputeNextGrade(const config::GradeInfo& grade_info,
                                        std::uint32_t rank,
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

  void FormGroups(std::vector<TicketEntityPtr> tickets,
                  std::uint32_t group_size, std::uint32_t season_type,
                  const config::SeasonTime& season_time, std::uint32_t grade,
                  std::mt19937& rng) {
    if (tickets.empty()) {
      return;
    }

    std::shuffle(tickets.begin(), tickets.end(), rng);

    std::vector<std::vector<TicketEntityPtr>> groups;
    if (group_size == 0) {
      groups.push_back(tickets);
    } else {
      for (std::size_t i = 0; i < tickets.size(); i += group_size) {
        std::size_t end = std::min(i + group_size, tickets.size());
        groups.emplace_back(
            std::next(tickets.begin(), static_cast<std::ptrdiff_t>(i)),
            std::next(tickets.begin(), static_cast<std::ptrdiff_t>(end)));
      }
    }

    for (const auto& group : groups) {
      if (group.empty()) {
        continue;
      }
      const std::uint32_t zone_id = group.front()->GetData().zone_id();
      const std::uint64_t group_id = group_id_allocator_.Allocate(zone_id);

      for (const auto& entity : group) {
        auto& ticket = entity->GetData();
        table::SeasonGroup& group_entry =
            (*ticket.mutable_seasons())[season_type];
        group_entry.set_type(season_type);
        group_entry.set_begin_time(season_time.begin_time());
        group_entry.set_end_time(season_time.end_time());
        group_entry.set_grade(grade);
        group_entry.set_group_id(group_id);
        entity_manager_.SetDirty(ticket.id());
      }
    }
  }

  TicketEntityManagerInterface& entity_manager_;
  GroupIdAllocator group_id_allocator_;
};

TicketManager::TicketManager(TicketEntityManagerInterface& entity_manager)
    : impl_(std::make_unique<Impl>(entity_manager)) {}

TicketManager::~TicketManager() = default;

void TicketManager::BuildSeason(const config::SeasonInfo& season_info,
                                const config::SeasonTime& season_time,
                                std::uint32_t now_time, std::mt19937& rng) {
  impl_->BuildSeason(season_info, season_time, now_time, rng);
}

void TicketManager::NextSeason(const config::SeasonInfo& season_info,
                               const config::SeasonTime& season_time,
                               std::uint32_t now_time, std::mt19937& rng) {
  impl_->NextSeason(season_info, season_time, now_time, rng);
}

void TicketManager::AddTicket(const config::SeasonInfo& season_info,
                              const config::SeasonTime& season_time,
                              std::uint32_t now_time, std::uint64_t ticket_id,
                              std::mt19937& rng) {
  impl_->AddTicket(season_info, season_time, now_time, ticket_id, rng);
}

}  // namespace cmatch
