// 异常处理与数据修复测试

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cmatch/config.pb.h"
#include "cmatch/ticket_manager.h"
#include "mock_ticket_entity_manager.h"

namespace cmatch {
namespace {

config::SeasonInfo MakeSeasonInfo(std::uint32_t season_type) {
  config::SeasonInfo info;
  info.set_type(season_type);
  info.set_initial_score(1000);
  info.set_score_attr_id(1);
  info.set_reset_score(false);
  info.set_initial_grade(1);

  {
    config::GradeInfo grade;
    grade.set_grade(1);
    grade.set_prev_grade(1);
    grade.set_next_grade(2);
    grade.set_group_size(2);
    grade.set_promote_rank(1);
    grade.set_promote_rank_percent(0.0F);
    grade.set_demote_rank(0);
    grade.set_demote_rank_percent(0.0F);
    (*info.mutable_grades())[1] = grade;
  }

  {
    config::GradeInfo grade;
    grade.set_grade(2);
    grade.set_prev_grade(1);
    grade.set_next_grade(2);
    grade.set_group_size(0);
    grade.set_promote_rank(0);
    grade.set_promote_rank_percent(0.0F);
    grade.set_demote_rank(0);
    grade.set_demote_rank_percent(0.0F);
    (*info.mutable_grades())[2] = grade;
  }

  return info;
}

config::SeasonTime MakeSeasonTime(std::uint64_t begin, std::uint64_t end) {
  config::SeasonTime time;
  time.set_begin_time(begin);
  time.set_end_time(end);
  return time;
}

void SetScore(const TicketEntityPtr& entity, std::uint32_t attr_id,
              std::uint64_t score) {
  (*entity->GetData().mutable_attributes())[attr_id] = score;
}

TEST(ExceptionHandlingTest, RepairsSeasonTimeMismatch) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime config_time = MakeSeasonTime(100, 200);
  config::SeasonTime stale_time = MakeSeasonTime(0, 50);

  auto entity = manager.GetOrCreateEntity(1, 1);
  {
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(stale_time.begin_time());
    group.set_end_time(stale_time.end_time());
    group.set_grade(1);
    group.set_group_id(1000);
  }

  std::mt19937 rng(12345);
  tm.Initialize(info, config_time, 150, rng);

  const auto& group = entity->GetData().seasons().at(1);
  EXPECT_EQ(group.begin_time(), config_time.begin_time());
  EXPECT_EQ(group.end_time(), config_time.end_time());
  EXPECT_TRUE(manager.IsDirty(1));
}

TEST(ExceptionHandlingTest, OutOfSeasonTicketsAreSettledAndAdded) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  // Disable promotion so grade stays 1 for verification
  (*info.mutable_grades())[1].set_promote_rank(0);
  config::SeasonTime old_time = MakeSeasonTime(0, 100);
  config::SeasonTime new_time = MakeSeasonTime(100, 200);

