// 凭据实体管理器接口

#ifndef CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_
#define CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_

#include <cstdint>
#include <unordered_map>

#include "cmatch/ticket_entity_interface.h"

namespace cmatch {

/// @brief 凭据实体管理器接口
///
/// 负责凭据实体的生命周期管理、全局访问与脏数据标记。
/// 具体实现可基于内存、数据库缓存或其他持久化策略。
class TicketEntityManagerInterface {
 public:
  virtual ~TicketEntityManagerInterface() = default;

  /// @brief 获取指定 ID 的凭据实体
  /// @param[in] id 凭据 ID
  /// @return 凭据实体指针，不存在返回 nullptr
  virtual TicketEntityPtr GetEntity(std::uint64_t id) const = 0;

  /// @brief 获取或创建指定 ID 的凭据实体
  /// @param[in] id 凭据 ID
  /// @param[in] zone_id 区域 ID
  /// @return 凭据实体指针
  virtual TicketEntityPtr GetOrCreateEntity(std::uint64_t id,
                                            std::uint32_t zone_id) = 0;

  /// @brief 获取所有凭据实体
  /// @return 凭据实体映射，key 为凭据 ID
  virtual const std::unordered_map<std::uint64_t, TicketEntityPtr>&
  GetEntities() const = 0;

  /// @brief 设置凭据实体为脏数据
  ///
  /// 由实体管理器负责在适当时机将脏数据保存到数据库。
  /// @param[in] id 凭据 ID
  virtual void SetDirty(std::uint64_t id) = 0;

  /// @brief 判断实体管理器是否已完成加载
  /// @return 已加载返回 true
  virtual bool IsLoaded() const = 0;
};

}  // namespace cmatch

#endif  // CMATCH_SRC_CMATCH_TICKET_ENTITY_MANAGER_INTERFACE_H_
