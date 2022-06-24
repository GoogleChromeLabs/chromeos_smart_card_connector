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

#include "admin_policy_getter.h"

#include <string>

#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

template <>
StructValueDescriptor<AdminPolicy>::Description
StructValueDescriptor<AdminPolicy>::GetDescription() {
  // Note: Strings passed to WithField() below must match the keys in
  // //third_party/pcsc-lite/naclport/server_clients_management/src/client-handler.js.
  return Describe("AdminPolicy")
      .WithField(&AdminPolicy::force_allowed_client_app_ids,
                 "force_allowed_client_app_ids")
      .WithField(&AdminPolicy::scard_disconnect_fallback_client_app_ids,
                 "scard_disconnect_fallback_client_app_ids");
}

std::string DebugDumpAdminPolicyString(const AdminPolicy admin_policy) {
  std::string force_allowed_client_app_ids;
  for (const std::string& item : admin_policy.force_allowed_client_app_ids) {
    if (!force_allowed_client_app_ids.empty())
      force_allowed_client_app_ids += ", ";
    force_allowed_client_app_ids += "\"" + item + "\"";
  }
  std::string scard_disconnect_fallback_client_app_ids;
  for (const std::string& item :
       admin_policy.scard_disconnect_fallback_client_app_ids) {
    if (!scard_disconnect_fallback_client_app_ids.empty())
      scard_disconnect_fallback_client_app_ids += ", ";
    scard_disconnect_fallback_client_app_ids += "\"" + item + "\"";
  }
  return "{[" + force_allowed_client_app_ids + "], [" +
         scard_disconnect_fallback_client_app_ids + "]}";
}

AdminPolicyGetter::AdminPolicyGetter() = default;

AdminPolicyGetter::~AdminPolicyGetter() = default;

AdminPolicy AdminPolicyGetter::Get() {
  return admin_policy_;
}

void AdminPolicyGetter::UpdateAdminPolicy(const AdminPolicy admin_policy) {
  GOOGLE_SMART_CARD_LOG_INFO
      << "Received the following policy data from the managed storage: "
      << DebugDumpAdminPolicyString(admin_policy);

  admin_policy_ = admin_policy;
}

}  // namespace google_smart_card