  // In-season ticket in grade 1
  auto in_season = manager.GetOrCreateEntity(1, 1);
  SetScore(in_season, info.score_attr_id(), 50);
  {
    auto& group = (*in_season->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(new_time.begin_time());
    group.set_end_time(new_time.end_time());
    group.set_grade(1);
    group.set_group_id(1000);
  }

  // Out-of-season ticket in grade 1, same old group
  auto out_of_season = manager.GetOrCreateEntity(2, 1);
  SetScore(out_of_season, info.score_attr_id(), 150);
  {
    auto& group = (*out_of_season->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(old_time.begin_time());
    group.set_end_time(old_time.end_time());
    group.set_grade(1);
    group.set_group_id(1000);
  }

  std::mt19937 rng(12345);
  tm.Initialize(info, new_time, 150, rng);

  // Out-of-season ticket should be settled and then grouped in current season
  ASSERT_EQ(out_of_season->GetData().settlements().count(1), 1);
  const auto& settlement = out_of_season->GetData().settlements().at(1);
  EXPECT_EQ(settlement.rank(), 1);  // higher score

  // Both should now be in the same current season group for grade 1
  EXPECT_EQ(in_season->GetData().seasons().at(1).group_id(),
            out_of_season->GetData().seasons().at(1).group_id());
  EXPECT_EQ(in_season->GetData().seasons().at(1).grade(), 1);
  EXPECT_EQ(out_of_season->GetData().seasons().at(1).grade(), 1);
}

TEST(ExceptionHandlingTest, AlreadySettledTicketsAreNotOverwritten) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime old_time = MakeSeasonTime(0, 100);
  config::SeasonTime new_time = MakeSeasonTime(100, 200);

  // Two out-of-season tickets in same old group
  auto entity1 = manager.GetOrCreateEntity(1, 1);
  auto entity2 = manager.GetOrCreateEntity(2, 1);
  SetScore(entity1, info.score_attr_id(), 100);
  SetScore(entity2, info.score_attr_id(), 200);

  for (std::uint64_t id : {1, 2}) {
    auto entity = manager.GetEntity(id);
    {
      auto& group = (*entity->GetData().mutable_seasons())[1];
      group.set_type(1);
      group.set_begin_time(old_time.begin_time());
      group.set_end_time(old_time.end_time());
      group.set_grade(1);
      group.set_group_id(1000);
    }
  }

  // Pre-settle entity1 with custom rank
  {
    auto& settlement = (*entity1->GetData().mutable_settlements())[1];
    settlement.set_type(1);
    settlement.set_rank(99);
    settlement.set_rank_percent(0.99F);
    settlement.set_score(9999);
  }

  std::mt19937 rng(12345);
  tm.Initialize(info, new_time, 150, rng);

  // entity1 settlement should remain unchanged
  EXPECT_EQ(entity1->GetData().settlements().at(1).rank(), 99);
  EXPECT_EQ(entity1->GetData().settlements().at(1).score(), 9999);

  // entity2 should be newly settled
  EXPECT_EQ(entity2->GetData().settlements().at(1).rank(), 1);
}

TEST(ExceptionHandlingTest, ResetScoreOnNextSeason) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  info.set_reset_score(true);
  info.set_initial_score(777);
  config::SeasonTime time = MakeSeasonTime(0, 100);

  auto entity = manager.GetOrCreateEntity(1, 1);
  SetScore(entity, info.score_attr_id(), 5000);
  {
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  std::mt19937 rng(12345);
  tm.NextSeason(info, time, 200, rng);

  EXPECT_EQ(entity->GetData().attributes().at(info.score_attr_id()), 777);
}

TEST(ExceptionHandlingTest, KeepScoreOnNextSeason) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  info.set_reset_score(false);
  info.set_initial_score(777);
  config::SeasonTime time = MakeSeasonTime(0, 100);

  auto entity = manager.GetOrCreateEntity(1, 1);
  SetScore(entity, info.score_attr_id(), 5000);
  {
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  std::mt19937 rng(12345);
  tm.NextSeason(info, time, 200, rng);

  EXPECT_EQ(entity->GetData().attributes().at(info.score_attr_id()), 5000);
}

TEST(ExceptionHandlingTest, MergedServerGroupIdsDoNotConflict) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  info.set_initial_grade(1);
  // group_size = 1 so each ticket gets its own group
  (*info.mutable_grades())[1].set_group_size(1);
  // Disable promotion so both stay in grade 1
  (*info.mutable_grades())[1].set_promote_rank(0);
  config::SeasonTime time = MakeSeasonTime(0, 100);

  // Pre-existing group IDs from different zones sharing the same sequence
  {
    auto entity = manager.GetOrCreateEntity(1, 1);
    SetScore(entity, info.score_attr_id(), 100);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_grade(1);
    group.set_group_id((static_cast<std::uint64_t>(1) << 32) | 10);
  }
  {
    auto entity = manager.GetOrCreateEntity(2, 2);
    SetScore(entity, info.score_attr_id(), 200);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_grade(1);
    group.set_group_id((static_cast<std::uint64_t>(2) << 32) | 10);
  }

  // Both are in season, will be regrouped with new unique IDs
  std::mt19937 rng(12345);
  tm.Initialize(info, time, 50, rng);

  std::unordered_set<std::uint64_t> group_ids;
  for (const auto& [id, entity] : manager.GetEntities()) {
    (void)id;
    group_ids.insert(entity->GetData().seasons().at(1).group_id());
  }
  EXPECT_EQ(group_ids.size(), 2);
}

}  // namespace
}  // namespace cmatch
