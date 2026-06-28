// 端到端与边界场景集成测试

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cmatch/config.pb.h"
#include "cmatch/lib.pb.h"
#include "cmatch/match_group_service_impl.h"
#include "cmatch/season_config_interface.h"
#include "cmatch/ticket_manager.h"
#include "mock_ticket_entity_manager.h"

namespace cmatch {
namespace {

class MockSeasonConfig : public SeasonConfigInterface {
 public:
  MockSeasonConfig() = default;

  explicit MockSeasonConfig(const std::vector<config::SeasonInfo>& infos) {
    for (const auto& info : infos) {
      infos_[info.type()] = info;
    }
  }

  std::vector<std::uint32_t> GetTypes() const override {
    auto types = std::vector<std::uint32_t>{};
    types.reserve(infos_.size());
    for (const auto& [type, _] : infos_) {
      types.push_back(type);
    }
    std::sort(types.begin(), types.end());
    return types;
  }

  bool GetInfo(std::uint32_t type, config::SeasonInfo& info) const override {
    auto it = infos_.find(type);
    if (it == infos_.end()) {
      return false;
    }
    info = it->second;
    return true;
  }

  bool GetTime(std::uint32_t type, config::SeasonTime& time) const override {
    auto it = times_.find(type);
    if (it == times_.end()) {
      return false;
    }
    time = it->second;
    return true;
  }

  void AddSeason(const config::SeasonInfo& info,
                 const config::SeasonTime& time) {
    infos_[info.type()] = info;
    times_[info.type()] = time;
  }

 private:
  std::unordered_map<std::uint32_t, config::SeasonInfo> infos_;
  std::unordered_map<std::uint32_t, config::SeasonTime> times_;
};

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
    // group_size=0 表示顶级段位不再进行分组，所有凭据视为同一组
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

class CMatchIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    config_ = std::make_unique<MockSeasonConfig>();
    manager_ = std::make_unique<testing::MockTicketEntityManager>();
    ticket_manager_ = std::make_unique<TicketManager>(*manager_);
    service_ = std::make_unique<MatchGroupServiceImpl>(*config_, *manager_,
                                                       *ticket_manager_);
  }

  std::unique_ptr<MockSeasonConfig> config_;
  std::unique_ptr<testing::MockTicketEntityManager> manager_;
  std::unique_ptr<TicketManager> ticket_manager_;
  std::unique_ptr<MatchGroupServiceImpl> service_;
};

