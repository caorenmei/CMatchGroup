// 凭据实体管理器接口

#ifndef CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_
#define CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_

#include <cstdint>
#include <unordered_map>

#include "cmatch/ticket_entity_interface.h"

namespace cmatch {

// 凭据实体管理器接口，负责凭据实体的生命周期与脏数据标记
class TicketEntityManagerInterface {
 public:
  virtual ~TicketEntityManagerInterface() = default;

  // 获取凭据实体
  virtual TicketEntityPtr GetEntity(std::uint64_t id) const = 0;

  // 获取或创建凭据实体
  virtual TicketEntityPtr GetOrCreateEntity(std::uint64_t id,
                                            std::uint32_t zone_id) = 0;

  // 获取所有凭据实体
  virtual const std::unordered_map<std::uint64_t, TicketEntityPtr>&
  GetEntities() const = 0;

  // 设置凭据实体为脏数据，由实体管理器负责保存脏数据到数据库
  virtual void SetDirty(std::uint64_t id) = 0;

  // 判断实体管理器是否已加载
  virtual bool IsLoaded() const = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_
