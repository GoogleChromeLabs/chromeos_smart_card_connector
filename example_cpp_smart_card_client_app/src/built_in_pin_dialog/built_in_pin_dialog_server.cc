// Copyright 2020 Google Inc. All Rights Reserved.
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

#include "built_in_pin_dialog/built_in_pin_dialog_server.h"

#include <string>
#include <utility>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>

namespace gsc = google_smart_card;
namespace scc = smart_card_client;

namespace smart_card_client {

namespace {

// Note: This parameter should stay in sync with the JS side
// (pin-dialog-backend.js).
constexpr char kRequesterName[] = "built_in_pin_dialog";

// This structure that holds the result of the built-in PIN dialog.
struct BuiltInPinDialogResponse {
  std::string pin;
};

}  // namespace

BuiltInPinDialogServer::BuiltInPinDialogServer(
    gsc::GlobalContext* global_context,
    gsc::TypedMessageRouter* typed_message_router)
    : js_requester_(kRequesterName, global_context, typed_message_router) {}

BuiltInPinDialogServer::~BuiltInPinDialogServer() = default;

void BuiltInPinDialogServer::Detach() {
  js_requester_.Detach();
}

bool BuiltInPinDialogServer::RequestPin(std::string* pin) {
  gsc::GenericRequestResult request_result = js_requester_.PerformSyncRequest(
      /*payload=*/gsc::Value(gsc::Value::Type::kDictionary));
  if (!request_result.is_successful())
    return false;
  BuiltInPinDialogResponse response =
      gsc::ConvertFromValueOrDie<BuiltInPinDialogResponse>(
          std::move(request_result).TakePayload());
  *pin = std::move(response.pin);
  return true;
}

}  // namespace smart_card_client

namespace google_smart_card {

// Register `scc::BuiltInPinDialogResponse` for conversions to/from `Value`.
template <>
StructValueDescriptor<scc::BuiltInPinDialogResponse>::Description
StructValueDescriptor<scc::BuiltInPinDialogResponse>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // pin-dialog-backend.js.
  return Describe("BuiltInPinDialogResponse")
      .WithField(&scc::BuiltInPinDialogResponse::pin, "pin");
}

}  // namespace google_smart_card
