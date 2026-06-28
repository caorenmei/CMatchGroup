// 异常处理与数据修复测试

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <unordered_set>

#include "cmatch/config.pb.h"
#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_manager.h"
#include "mock_ticket_entity_manager.h"

namespace cmatch {
namespace {

config::SeasonInfo MakeSeasonInfo(std::uint32_t season_type) {
  auto info = config::SeasonInfo{};
  info.set_type(season_type);
  info.set_initial_score(1000);
  info.set_score_attr_id(1);
  info.set_reset_score(false);
  info.set_initial_grade(1);

  {
    auto grade = config::GradeInfo{};
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
    auto grade = config::GradeInfo{};
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
  auto time = config::SeasonTime{};
  time.set_begin_time(begin);
  time.set_end_time(end);
  return time;
}

void SetScore(const TicketEntityPtr& entity, std::uint32_t attr_id,
              std::uint64_t score) {
  (*entity->GetData().mutable_attributes())[attr_id] = score;
}

class MockSeasonConfig : public SeasonConfigInterface {
 public:
  explicit MockSeasonConfig(config::SeasonInfo info) : info_(std::move(info)) {}

  std::vector<std::uint32_t> GetTypes() const override {
    return {info_.type()};
  }

  bool GetInfo(std::uint32_t type, config::SeasonInfo& info) const override {
    if (type != info_.type()) {
      return false;
    }
    info = info_;
    return true;
  }

  bool GetTime(std::uint32_t type, config::SeasonTime& time) const override {
    if (type != info_.type()) {
      return false;
    }
    time = time_;
    return true;
  }

  void SetTime(const config::SeasonTime& time) { time_ = time; }

 private:
  config::SeasonInfo info_;
  config::SeasonTime time_;
};

MockSeasonConfig MakeMockConfig(const config::SeasonInfo& info,
                                const config::SeasonTime& time) {
  auto config = MockSeasonConfig{info};
  config.SetTime(time);
  return config;
}

TEST(ExceptionHandlingTest, RepairsSeasonTimeMismatch) {
  auto manager = testing::MockTicketEntityManager{};
  auto tm = TicketManager{manager};

  auto info = MakeSeasonInfo(1);
  auto config_time = MakeSeasonTime(100, 200);
  auto stale_time = MakeSeasonTime(0, 50);

  auto entity = manager.GetOrCreateEntity(1, 1);
  {
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(stale_time.begin_time());
    group.set_end_time(stale_time.end_time());
    group.set_grade(1);
    group.set_group_id(1000);
  }

  auto rng = std::mt19937{12345};
  tm.Initialize(MakeMockConfig(info, config_time), 150, rng);

  const auto& group = entity->GetData().seasons().at(1);
  EXPECT_EQ(group.begin_time(), config_time.begin_time());
  EXPECT_EQ(group.end_time(), config_time.end_time());
  EXPECT_TRUE(manager.IsDirty(1));
}

TEST(ExceptionHandlingTest, OutOfSeasonTicketsAreSettledAndAdded) {
  auto manager = testing::MockTicketEntityManager{};
  auto tm = TicketManager{manager};

  auto info = MakeSeasonInfo(1);
  // Disable promotion so grade stays 1 for verification
  (*info.mutable_grades())[1].set_promote_rank(0);
  auto old_time = MakeSeasonTime(0, 100);
  auto new_time = MakeSeasonTime(100, 200);

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

  auto rng = std::mt19937{12345};
  tm.Initialize(MakeMockConfig(info, new_time), 150, rng);

  // Out-of-season ticket should be settled and then grouped in current season
  ASSERT_EQ(out_of_season->GetData().settlements().count(1), 1);
  const auto& settlement = out_of_season->GetData().settlements().at(1);
  EXPECT_EQ(settlement.rank(), 1);  // higher score

  // In-season ticket keeps its original group; out-of-season ticket gets a new
  // current-season group.
  EXPECT_EQ(in_season->GetData().seasons().at(1).group_id(), 1000);
  EXPECT_NE(out_of_season->GetData().seasons().at(1).group_id(), 1000);
  EXPECT_NE(out_of_season->GetData().seasons().at(1).group_id(),
            in_season->GetData().seasons().at(1).group_id());
  EXPECT_EQ(in_season->GetData().seasons().at(1).grade(), 1);
  EXPECT_EQ(out_of_season->GetData().seasons().at(1).grade(), 1);
}

TEST(ExceptionHandlingTest, AlreadySettledTicketsAreNotOverwritten) {
  auto manager = testing::MockTicketEntityManager{};
  auto tm = TicketManager{manager};

  auto info = MakeSeasonInfo(1);
  auto old_time = MakeSeasonTime(0, 100);
  auto new_time = MakeSeasonTime(100, 200);

  // Two out-of-season tickets in same old group
  auto entity1 = manager.GetOrCreateEntity(1, 1);
  auto entity2 = manager.GetOrCreateEntity(2, 1);
  SetScore(entity1, info.score_attr_id(), 100);
  SetScore(entity2, info.score_attr_id(), 200);

  for (auto id : {std::uint64_t{1}, std::uint64_t{2}}) {
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

  auto rng = std::mt19937{12345};
  tm.Initialize(MakeMockConfig(info, new_time), 150, rng);

  // entity1 settlement should remain unchanged
  EXPECT_EQ(entity1->GetData().settlements().at(1).rank(), 99);
  EXPECT_EQ(entity1->GetData().settlements().at(1).score(), 9999);

  // entity2 should be newly settled
  EXPECT_EQ(entity2->GetData().settlements().at(1).rank(), 1);
}

TEST(ExceptionHandlingTest, ResetScoreOnNextSeason) {
  auto manager = testing::MockTicketEntityManager{};
  auto tm = TicketManager{manager};

  auto info = MakeSeasonInfo(1);
  info.set_reset_score(true);
  info.set_initial_score(777);
  auto time = MakeSeasonTime(0, 100);

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

  auto rng = std::mt19937{12345};
  tm.NextSeason(info, time, 200, rng);

  EXPECT_EQ(entity->GetData().attributes().at(info.score_attr_id()), 777);
}

TEST(ExceptionHandlingTest, KeepScoreOnNextSeason) {
  auto manager = testing::MockTicketEntityManager{};
  auto tm = TicketManager{manager};

  auto info = MakeSeasonInfo(1);
  info.set_reset_score(false);
  info.set_initial_score(777);
  auto time = MakeSeasonTime(0, 100);

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

  auto rng = std::mt19937{12345};
  tm.NextSeason(info, time, 200, rng);

  EXPECT_EQ(entity->GetData().attributes().at(info.score_attr_id()), 5000);
}

TEST(ExceptionHandlingTest, MergedServerGroupIdsDoNotConflict) {
  auto manager = testing::MockTicketEntityManager{};
  auto tm = TicketManager{manager};

  auto info = MakeSeasonInfo(1);
  info.set_initial_grade(1);
  // group_size = 1 so each ticket gets its own group
  (*info.mutable_grades())[1].set_group_size(1);
  // Disable promotion so both stay in grade 1
  (*info.mutable_grades())[1].set_promote_rank(0);
  auto time = MakeSeasonTime(0, 100);

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
  auto rng = std::mt19937{12345};
  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  auto group_ids = std::unordered_set<std::uint64_t>{};
  for (const auto& [id, entity] : manager.GetEntities()) {
    (void)id;
    group_ids.insert(entity->GetData().seasons().at(1).group_id());
  }
  EXPECT_EQ(group_ids.size(), 2);
}

}  // namespace
}  // namespace cmatch
