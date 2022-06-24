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

#include <string>
#include <vector>

namespace google_smart_card {

struct AdminPolicy {
  // Force allowed client App identifiers.
  std::vector<std::string> force_allowed_client_app_ids;

  // Client App identifiers using the SCardDisconnect fallback.
  std::vector<std::string> scard_disconnect_fallback_client_app_ids;
};

std::string DebugDumpAdminPolicyString(AdminPolicy admin_policy);

// This class is used to cache a current version of the AdminPolicy.
class AdminPolicyGetter final {
 public:
  AdminPolicyGetter();
  AdminPolicyGetter(const AdminPolicyGetter&) = delete;
  AdminPolicyGetter& operator=(const AdminPolicyGetter&) = delete;
  ~AdminPolicyGetter();

  // Returns the current AdminPolicy.
  AdminPolicy Get();

  // Replace the currently cached policy with |admin_policy|.
  void UpdateAdminPolicy(const AdminPolicy admin_policy);

 private:
  AdminPolicy admin_policy_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_ADMIN_POLICY_GETTER_H_
