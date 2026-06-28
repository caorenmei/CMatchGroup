// TicketManager 匹配算法测试

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cmatch/config.pb.h"
#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_manager.h"
#include "mock_ticket_entity_manager.h"

namespace cmatch {
namespace {

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

MockSeasonConfig MakeMockConfig(const config::SeasonInfo& info,
                                const config::SeasonTime& time) {
  MockSeasonConfig config(info);
  config.SetTime(time);
  return config;
}

TEST(TicketManagerTest, IsInSeasonBoundaries) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(100, 200);
  std::mt19937 rng(12345);

  manager.GetOrCreateEntity(1, 1);
  SetScore(manager.GetEntity(1), info.score_attr_id(), 0);

  // now_time == begin_time: in season
  tm.Initialize(MakeMockConfig(info, time), 100, rng);
  EXPECT_TRUE(manager.GetEntity(1)->GetData().seasons().at(1).begin_time() ==
              time.begin_time());

  // now_time == end_time - 1: in season
  tm.Initialize(MakeMockConfig(info, time), 199, rng);
  EXPECT_EQ(manager.GetEntity(1)->GetData().seasons().count(1), 1);

  // now_time == end_time: out of season -> settlement
  tm.Initialize(MakeMockConfig(info, time), 200, rng);
  EXPECT_EQ(manager.GetEntity(1)->GetData().settlements().count(1), 1);
}

TEST(TicketManagerTest, SettlementRankingAndPercentage) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  info.set_initial_grade(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  // Create 4 tickets with different scores, same zone, all in the same group
  for (std::uint64_t id = 1; id <= 4; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 100);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  // Build season with now_time out of season to force settlement
  tm.Initialize(MakeMockConfig(info, time), 200, rng);

  ASSERT_EQ(manager.GetEntity(1)->GetData().settlements().count(1), 1);
  const auto& settlement1 = manager.GetEntity(1)->GetData().settlements().at(1);
  const auto& settlement4 = manager.GetEntity(4)->GetData().settlements().at(1);

  // Highest score (400) gets rank 1
  EXPECT_EQ(settlement4.rank(), 1);
  EXPECT_FLOAT_EQ(settlement4.rank_percent(), 0.25F);

  // Lowest score (100) gets rank 4
  EXPECT_EQ(settlement1.rank(), 4);
  EXPECT_FLOAT_EQ(settlement1.rank_percent(), 1.0F);
}

TEST(TicketManagerTest, SettlementTieBreakById) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  auto entity1 = manager.GetOrCreateEntity(1, 1);
  auto entity2 = manager.GetOrCreateEntity(2, 1);
  SetScore(entity1, info.score_attr_id(), 100);
  SetScore(entity2, info.score_attr_id(), 100);

  {
    auto& group = (*entity1->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(2001);
  }
  {
    auto& group = (*entity2->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(2001);
  }

  tm.Initialize(MakeMockConfig(info, time), 200, rng);

  const auto& settlement1 = manager.GetEntity(1)->GetData().settlements().at(1);
  const auto& settlement2 = manager.GetEntity(2)->GetData().settlements().at(1);

  EXPECT_EQ(settlement1.rank(), 1);
  EXPECT_EQ(settlement2.rank(), 2);
}

TEST(TicketManagerTest, GroupIdUniquenessAcrossZonesAndSettlements) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  // Pre-populate with existing group ids from different zones, same sequence
  {
    auto entity = manager.GetOrCreateEntity(1, 1);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_group_id((static_cast<std::uint64_t>(1) << 32) | 5);
  }
  {
    auto entity = manager.GetOrCreateEntity(2, 2);
    auto& settlement = (*entity->GetData().mutable_settlements())[1];
    settlement.set_group_id((static_cast<std::uint64_t>(2) << 32) | 5);
  }

  // Add a new ticket: allocator must start after sequence 5
  auto entity3 = manager.GetOrCreateEntity(3, 1);
  SetScore(entity3, info.score_attr_id(), 100);
  tm.AddTicket(MakeMockConfig(info, time), 50, 3, rng);

  const std::uint64_t new_group_id =
      manager.GetEntity(3)->GetData().seasons().at(1).group_id();
  const std::uint32_t new_sequence = static_cast<std::uint32_t>(new_group_id);

  EXPECT_GT(new_sequence, 5);
}

TEST(TicketManagerTest, GradePromotionByRank) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  // Two tickets in grade 1, group_size 2 means one group
  auto entity1 = manager.GetOrCreateEntity(1, 1);
  auto entity2 = manager.GetOrCreateEntity(2, 1);
  SetScore(entity1, info.score_attr_id(), 100);
  SetScore(entity2, info.score_attr_id(), 200);

