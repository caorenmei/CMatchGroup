// 内存 Mock 凭据实体与管理器

#ifndef CMATCH_TESTS_CMATCH_MOCK_TICKET_ENTITY_MANAGER_H_
#define CMATCH_TESTS_CMATCH_MOCK_TICKET_ENTITY_MANAGER_H_

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "cmatch/table.pb.h"
#include "cmatch/ticket_entity_interface.h"
#include "cmatch/ticket_entity_manager_interface.h"

namespace cmatch {
namespace testing {

// 内存凭据实体实现
class MockTicketEntity : public TicketEntityInterface {
 public:
  explicit MockTicketEntity(std::uint64_t id, std::uint32_t zone_id) {
    data_.set_id(id);
    data_.set_zone_id(zone_id);
  }

  std::uint64_t GetKey() const override { return data_.id(); }

  table::Ticket& GetData() override { return data_; }
  const table::Ticket& GetData() const override { return data_; }

  bool IsFull() const override { return true; }

 private:
  table::Ticket data_;
};

// 内存凭据实体管理器实现
class MockTicketEntityManager : public TicketEntityManagerInterface {
 public:
  MockTicketEntityManager() = default;

  TicketEntityPtr GetEntity(std::uint64_t id) const override {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
      return nullptr;
    }
    return it->second;
  }

  TicketEntityPtr GetOrCreateEntity(std::uint64_t id,
                                    std::uint32_t zone_id) override {
    auto it = entities_.find(id);
    if (it != entities_.end()) {
      return it->second;
    }
    auto entity = std::make_shared<MockTicketEntity>(id, zone_id);
    entities_[id] = entity;
    return entity;
  }

  const std::unordered_map<std::uint64_t, TicketEntityPtr>& GetEntities()
      const override {
    return entities_;
  }

  void SetDirty(std::uint64_t id) override { dirty_.insert(id); }

  bool IsLoaded() const override { return loaded_; }

  void SetLoaded(bool loaded) { loaded_ = loaded; }

  bool IsDirty(std::uint64_t id) const { return dirty_.contains(id); }

 private:
  std::unordered_map<std::uint64_t, TicketEntityPtr> entities_;
  std::unordered_set<std::uint64_t> dirty_;
  bool loaded_ = true;
};

}  // namespace testing
}  // namespace cmatch

#endif  // CMATCH_TESTS_CMATCH_MOCK_TICKET_ENTITY_MANAGER_H_
