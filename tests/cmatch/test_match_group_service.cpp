// 匹配分组服务测试

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <memory>
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

  return info;
}

config::SeasonTime MakeSeasonTime(std::uint64_t begin, std::uint64_t end) {
  config::SeasonTime time;
  time.set_begin_time(begin);
  time.set_end_time(end);
  return time;
}

class MatchGroupServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    info_ = MakeSeasonInfo(1);
    time_ = MakeSeasonTime(0, 100);
    config_ = std::make_unique<MockSeasonConfig>(info_);
    config_->SetTime(time_);
    manager_ = std::make_unique<testing::MockTicketEntityManager>();
    ticket_manager_ = std::make_unique<TicketManager>(*manager_);
    service_ = std::make_unique<MatchGroupServiceImpl>(*config_, *manager_,
                                                       *ticket_manager_);
  }

  config::SeasonInfo info_;
  config::SeasonTime time_;
  std::unique_ptr<MockSeasonConfig> config_;
  std::unique_ptr<testing::MockTicketEntityManager> manager_;
  std::unique_ptr<TicketManager> ticket_manager_;
  std::unique_ptr<MatchGroupServiceImpl> service_;
};

TEST_F(MatchGroupServiceTest, GetSeasonListReturnsConfiguredSeasons) {
  auto req = std::make_shared<lib::GetSeasonListReq>();
  lib::GetSeasonListResp resp;
  service_->GetSeasonList(
      req, [&resp](const lib::GetSeasonListResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  ASSERT_EQ(resp.seasons().size(), 1);
  EXPECT_EQ(resp.seasons(0).type(), 1);
  EXPECT_EQ(resp.seasons(0).begin_time(), 0);
  EXPECT_EQ(resp.seasons(0).end_time(), 100);
}

TEST_F(MatchGroupServiceTest, SubmitTicketCreatesEntity) {
  auto req = std::make_shared<lib::SubmitTicketReq>();
  req->mutable_ticket()->set_id(100);
  req->mutable_ticket()->set_zone_id(2);
  (*req->mutable_ticket()->mutable_attributes())[1] = 500;
  req->add_registrations(1);

  lib::SubmitTicketResp resp;
  service_->SubmitTicket(req,
                         [&resp](const lib::SubmitTicketResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  auto entity = manager_->GetEntity(100);
  ASSERT_NE(entity, nullptr);
  EXPECT_EQ(entity->GetData().zone_id(), 2);
  EXPECT_EQ(entity->GetData().attributes().at(1), 500);
  EXPECT_TRUE(manager_->IsDirty(100));
}

TEST_F(MatchGroupServiceTest, SubmitTicketInvalidParameter) {
  auto req = std::make_shared<lib::SubmitTicketReq>();
  req->mutable_ticket()->set_id(0);
  req->mutable_ticket()->set_zone_id(1);

  lib::SubmitTicketResp resp;
  service_->SubmitTicket(req,
                         [&resp](const lib::SubmitTicketResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::INVALID_PARAMETER);
}

TEST_F(MatchGroupServiceTest, GetTicketReturnsData) {
  auto entity = manager_->GetOrCreateEntity(42, 3);
  (*entity->GetData().mutable_attributes())[1] = 123;
  entity->GetData().add_registrations(1);
  {
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(999);
  }

  auto req = std::make_shared<lib::GetTicketReq>();
  req->set_id(42);

  lib::GetTicketResp resp;
  service_->GetTicket(req, [&resp](const lib::GetTicketResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  EXPECT_EQ(resp.ticket().id(), 42);
  EXPECT_EQ(resp.ticket().zone_id(), 3);
  EXPECT_EQ(resp.ticket().attributes().at(1), 123);
  ASSERT_EQ(resp.registrations().size(), 1);
  EXPECT_EQ(resp.registrations(0), 1);
  ASSERT_EQ(resp.seasons().size(), 1);
  EXPECT_EQ(resp.seasons().at(1).group_id(), 999);
}

TEST_F(MatchGroupServiceTest, GetTicketNotFound) {
  auto req = std::make_shared<lib::GetTicketReq>();
  req->set_id(999);

  lib::GetTicketResp resp;
  service_->GetTicket(req, [&resp](const lib::GetTicketResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::TICKET_NOT_FOUND);
}

TEST_F(MatchGroupServiceTest, RegisterSeasonAddsRegistrations) {
  auto entity = manager_->GetOrCreateEntity(7, 1);
  lib::RegisterSeasonResp resp;

  auto req = std::make_shared<lib::RegisterSeasonReq>();
  req->set_id(7);
  req->add_types(1);

  service_->RegisterSeason(
      req, [&resp](const lib::RegisterSeasonResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  ASSERT_EQ(entity->GetData().registrations().size(), 1);
  EXPECT_EQ(entity->GetData().registrations(0), 1);
  EXPECT_TRUE(manager_->IsDirty(7));
}

TEST_F(MatchGroupServiceTest, RegisterSeasonTicketNotFound) {
  auto req = std::make_shared<lib::RegisterSeasonReq>();
  req->set_id(999);
  req->add_types(1);

  lib::RegisterSeasonResp resp;
  service_->RegisterSeason(
      req, [&resp](const lib::RegisterSeasonResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::TICKET_NOT_FOUND);
}

TEST_F(MatchGroupServiceTest, RegisterSeasonNotFound) {
  manager_->GetOrCreateEntity(8, 1);
  auto req = std::make_shared<lib::RegisterSeasonReq>();
  req->set_id(8);
  req->add_types(999);

  lib::RegisterSeasonResp resp;
  service_->RegisterSeason(
      req, [&resp](const lib::RegisterSeasonResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::SEASON_NOT_FOUND);
}

TEST_F(MatchGroupServiceTest, GetTicketListSkipsMissing) {
  auto entity = manager_->GetOrCreateEntity(10, 1);
  entity->GetData().set_zone_id(1);

  auto req = std::make_shared<lib::GetTicketListReq>();
  req->add_ids(10);
  req->add_ids(11);

  lib::GetTicketListResp resp;
  service_->GetTicketList(
      req, [&resp](const lib::GetTicketListResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  ASSERT_EQ(resp.tickets().size(), 1);
  EXPECT_EQ(resp.tickets(0).id(), 10);
}

TEST_F(MatchGroupServiceTest, GetGroupMembersReturnsMap) {
  auto entity = manager_->GetOrCreateEntity(20, 5);
  {
    auto& group = (*entity->GetData().mutable_seasons())[1];
    group.set_type(1);
    group.set_begin_time(0);
    group.set_end_time(100);
    group.set_grade(1);
    group.set_group_id(555);
  }

  auto req = std::make_shared<lib::GetGroupMembersReq>();
  req->set_type(1);
  req->set_group_id(555);

  lib::GetGroupMembersResp resp;
  service_->GetGroupMembers(
      req, [&resp](const lib::GetGroupMembersResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  ASSERT_EQ(resp.members().size(), 1);
  EXPECT_EQ(resp.members().at(20), 5);
}

TEST_F(MatchGroupServiceTest, GetSettlementListReturnsData) {
  auto entity = manager_->GetOrCreateEntity(30, 1);
  {
    auto& settlement = (*entity->GetData().mutable_settlements())[1];
    settlement.set_type(1);
    settlement.set_rank(1);
    settlement.set_rank_percent(0.5F);
  }

  auto req = std::make_shared<lib::GetSettlementListReq>();
  req->set_id(30);

  lib::GetSettlementListResp resp;
  service_->GetSettlementList(
      req, [&resp](const lib::GetSettlementListResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  ASSERT_EQ(resp.settlements().size(), 1);
  EXPECT_EQ(resp.settlements(0).rank(), 1);
}

TEST_F(MatchGroupServiceTest, GetSettlementListTicketNotFound) {
  auto req = std::make_shared<lib::GetSettlementListReq>();
  req->set_id(999);

  lib::GetSettlementListResp resp;
  service_->GetSettlementList(
      req, [&resp](const lib::GetSettlementListResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::TICKET_NOT_FOUND);
}

TEST_F(MatchGroupServiceTest, RemoveSettlementListErasesKey) {
  auto entity = manager_->GetOrCreateEntity(40, 1);
  {
    auto& settlement = (*entity->GetData().mutable_settlements())[1];
    settlement.set_type(1);
    settlement.set_rank(1);
  }

  auto req = std::make_shared<lib::RemoveSettlementListReq>();
  req->set_id(40);
  req->add_settlement_ids(1);

  lib::RemoveSettlementListResp resp;
  service_->RemoveSettlementList(
      req, [&resp](const lib::RemoveSettlementListResp& r) { resp = r; });

  EXPECT_EQ(resp.result(), lib::OK);
  EXPECT_EQ(entity->GetData().settlements().count(1), 0);
  EXPECT_TRUE(manager_->IsDirty(40));
}

}  // namespace
}  // namespace cmatch