  // Place them in grade 1
  {
    auto& group = (*entity1->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1001);
  }
  {
    auto& group = (*entity2->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1001);
  }

  tm.NextSeason(info, time, 200, rng);

  // Winner (rank 1) promotes to grade 2
  EXPECT_EQ(manager.GetEntity(2)->GetData().seasons().at(1).grade(), 2);
  // Loser stays in grade 1
  EXPECT_EQ(manager.GetEntity(1)->GetData().seasons().at(1).grade(), 1);
}

TEST(TicketManagerTest, GradeDemotionByRank) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  // Grade 2 demotes at rank > 1
  (*info.mutable_grades())[2].set_demote_rank(1);
  (*info.mutable_grades())[2].set_prev_grade(1);

  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  auto entity1 = manager.GetOrCreateEntity(1, 1);
  auto entity2 = manager.GetOrCreateEntity(2, 1);
  SetScore(entity1, info.score_attr_id(), 100);
  SetScore(entity2, info.score_attr_id(), 200);

  // Both in grade 2
  {
    auto& group = (*entity1->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(2);
    group.set_group_id(1001);
  }
  {
    auto& group = (*entity2->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(2);
    group.set_group_id(1001);
  }

  tm.NextSeason(info, time, 200, rng);

  // Rank 2 demotes to grade 1
  EXPECT_EQ(manager.GetEntity(1)->GetData().seasons().at(1).grade(), 1);
  // Rank 1 stays in grade 2
  EXPECT_EQ(manager.GetEntity(2)->GetData().seasons().at(1).grade(), 2);
}

TEST(TicketManagerTest, NoPromotionWhenNotConfigured) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  (*info.mutable_grades())[1].set_promote_rank(0);

  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  auto entity = manager.GetOrCreateEntity(1, 1);
  SetScore(entity, info.score_attr_id(), 9999);
  {
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1001);
  }

  tm.NextSeason(info, time, 200, rng);

  EXPECT_EQ(entity->GetData().seasons().at(1).grade(), 1);
}

TEST(TicketManagerTest, RandomGroupFormation) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);

  for (std::uint64_t id = 1; id <= 4; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
  }

  std::mt19937 rng(12345);
  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  // group_size = 2 -> 2 groups
  std::unordered_map<std::uint64_t, std::size_t> group_sizes;
  for (const auto& [id, entity] : manager.GetEntities()) {
    (void)id;
    ++group_sizes[entity->GetData().seasons().at(1).group_id()];
  }
  EXPECT_EQ(group_sizes.size(), 2);
  for (const auto& [group_id, size] : group_sizes) {
    (void)group_id;
    EXPECT_EQ(size, 2);
  }
}

TEST(TicketManagerTest, GroupSizeZeroFormsSingleGroup) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  (*info.mutable_grades())[1].set_group_size(0);

  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  for (std::uint64_t id = 1; id <= 10; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
  }

  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  std::unordered_set<std::uint64_t> group_ids;
  for (const auto& [id, entity] : manager.GetEntities()) {
    (void)id;
    group_ids.insert(entity->GetData().seasons().at(1).group_id());
  }
  EXPECT_EQ(group_ids.size(), 1);
}

