// Protobuf 消息测试

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "cmatch/config.pb.h"
#include "cmatch/table.pb.h"

namespace cmatch {
namespace {

TEST(ProtobufMessagesTest, GradeInfoFields) {
  config::GradeInfo grade;
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
  config::SeasonInfo season;
  season.set_type(1);

  config::GradeInfo& grade = (*season.mutable_grades())[1];
  grade.set_grade(1);
  grade.set_group_size(5);

  EXPECT_EQ(season.type(), 1);
  ASSERT_EQ(season.grades().size(), 1);
  EXPECT_EQ(season.grades().at(1).grade(), 1);
  EXPECT_EQ(season.grades().at(1).group_size(), 5);
}

TEST(ProtobufMessagesTest, SeasonTimeDefaults) {
  config::SeasonTime time;
  EXPECT_EQ(time.begin_time(), 0);
  EXPECT_EQ(time.end_time(), 0);
}

TEST(ProtobufMessagesTest, TicketSerialization) {
  table::Ticket ticket;
  ticket.set_id(1001);
  ticket.set_zone_id(2);
  ticket.set_auto_id(3);
  (*ticket.mutable_attributes())[1] = 100;
  ticket.add_registrations(1);

  table::SeasonGroup& group = (*ticket.mutable_seasons())[1];
  group.set_type(1);
  group.set_begin_time(1000);
  group.set_end_time(2000);
  group.set_grade(1);
  group.set_group_id(0x0000000200000001ULL);

  const std::string serialized = ticket.SerializeAsString();
  table::Ticket parsed;
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
  config::GradeInfo grade;
  EXPECT_EQ(grade.grade(), 0);
  EXPECT_EQ(grade.promote_rank(), 0);
  EXPECT_FLOAT_EQ(grade.promote_rank_percent(), 0.0F);
}

}  // namespace
}  // namespace cmatch
