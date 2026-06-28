// Protobuf 消息测试

#include <gtest/gtest.h>

#include "cmatch/config.pb.h"
#include "cmatch/table.pb.h"

namespace cmatch {
namespace {

TEST(ProtobufMessagesTest, GradeInfoFields) {
  auto grade = config::GradeInfo{};
  grade.set_grade(1);
  grade.set_prev_grade(0);
  grade.set_next_grade(2);
  grade.set_group_size(10);
  grade.set_promote_rank(3);
  grade.set_promote_rank_percent(0.3F);
  grade.set_demote_rank(8);
  grade.set_demote_rank_percent(0.8F);

  EXPECT_EQ(grade.grade(), 1);
  EXPECT_EQ(grade.prev_grade(), 0);
  EXPECT_EQ(grade.next_grade(), 2);
  EXPECT_EQ(grade.group_size(), 10);
  EXPECT_EQ(grade.promote_rank(), 3);
  EXPECT_FLOAT_EQ(grade.promote_rank_percent(), 0.3F);
  EXPECT_EQ(grade.demote_rank(), 8);
  EXPECT_FLOAT_EQ(grade.demote_rank_percent(), 0.8F);
}

TEST(ProtobufMessagesTest, SeasonInfoGradesMap) {
  auto season = config::SeasonInfo{};
  season.set_type(1);

  auto& grade = (*season.mutable_grades())[1];
  grade.set_grade(1);
  grade.set_group_size(5);

  EXPECT_EQ(season.type(), 1);
  ASSERT_EQ(season.grades().size(), 1);
  EXPECT_EQ(season.grades().at(1).grade(), 1);
  EXPECT_EQ(season.grades().at(1).group_size(), 5);
}

TEST(ProtobufMessagesTest, SeasonTimeDefaults) {
  auto time = config::SeasonTime{};
  EXPECT_EQ(time.begin_time(), 0);
  EXPECT_EQ(time.end_time(), 0);
}

TEST(ProtobufMessagesTest, TicketSerialization) {
  auto ticket = table::Ticket{};
  ticket.set_id(1001);
  ticket.set_zone_id(2);
  ticket.set_auto_id(3);
  (*ticket.mutable_attributes())[1] = 100;
  ticket.add_registrations(1);

  auto& group = (*ticket.mutable_seasons())[1];
  group.set_type(1);
  group.set_begin_time(1000);
  group.set_end_time(2000);
  group.set_grade(1);
  group.set_group_id(0x0000000200000001ULL);

  const auto serialized = ticket.SerializeAsString();
  auto parsed = table::Ticket{};
  ASSERT_TRUE(parsed.ParseFromString(serialized));

  EXPECT_EQ(parsed.id(), 1001);
  EXPECT_EQ(parsed.zone_id(), 2);
  EXPECT_EQ(parsed.auto_id(), 3);
  ASSERT_EQ(parsed.attributes().size(), 1);
  EXPECT_EQ(parsed.attributes().at(1), 100);
  ASSERT_EQ(parsed.registrations().size(), 1);
  EXPECT_EQ(parsed.registrations(0), 1);
  ASSERT_EQ(parsed.seasons().size(), 1);
  EXPECT_EQ(parsed.seasons().at(1).group_id(), 0x0000000200000001ULL);
}

TEST(ProtobufMessagesTest, UnsetUint32DefaultsToZero) {
  auto grade = config::GradeInfo{};
  EXPECT_EQ(grade.grade(), 0);
  EXPECT_EQ(grade.promote_rank(), 0);
  EXPECT_FLOAT_EQ(grade.promote_rank_percent(), 0.0F);
}

}  // namespace
}  // namespace cmatch
