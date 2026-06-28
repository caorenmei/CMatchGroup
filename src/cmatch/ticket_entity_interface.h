// 凭据实体接口

#ifndef CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_
#define CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_

#include <cstdint>
#include <memory>

#include "cmatch/table.pb.h"

namespace cmatch {

// 凭据实体接口，封装单个凭据的访问能力
class TicketEntityInterface {
 public:
  virtual ~TicketEntityInterface() = default;

  // 获取凭据 ID
  virtual std::uint64_t GetKey() const = 0;

  // 获取凭据数据
  virtual table::Ticket& GetData() = 0;
  virtual const table::Ticket& GetData() const = 0;

  // 判断凭据是否有效
  virtual bool IsFull() const = 0;
};

using TicketEntityPtr = std::shared_ptr<TicketEntityInterface>;

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_