// R6.1 端到端集成测试：完整业务流程
TEST_F(CMatchIntegrationTest, EndToEndWorkflow) {
  // 1. 准备内存配置实现
  auto info = MakeSeasonInfo(1);
  auto time = MakeSeasonTime(0, 100);
  config_->AddSeason(info, time);

  // 2. 准备内存实体管理器已就绪（manager_）

  // 3. TicketManager 与 MatchGroupServiceDefault 已在 SetUp 中构造

  // 4. 调用 GetSeasonList 验证赛季列表
  {
    auto req = std::make_shared<lib::GetSeasonListReq>();
    auto resp = lib::GetSeasonListResp{};
    service_->GetSeasonList(
        req, [&resp](const lib::GetSeasonListResp& r) { resp = r; });

    EXPECT_EQ(resp.result(), lib::OK);
    ASSERT_EQ(resp.seasons().size(), 1);
    EXPECT_EQ(resp.seasons(0).type(), 1);
    EXPECT_EQ(resp.seasons(0).begin_time(), 0);
    EXPECT_EQ(resp.seasons(0).end_time(), 100);
  }

  // 5. 调用 SubmitTicket 提交多个凭据
  for (auto id = std::uint64_t{1}; id <= 4; ++id) {
    auto req = std::make_shared<lib::SubmitTicketReq>();
    req->mutable_ticket()->set_id(id);
    req->mutable_ticket()->set_zone_id(1);
    (*req->mutable_ticket()->mutable_attributes())[1] = id * 100;
    req->add_registrations(1);

    auto resp = lib::SubmitTicketResp{};
    service_->SubmitTicket(
        req, [&resp](const lib::SubmitTicketResp& r) { resp = r; });
    EXPECT_EQ(resp.result(), lib::OK);
  }

  // 验证凭据已创建
  for (auto id = std::uint64_t{1}; id <= 4; ++id) {
    auto entity = manager_->GetEntity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->GetData().attributes().at(1), id * 100);
  }

  // 6. 调用 RegisterSeason 为未在 SubmitTicket 中报名的凭据报名（id=5
  // 未提交过）
  {
    auto req = std::make_shared<lib::RegisterSeasonReq>();
    req->set_id(5);
    req->add_types(1);

    auto resp = lib::RegisterSeasonResp{};
    service_->RegisterSeason(
        req, [&resp](const lib::RegisterSeasonResp& r) { resp = r; });
    // 凭据不存在，应返回 TICKET_NOT_FOUND 错误
    EXPECT_EQ(resp.result(), lib::TICKET_NOT_FOUND);
  }

  // 为已存在的凭据报名（id=1 已在 SubmitTicket 中报名，验证幂等性）
  {
    auto req = std::make_shared<lib::RegisterSeasonReq>();
    req->set_id(1);
    req->add_types(1);

    auto resp = lib::RegisterSeasonResp{};
    service_->RegisterSeason(
        req, [&resp](const lib::RegisterSeasonResp& r) { resp = r; });
    EXPECT_EQ(resp.result(), lib::OK);
  }

  // 7. 调用 BuildSeason 构建分组（通过 TicketManager 直接调用）
  {
    auto rng = std::mt19937{12345};
    auto init_config = MockSeasonConfig{};
    init_config.AddSeason(info, time);
    ticket_manager_->Initialize(init_config, 50, rng);
  }

  // 记录 BuildSeason 后的分组，用于后续验证段位升降
  auto pre_next_groups =
      std::unordered_map<std::uint64_t, std::vector<std::uint64_t>>{};
  for (auto id = std::uint64_t{1}; id <= 4; ++id) {
    auto entity = manager_->GetEntity(id);
    ASSERT_NE(entity, nullptr);
    auto group_id = entity->GetData().seasons().at(1).group_id();
    pre_next_groups[group_id].push_back(id);
  }

  // 8. 调用 GetGroupMembers 验证分组成员（group_size=2，4 个凭据应分为 2
  // 组，每组 2 人）
  {
    // 验证共有 2 个分组，每组恰好 2 个成员
    EXPECT_EQ(pre_next_groups.size(), 2);
    for (const auto& [group_id, members] : pre_next_groups) {
      (void)group_id;
      EXPECT_EQ(members.size(), 2)
          << "分组 ID " << group_id << " 的成员数应为 2";
    }

    // 验证通过 GetGroupMembers 接口获取第一个凭据所在组的成员
    auto entity = manager_->GetEntity(1);
    ASSERT_NE(entity, nullptr);
    auto group_id = entity->GetData().seasons().at(1).group_id();

    auto req = std::make_shared<lib::GetGroupMembersReq>();
    req->set_type(1);
    req->set_group_id(group_id);

    auto resp = lib::GetGroupMembersResp{};
    service_->GetGroupMembers(
        req, [&resp](const lib::GetGroupMembersResp& r) { resp = r; });

    EXPECT_EQ(resp.result(), lib::OK);
    EXPECT_EQ(resp.members().size(), 2);
  }

  // 9. 模拟时间推进，调用 NextSeason 切换赛季
  {
    auto new_time = MakeSeasonTime(100, 200);
    auto rng = std::mt19937{12345};
    ticket_manager_->NextSeason(info, new_time, 150, rng);
  }

  // 10. 调用 GetSettlementList 验证结算数据
  {
    auto req = std::make_shared<lib::GetSettlementListReq>();
    req->set_id(1);

    auto resp = lib::GetSettlementListResp{};
    service_->GetSettlementList(
        req, [&resp](const lib::GetSettlementListResp& r) { resp = r; });

    EXPECT_EQ(resp.result(), lib::OK);
    ASSERT_EQ(resp.settlements().size(), 1);
    // 结算数据应包含排名信息
    EXPECT_GT(resp.settlements(0).rank(), 0);
  }

  // 11. 验证段位升降与积分重置行为
  {
    // group_size=2，4 个凭据分为 2 组，每组 promote_rank=1，应有 2 个凭据升段
    // 使用 BuildSeason 后的分组记录验证：每组中积分最高者升段
    for (const auto& [group_id, members] : pre_next_groups) {
      (void)group_id;
      ASSERT_EQ(members.size(), 2);
      auto promoted_id = std::uint64_t{0};
      auto max_score = std::uint64_t{0};
      for (auto id : members) {
        auto e = manager_->GetEntity(id);
        ASSERT_NE(e, nullptr);
        auto score = e->GetData().attributes().at(1);
        if (score > max_score) {
          max_score = score;
          promoted_id = id;
        }
      }
      ASSERT_NE(promoted_id, 0);

      // 验证该组中积分最高者已升段
      auto promoted = manager_->GetEntity(promoted_id);
      ASSERT_NE(promoted, nullptr);
      EXPECT_EQ(promoted->GetData().seasons().at(1).grade(), 2)
          << "凭据 " << promoted_id << " 应升段";

      // 验证该组中其余凭据留在原段位
      for (auto id : members) {
        if (id == promoted_id) {
          continue;
        }
        auto e = manager_->GetEntity(id);
        ASSERT_NE(e, nullptr);
        EXPECT_EQ(e->GetData().seasons().at(1).grade(), 1)
            << "凭据 " << id << " 应留在原段位";
      }
    }

    // 验证恰好有 2 个凭据升段
    auto promoted_count = std::size_t{0};
    for (auto id = std::uint64_t{1}; id <= 4; ++id) {
      auto e = manager_->GetEntity(id);
      ASSERT_NE(e, nullptr);
      if (e->GetData().seasons().at(1).grade() == 2) {
        ++promoted_count;
      }
    }
    EXPECT_EQ(promoted_count, 2);

    // 未配置 reset_score，积分应保持不变
    auto entity1 = manager_->GetEntity(1);
    ASSERT_NE(entity1, nullptr);
    EXPECT_EQ(entity1->GetData().attributes().at(1), 100);
    auto entity4 = manager_->GetEntity(4);
    ASSERT_NE(entity4, nullptr);
    EXPECT_EQ(entity4->GetData().attributes().at(1), 400);
  }
}