TEST(TicketManagerTest, RemainderFormsGroup) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  // group_size = 3 with 5 tickets -> groups of 3 and 2
  (*info.mutable_grades())[1].set_group_size(3);

  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  for (std::uint64_t id = 1; id <= 5; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
  }

  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  std::unordered_map<std::uint64_t, std::size_t> group_sizes;
  for (const auto& [id, entity] : manager.GetEntities()) {
    (void)id;
    ++group_sizes[entity->GetData().seasons().at(1).group_id()];
  }
  ASSERT_EQ(group_sizes.size(), 2);
  bool has_three = false;
  bool has_two = false;
  for (const auto& [group_id, size] : group_sizes) {
    (void)group_id;
    if (size == 3) {
      has_three = true;
    }
    if (size == 2) {
      has_two = true;
    }
  }
  EXPECT_TRUE(has_three);
  EXPECT_TRUE(has_two);
}

TEST(TicketManagerTest, AddTicketCreatesInitialGroup) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  auto entity = manager.GetOrCreateEntity(42, 5);
  SetScore(entity, info.score_attr_id(), 100);

  tm.AddTicket(MakeMockConfig(info, time), 50, 42, rng);

  const auto& ticket = entity->GetData();
  ASSERT_EQ(ticket.seasons().count(1), 1);
  const auto& group = ticket.seasons().at(1);
  EXPECT_EQ(group.type(), 1);
  EXPECT_EQ(group.grade(), info.initial_grade());
  EXPECT_EQ(group.begin_time(), time.begin_time());
  EXPECT_EQ(group.end_time(), time.end_time());
  EXPECT_EQ(group.group_id() >> 32, 5);  // high 32 bits = zone_id
  EXPECT_TRUE(manager.IsDirty(42));
}

TEST(TicketManagerTest, AddTicketFillsUnfilledGroupFirst) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  (*info.mutable_grades())[1].set_group_size(3);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  // 当前赛季内有一个未满分组（size=1，容量=3）
  auto existing = manager.GetOrCreateEntity(1, 1);
  SetScore(existing, info.score_attr_id(), 100);
  {
    auto& group = (*existing->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  // 重建索引（Initialize 会执行）
  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  // 新凭据应加入 group 1000
  auto new_entity = manager.GetOrCreateEntity(2, 1);
  SetScore(new_entity, info.score_attr_id(), 200);
  tm.AddTicket(MakeMockConfig(info, time), 50, 2, rng);

  EXPECT_EQ(new_entity->GetData().seasons().at(1).group_id(), 1000);
}

TEST(TicketManagerTest, InSeasonTicketsKeepGroupId) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  auto entity = manager.GetOrCreateEntity(1, 1);
  SetScore(entity, info.score_attr_id(), 100);
  auto& group = (*entity->GetData().mutable_seasons())[1];
  group.set_type(1);
  group.set_begin_time(0);
  group.set_end_time(100);
  group.set_grade(1);
  group.set_group_id(12345);

  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  EXPECT_EQ(entity->GetData().seasons().at(1).group_id(), 12345);
}

TEST(TicketManagerTest, OutOfSeasonTicketsGetNewGroupId) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  auto entity = manager.GetOrCreateEntity(1, 1);
  SetScore(entity, info.score_attr_id(), 100);
  auto& group = (*entity->GetData().mutable_seasons())[1];
  group.set_type(1);
  group.set_begin_time(0);
  group.set_end_time(50);
  group.set_grade(1);
  group.set_group_id(12345);

  tm.Initialize(MakeMockConfig(info, time), 75, rng);

  EXPECT_NE(entity->GetData().seasons().at(1).group_id(), 12345);
}

TEST(TicketManagerTest, InSeasonTimeIsRepaired) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  auto entity = manager.GetOrCreateEntity(1, 1);
  SetScore(entity, info.score_attr_id(), 100);
  auto& group = (*entity->GetData().mutable_seasons())[1];
  group.set_type(1);
  group.set_begin_time(10);
  group.set_end_time(90);
  group.set_grade(1);
  group.set_group_id(12345);

  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  const auto& repaired = entity->GetData().seasons().at(1);
  EXPECT_EQ(repaired.begin_time(), time.begin_time());
  EXPECT_EQ(repaired.end_time(), time.end_time());
  EXPECT_TRUE(manager.IsDirty(1));
}

