// 凭据实体接口

#ifndef CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_
#define CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_

#include <cstdint>
#include <memory>

#include "cmatch/table.pb.h"

namespace cmatch {

/// @brief 凭据实体接口
///
/// 封装单个凭据的访问能力，是 TicketEntityManagerInterface 管理的基本单元。
class TicketEntityInterface {
 public:
  virtual ~TicketEntityInterface() = default;

  /// @brief 获取凭据 ID
  /// @return 凭据唯一标识
  virtual std::uint64_t GetKey() const = 0;

  /// @brief 获取凭据数据（可修改）
  /// @return 凭据 Protobuf 数据引用
  virtual table::Ticket& GetData() = 0;

  /// @brief 获取凭据数据（只读）
  /// @return 凭据 Protobuf 数据常量引用
  virtual const table::Ticket& GetData() const = 0;

  /// @brief 判断凭据是否有效
  /// @return 有效返回 true，否则返回 false
  virtual bool IsFull() const = 0;
};

/// @brief 凭据实体智能指针
using TicketEntityPtr = std::shared_ptr<TicketEntityInterface>;

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_ENTITY_INTERFACE_H_
