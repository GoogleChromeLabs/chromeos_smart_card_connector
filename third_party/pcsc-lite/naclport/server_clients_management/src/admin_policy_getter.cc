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

#include "third_party/pcsc-lite/naclport/server_clients_management/src/admin_policy_getter.h"

#include <condition_variable>
#include <string>

#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/cpp/src/public/value_debug_dumping.h"

namespace google_smart_card {

namespace {

// Message type used to signal updates to the admin policy. This must match the
// constant in
// //third_party/pcsc-lite/naclport/server_clients_management/src/admin-policy-service.js.
constexpr char kUpdateAdminPolicyMessageType[] = "update_admin_policy";

}  // namespace

template <>
StructValueDescriptor<AdminPolicy>::Description
StructValueDescriptor<AdminPolicy>::GetDescription() {
  // Note: Strings passed to WithField() below must match the keys in
  // //smart_card_connector/src/managed_storage_schema.json
  return Describe("AdminPolicy")
      .WithField(&AdminPolicy::force_allowed_client_app_ids,
                 "force_allowed_client_app_ids")
      .WithField(&AdminPolicy::scard_disconnect_fallback_client_app_ids,
                 "scard_disconnect_fallback_client_app_ids")
      .PermitUnknownFields();
}

AdminPolicyGetter::AdminPolicyGetter() = default;

AdminPolicyGetter::~AdminPolicyGetter() {
  if (!shutting_down_)
    ShutDown();
}

optional<AdminPolicy> AdminPolicyGetter::WaitAndGet() {
  // Wait until the policy is received for the first time
  std::unique_lock<std::mutex> lock(mutex_);
  condition_variable_.wait(
      lock, [this]() { return shutting_down_ || admin_policy_; });
  if (shutting_down_)
    return {};
  return admin_policy_;
}

void AdminPolicyGetter::ShutDown() {
  std::unique_lock<std::mutex> lock(mutex_);
  shutting_down_ = true;
  condition_variable_.notify_all();
}

std::string AdminPolicyGetter::GetListenedMessageType() const {
  return kUpdateAdminPolicyMessageType;
}

bool AdminPolicyGetter::OnTypedMessageReceived(Value data) {
  AdminPolicy message_data;
  std::string error_message;
  if (!ConvertFromValue(std::move(data), &message_data, &error_message)) {
    GOOGLE_SMART_CARD_LOG_WARNING << "Failed to parse admin policy message: "
                                  << error_message;
    // Continue in order to pretend that an empty policy value was received to
    // unblock WaitAndGet() callers.
  }

  UpdateAdminPolicy(message_data);
  return true;
}

void AdminPolicyGetter::UpdateAdminPolicy(const AdminPolicy& admin_policy) {
  const std::unique_lock<std::mutex> lock(mutex_);

  Value value;
  if (ConvertToValue(admin_policy, &value)) {
    GOOGLE_SMART_CARD_LOG_INFO
        << "Received the following policy data from the managed storage: "
        << DebugDumpValueFull(value);
  }
  admin_policy_ = admin_policy;

  condition_variable_.notify_all();
}

}  // namespace google_smart_card
