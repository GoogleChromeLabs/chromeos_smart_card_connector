// Copyright 2022 Google Inc. All Rights Reserved.
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

#ifndef GOOGLE_SMART_CARD_COMMON_ADMIN_POLICY_GETTER_H_
#define GOOGLE_SMART_CARD_COMMON_ADMIN_POLICY_GETTER_H_

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "common/cpp/src/public/messaging/typed_message_listener.h"
#include "common/cpp/src/public/optional.h"

namespace google_smart_card {

struct AdminPolicy {
  // Force allowed client App identifiers.
  optional<std::vector<std::string>> force_allowed_client_app_ids;

  // Client App identifiers using the SCardDisconnect fallback.
  optional<std::vector<std::string>> scard_disconnect_fallback_client_app_ids;
};

// This class listens for the update admin policy messages received from the
// JavaScript side and stores the current version of the policy.
class AdminPolicyGetter final : public TypedMessageListener {
 public:
  AdminPolicyGetter();
  AdminPolicyGetter(const AdminPolicyGetter&) = delete;
  AdminPolicyGetter& operator=(const AdminPolicyGetter&) = delete;
  ~AdminPolicyGetter();

  // Returns the current admin policy. If it has not been received yet, it waits
  // in a blocking way.
  optional<AdminPolicy> WaitAndGet();

  // Replace the currently cached policy with |admin_policy|.
  void UpdateAdminPolicy(const AdminPolicy& admin_policy);

  // Switches into the "shutting down" state. This makes all ongoing and future
  // `WaitAndGet()` calls return a null optional.
  void ShutDown();

  // TypedMessageListener:
  std::string GetListenedMessageType() const override;
  bool OnTypedMessageReceived(Value data) override;

 private:
  std::mutex mutex_;
  std::condition_variable condition_variable_;
  optional<AdminPolicy> admin_policy_;
  bool shutting_down_ = false;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_ADMIN_POLICY_GETTER_H_