// R6.1 端到端集成测试：验证 reset_score=true 时积分重置行为
TEST_F(CMatchIntegrationTest, NextSeasonResetsScoreWhenConfigured) {
  auto info = MakeSeasonInfo(1);
  info.set_reset_score(true);
  info.set_initial_score(1000);
  auto time = MakeSeasonTime(0, 100);
  config_->AddSeason(info, time);

  // 提交凭据并设置积分
  for (auto id = std::uint64_t{1}; id <= 4; ++id) {
    auto req = std::make_shared<lib::SubmitTicketReq>();
    req->mutable_ticket()->set_id(id);
    req->mutable_ticket()->set_zone_id(1);
    (*req->mutable_ticket()->mutable_attributes())[1] = id * 100;
    req->add_registrations(1);

    auto resp = lib::SubmitTicketResp{};
    service_->SubmitTicket(
        req, [&resp](const lib::SubmitTicketResp& r) { resp = r; });
    EXPECT_EQ(resp.result(), lib::OK);
  }

  // 构建分组
  {
    auto rng = std::mt19937{12345};
    auto init_config = MockSeasonConfig{};
    init_config.AddSeason(info, time);
    ticket_manager_->Initialize(init_config, 50, rng);
  }

  // 模拟时间推进，调用 NextSeason 切换赛季
  {
    auto new_time = MakeSeasonTime(100, 200);
    auto rng = std::mt19937{12345};
    ticket_manager_->NextSeason(info, new_time, 150, rng);
  }

  // 验证所有凭据的积分已被重置为 initial_score
  for (auto id = std::uint64_t{1}; id <= 4; ++id) {
    auto entity = manager_->GetEntity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->GetData().attributes().at(1), 1000)
        << "凭据 " << id << " 的积分应被重置为 initial_score";
  }
}

