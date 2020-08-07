// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RCLCPP__CALLBACK_GROUP_HPP_
#define RCLCPP__CALLBACK_GROUP_HPP_

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "rclcpp/client.hpp"
#include "rclcpp/publisher_base.hpp"
#include "rclcpp/service.hpp"
#include "rclcpp/subscription_base.hpp"
#include "rclcpp/timer.hpp"
#include "rclcpp/visibility_control.hpp"
#include "rclcpp/waitable.hpp"

namespace rclcpp
{

// Forward declarations for friend statement in class CallbackGroup
namespace node_interfaces
{
class NodeServices;
class NodeTimers;
class NodeTopics;
class NodeWaitables;
}  // namespace node_interfaces

enum class CallbackGroupType
{
  MutuallyExclusive,
  Reentrant
};

class CallbackGroup
{
  friend class rclcpp::node_interfaces::NodeServices;
  friend class rclcpp::node_interfaces::NodeTimers;
  friend class rclcpp::node_interfaces::NodeTopics;
  friend class rclcpp::node_interfaces::NodeWaitables;

public:
  RCLCPP_SMART_PTR_DEFINITIONS(CallbackGroup)

  /// Constructor for CallbackGroup
  /**
   * When the user creates a callback group, the user needs
   * to choose the type of callback group desired: `Mutually Exclusive`
   * or 'Reentrant'. The type desired will depend on the application:
   * Callbacks in Reentrant Callback Groups must be able to:
   *      - run at the same time as themselves (reentrant)
   *      - run at the same time as other callbacks in their group
   *      - run at the same time as other callbacks in other groups
   * Whereas, callbacks in Mutually Exclusive Callback Groups:
   *      - will not be run multiple times simultaneously (non-reentrant)
   *      - will not be run at the same time as other callbacks in their group
   *      - but must run at the same time as callbacks in other groups
   * Additiionally, the callback group can be added manually or automatically.
   * To add manually and do not desire the executor to automatically add
   * a callback group to an executor associated with the node that manages
   * the callback group, set automatically_add_to_executor_with_node to
   * false. Otherwise, true. Note when manually adding a callback group,
   * use the add_callback_group function from the executor when the
   * callback group is created. For an executor to automatically add
   * a callback group, the node of the callback group needs to be associated
   * with an executor. In order to associate the node with an executor,
   * use the `add_node` function from the executor. Whether you added
   * the node to the executor after or before creating a callback group
   * is irrelevant; the callback group will be added by the executor in
   * any case.
   * \param[in] group_type They type of callback group that a user wants.
   * \param[in] automatically_add_to_executor_with_node a
   * boolean that determines whether a callback group is added to the
   * executor that a node is associated with.
   */
  RCLCPP_PUBLIC
  explicit CallbackGroup(
    CallbackGroupType group_type,
    bool automatically_add_to_executor_with_node = true);

  template<typename Function>
  rclcpp::SubscriptionBase::SharedPtr
  find_subscription_ptrs_if(Function func) const
  {
    return _find_ptrs_if_impl<rclcpp::SubscriptionBase, Function>(func, subscription_ptrs_);
  }

  template<typename Function>
  rclcpp::TimerBase::SharedPtr
  find_timer_ptrs_if(Function func) const
  {
    return _find_ptrs_if_impl<rclcpp::TimerBase, Function>(func, timer_ptrs_);
  }

  template<typename Function>
  rclcpp::ServiceBase::SharedPtr
  find_service_ptrs_if(Function func) const
  {
    return _find_ptrs_if_impl<rclcpp::ServiceBase, Function>(func, service_ptrs_);
  }

  template<typename Function>
  rclcpp::ClientBase::SharedPtr
  find_client_ptrs_if(Function func) const
  {
    return _find_ptrs_if_impl<rclcpp::ClientBase, Function>(func, client_ptrs_);
  }

