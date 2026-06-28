// 核心接口与 Mock 行为测试

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "cmatch/config.pb.h"
#include "cmatch/season_config_interface.h"
#include "mock_ticket_entity_manager.h"

namespace cmatch {
namespace {

class MockSeasonConfig : public SeasonConfigInterface {
 public:
  std::vector<std::uint32_t> GetTypes() const override { return {1, 2}; }

  bool GetInfo(std::uint32_t type, config::SeasonInfo& info) const override {
    if (type != 1) {
      return false;
    }
    info.set_type(type);
    return true;
  }

  bool GetTime(std::uint32_t type, config::SeasonTime& time) const override {
    if (type != 1) {
      return false;
    }
    time.set_begin_time(1000);
    time.set_end_time(2000);
    return true;
  }
};

TEST(TicketEntityInterfaceTest, MockEntityBehavior) {
  testing::MockTicketEntity entity(42, 1);
  EXPECT_EQ(entity.GetKey(), 42);
  EXPECT_TRUE(entity.IsFull());

  entity.GetData().set_zone_id(2);
  EXPECT_EQ(entity.GetData().zone_id(), 2);
}

TEST(TicketEntityManagerInterfaceTest, GetOrCreateEntity) {
  testing::MockTicketEntityManager manager;

  TicketEntityPtr entity = manager.GetOrCreateEntity(100, 1);
  ASSERT_NE(entity, nullptr);
  EXPECT_EQ(entity->GetKey(), 100);
  EXPECT_EQ(entity->GetData().zone_id(), 1);

  TicketEntityPtr same = manager.GetOrCreateEntity(100, 2);
  EXPECT_EQ(same, entity);
  EXPECT_EQ(entity->GetData().zone_id(), 1);
}

TEST(TicketEntityManagerInterfaceTest, GetEntityMissing) {
  testing::MockTicketEntityManager manager;
  EXPECT_EQ(manager.GetEntity(999), nullptr);
}

TEST(TicketEntityManagerInterfaceTest, GetEntitiesAndDirty) {
  testing::MockTicketEntityManager manager;

  manager.GetOrCreateEntity(1, 1);
  manager.GetOrCreateEntity(2, 1);
  manager.SetDirty(1);

  EXPECT_EQ(manager.GetEntities().size(), 2);
  EXPECT_TRUE(manager.IsDirty(1));
  EXPECT_FALSE(manager.IsDirty(2));
}

TEST(TicketEntityManagerInterfaceTest, IsLoaded) {
  testing::MockTicketEntityManager manager;
  EXPECT_TRUE(manager.IsLoaded());

  manager.SetLoaded(false);
  EXPECT_FALSE(manager.IsLoaded());
}

TEST(SeasonConfigInterfaceTest, MockConfigBehavior) {
  MockSeasonConfig config;

  std::vector<std::uint32_t> types = config.GetTypes();
  ASSERT_EQ(types.size(), 2);
  EXPECT_EQ(types[0], 1);
  EXPECT_EQ(types[1], 2);

  config::SeasonInfo info;
  EXPECT_TRUE(config.GetInfo(1, info));
  EXPECT_EQ(info.type(), 1);

  config::SeasonTime time;
  EXPECT_TRUE(config.GetTime(1, time));
  EXPECT_EQ(time.begin_time(), 1000);
  EXPECT_EQ(time.end_time(), 2000);

  EXPECT_FALSE(config.GetInfo(2, info));
  EXPECT_FALSE(config.GetTime(2, time));
}

}  // namespace
}  // namespace cmatch