TEST(TicketManagerTest, GroupIndexReturnsMembers) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  for (std::uint64_t id = 1; id <= 4; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
  }

  tm.Initialize(MakeMockConfig(info, time), 50, rng);

  std::unordered_map<std::uint64_t, std::size_t> group_sizes;
  for (std::uint64_t id = 1; id <= 4; ++id) {
    const auto& ticket = manager.GetEntity(id)->GetData();
    const std::uint64_t group_id = ticket.seasons().at(1).group_id();
    auto members = tm.GetGroupTicketIds(1, group_id);
    EXPECT_NE(std::find(members.begin(), members.end(), id), members.end());
    group_sizes[group_id] = members.size();
  }

  EXPECT_EQ(group_sizes.size(), 2);
  for (const auto& [group_id, size] : group_sizes) {
    (void)group_id;
    EXPECT_EQ(size, 2);
  }
}

TEST(TicketManagerTest, PendingTicketsFillUnfilledGroupsFirst) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  (*info.mutable_grades())[1].set_group_size(3);
  // 配置 grade 2：所有结算者都会升段回到 grade 1
  {
    config::GradeInfo grade;
    grade.set_grade(2);
    grade.set_prev_grade(1);
    grade.set_next_grade(1);
    grade.set_group_size(0);
    grade.set_promote_rank(0);
    grade.set_promote_rank_percent(1.0F);
    grade.set_demote_rank(0);
    grade.set_demote_rank_percent(0.0F);
    (*info.mutable_grades())[2] = grade;
  }
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  // 当前赛季内已有一个未满分组（size=1，容量=3）
  auto in_season = manager.GetOrCreateEntity(1, 1);
  SetScore(in_season, info.score_attr_id(), 100);
  {
    auto& group = (*in_season->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  // 不在当前赛季内的两个凭据，原段位为 2，结算后升段回到 grade 1
  for (std::uint64_t id = 2; id <= 3; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(50);  // 不在当前赛季 [0, 100) 内
    group.set_grade(2);
    group.set_group_id(2000 + id);
  }

  tm.Initialize(MakeMockConfig(info, time), 75, rng);

  // 凭据 2、3 应优先填入 group 1000，与凭据 1 同组
  EXPECT_EQ(manager.GetEntity(1)->GetData().seasons().at(1).group_id(), 1000);
  EXPECT_EQ(manager.GetEntity(2)->GetData().seasons().at(1).group_id(), 1000);
  EXPECT_EQ(manager.GetEntity(3)->GetData().seasons().at(1).group_id(), 1000);

  auto members = tm.GetGroupTicketIds(1, 1000);
  EXPECT_EQ(members.size(), 3);
}

TEST(TicketManagerTest, NextSeasonProducesSettlements) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

  for (std::uint64_t id = 1; id <= 3; ++id) {
    auto entity = manager.GetOrCreateEntity(id, 1);
    SetScore(entity, info.score_attr_id(), id * 10);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(1000);
  }

  tm.NextSeason(info, time, 200, rng);

  for (std::uint64_t id = 1; id <= 3; ++id) {
    const auto& ticket = manager.GetEntity(id)->GetData();
    ASSERT_EQ(ticket.settlements().count(1), 1);
    EXPECT_EQ(ticket.settlements().at(1).group_id(), 1000);
  }
}

TEST(TicketManagerTest, ResetScoreOnNextSeason) {
  testing::MockTicketEntityManager manager;
  TicketManager tm(manager);

  config::SeasonInfo info = MakeSeasonInfo(1);
  info.set_reset_score(true);
  info.set_initial_score(777);
  config::SeasonTime time = MakeSeasonTime(0, 100);
  std::mt19937 rng(12345);

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

  tm.NextSeason(info, time, 200, rng);

  EXPECT_EQ(entity->GetData().attributes().at(info.score_attr_id()), 777);
}

}  // namespace
}  // namespace cmatch