  template<typename Function>
  rclcpp::Waitable::SharedPtr
  find_waitable_ptrs_if(Function func) const
  {
    return _find_ptrs_if_impl<rclcpp::Waitable, Function>(func, waitable_ptrs_);
  }

  RCLCPP_PUBLIC
  std::atomic_bool &
  can_be_taken_from();

  RCLCPP_PUBLIC
  const CallbackGroupType &
  type() const;

  /// Return executor association atomic boolean
  /**
   * When a callback group is added to the executor,
   * we want to make sure that another executor
   * does not add this callback group.
   * When the callback group is removed from the executor,
   * this atomic boolean is set to false.
   * \return atomic boolean for association with executor
   */
  RCLCPP_PUBLIC
  std::atomic_bool &
  get_associated_with_executor_atomic();

  /// A boolean that determines whether a callback group
  /// can be added (i.e., in an 'allowable' state)
  /**
   * When a callback group is created, the user needs
   * to determine if a callback group can be added
   * automatically when a node is added to the executor.
   * Boolean is checked when a node is added to an
   * executor and before memory strategy collects entities
   * to add any callback group that was added after a node
   * is added to an executor
   * \return boolean that allows an executor to automatically
   * add a callback group
   */
  RCLCPP_PUBLIC
  const bool &
  automatically_add_to_executor_with_node() const {return automatically_add_to_executor_with_node_;}

protected:
  RCLCPP_DISABLE_COPY(CallbackGroup)

  RCLCPP_PUBLIC
  void
  add_publisher(const rclcpp::PublisherBase::SharedPtr publisher_ptr);

  RCLCPP_PUBLIC
  void
  add_subscription(const rclcpp::SubscriptionBase::SharedPtr subscription_ptr);

  RCLCPP_PUBLIC
  void
  add_timer(const rclcpp::TimerBase::SharedPtr timer_ptr);

  RCLCPP_PUBLIC
  void
  add_service(const rclcpp::ServiceBase::SharedPtr service_ptr);

  RCLCPP_PUBLIC
  void
  add_client(const rclcpp::ClientBase::SharedPtr client_ptr);

  RCLCPP_PUBLIC
  void
  add_waitable(const rclcpp::Waitable::SharedPtr waitable_ptr);

  RCLCPP_PUBLIC
  void
  remove_waitable(const rclcpp::Waitable::SharedPtr waitable_ptr) noexcept;

  CallbackGroupType type_;
  // Mutex to protect the subsequent vectors of pointers.
  mutable std::mutex mutex_;
  std::atomic_bool associated_with_executor_;
  std::vector<rclcpp::SubscriptionBase::WeakPtr> subscription_ptrs_;
  std::vector<rclcpp::TimerBase::WeakPtr> timer_ptrs_;
  std::vector<rclcpp::ServiceBase::WeakPtr> service_ptrs_;
  std::vector<rclcpp::ClientBase::WeakPtr> client_ptrs_;
  std::vector<rclcpp::Waitable::WeakPtr> waitable_ptrs_;
  std::atomic_bool can_be_taken_from_;
  const bool automatically_add_to_executor_with_node_;

private:
  template<typename TypeT, typename Function>
  typename TypeT::SharedPtr _find_ptrs_if_impl(
    Function func, const std::vector<typename TypeT::WeakPtr> & vect_ptrs) const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto & weak_ptr : vect_ptrs) {
      auto ref_ptr = weak_ptr.lock();
      if (ref_ptr && func(ref_ptr)) {
        return ref_ptr;
      }
    }
    return typename TypeT::SharedPtr();
  }
};

namespace callback_group
{

using CallbackGroupType [[deprecated("use rclcpp::CallbackGroupType instead")]] = CallbackGroupType;
using CallbackGroup [[deprecated("use rclcpp::CallbackGroup instead")]] = CallbackGroup;

}  // namespace callback_group
}  // namespace rclcpp

#endif  // RCLCPP__CALLBACK_GROUP_HPP_