// R6.2 边界场景：空配置
TEST_F(CMatchIntegrationTest, EmptyConfigReturnsEmptySeasonList) {
  auto req = std::make_shared<lib::GetSeasonListReq>();
  auto resp = lib::GetSeasonListResp{};
  service_->GetSeasonList(
      req, [&resp](const lib::GetSeasonListResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  EXPECT_EQ(resp.seasons().size(), 0);
}

// R6.2 边界场景：无凭据时 BuildSeason 不崩溃
TEST_F(CMatchIntegrationTest, BuildSeasonWithNoTicketsDoesNotCrash) {
  auto info = MakeSeasonInfo(1);
  auto time = MakeSeasonTime(0, 100);
  config_->AddSeason(info, time);

  auto rng = std::mt19937{12345};
  auto init_config = MockSeasonConfig{};
  init_config.AddSeason(info, time);
  ticket_manager_->Initialize(init_config, 50, rng);

  // 验证没有凭据存在，且未发生崩溃
  EXPECT_EQ(manager_->GetEntities().size(), 0);
}

// R6.2 边界场景：合服场景，不同 zone_id 的凭据分组 ID 唯一
TEST_F(CMatchIntegrationTest, MergedServerGroupIdUniqueness) {
  auto info = MakeSeasonInfo(1);
  (*info.mutable_grades())[1].set_group_size(1);    // 每个凭据独立分组
  (*info.mutable_grades())[1].set_promote_rank(0);  // 禁止升段
  auto time = MakeSeasonTime(0, 100);
  config_->AddSeason(info, time);

  // 从不同 zone 提交凭据
  for (auto id = std::uint64_t{1}; id <= 3; ++id) {
    auto req = std::make_shared<lib::SubmitTicketReq>();
    req->mutable_ticket()->set_id(id);
    req->mutable_ticket()->set_zone_id(static_cast<std::uint32_t>(id));
    (*req->mutable_ticket()->mutable_attributes())[1] = id * 100;
    req->add_registrations(1);

    auto resp = lib::SubmitTicketResp{};
    service_->SubmitTicket(
        req, [&resp](const lib::SubmitTicketResp& r) { resp = r; });
    EXPECT_EQ(resp.result(), lib::OK);
  }

  // 构建分组
  auto rng = std::mt19937{12345};
  auto init_config = MockSeasonConfig{};
  init_config.AddSeason(info, time);
  ticket_manager_->Initialize(init_config, 50, rng);

  // 验证所有分组 ID 唯一，且每个分组内的凭据均来自同一 zone
  auto group_members =
      std::unordered_map<std::uint64_t, std::vector<std::uint64_t>>{};
  for (const auto& [id, entity] : manager_->GetEntities()) {
    (void)id;
    auto group_id = entity->GetData().seasons().at(1).group_id();
    group_members[group_id].push_back(entity->GetData().id());
  }

  auto group_ids = std::unordered_set<std::uint64_t>{};
  for (const auto& [group_id, members] : group_members) {
    ASSERT_TRUE(group_ids.insert(group_id).second)
        << "分组 ID 重复: " << group_id;

    // 验证每个分组内的凭据均来自同一 zone（group_size=1，每组应只有 1 人）
    ASSERT_EQ(members.size(), 1) << "分组 ID " << group_id << " 的成员数应为 1";
  }
  EXPECT_EQ(group_ids.size(), 3);
}

// R6.2 边界场景：跨赛季修复，凭据时间不一致时修复后行为正确
TEST_F(CMatchIntegrationTest, CrossSeasonRepairAfterTimeMismatch) {
  auto info = MakeSeasonInfo(1);
  auto old_time = MakeSeasonTime(0, 100);
  auto new_time = MakeSeasonTime(100, 200);
  config_->AddSeason(info, new_time);

  // 提交凭据，但模拟旧赛季时间数据
  {
    auto req = std::make_shared<lib::SubmitTicketReq>();
    req->mutable_ticket()->set_id(1);
    req->mutable_ticket()->set_zone_id(1);
    (*req->mutable_ticket()->mutable_attributes())[1] = 100;
    req->add_registrations(1);

    auto resp = lib::SubmitTicketResp{};
    service_->SubmitTicket(
        req, [&resp](const lib::SubmitTicketResp& r) { resp = r; });
    EXPECT_EQ(resp.result(), lib::OK);
  }

  // 模拟凭据数据损坏：该凭据的赛季时间与配置不一致，
  // 用于验证 BuildSeason 能正确检测并修复此类问题
  {
    auto entity = manager_->GetEntity(1);
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(old_time.begin_time());
    group.set_end_time(old_time.end_time());
    group.set_grade(1);
    group.set_group_id(1000);
  }

  // 调用 BuildSeason 修复时间不一致
  auto rng = std::mt19937{12345};
  auto init_config = MockSeasonConfig{};
  init_config.AddSeason(info, new_time);
  ticket_manager_->Initialize(init_config, 150, rng);

  // 验证凭据时间已被修复为新的配置时间，并获得结算记录与重新分组
  auto entity = manager_->GetEntity(1);
  ASSERT_NE(entity, nullptr);
  const auto& group = entity->GetData().seasons().at(1);
  EXPECT_EQ(group.begin_time(), new_time.begin_time());
  EXPECT_EQ(group.end_time(), new_time.end_time());
  EXPECT_TRUE(manager_->IsDirty(1));

  // 验证凭据获得结算记录且排名大于 0
  ASSERT_TRUE(entity->GetData().settlements().contains(1));
  EXPECT_GT(entity->GetData().settlements().at(1).rank(), 0);

  // 验证凭据已被重新分组（group_id 不再是手动设置的 1000）
  EXPECT_NE(group.group_id(), 1000);

  // 验证凭据的赛季数据反映修复后的状态（凭据为唯一成员，应升段）
  EXPECT_EQ(group.type(), 1);
  EXPECT_EQ(group.grade(), 2);
}

}  // namespace
}  // namespace cmatch
