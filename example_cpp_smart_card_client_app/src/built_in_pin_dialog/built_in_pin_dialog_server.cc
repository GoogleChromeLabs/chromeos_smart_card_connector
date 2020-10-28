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

#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

namespace smart_card_client {

namespace {

// Note: These parameters should stay in sync with the JS side
// (pin-dialog-backend.js).
constexpr char kRequesterName[] = "built_in_pin_dialog";
constexpr char kPinMessageKey[] = "pin";

void ExtractPinRequestResult(
    const google_smart_card::GenericRequestResult& request_result,
    std::string* pin) {
  // TODO: Parse `Value` directly, without converting into `pp::Var`.
  const pp::VarDictionary result_var =
      google_smart_card::VarAs<pp::VarDictionary>(
          google_smart_card::ConvertValueToPpVar(request_result.payload()));

  google_smart_card::VarDictValuesExtractor extractor(result_var);
  extractor.Extract(kPinMessageKey, pin);
  extractor.CheckSuccessWithNoExtraKeysAllowed();
}

}  // namespace

BuiltInPinDialogServer::BuiltInPinDialogServer(
    google_smart_card::TypedMessageRouter* typed_message_router,
    pp::Instance* pp_instance, pp::Core* pp_core)
    : js_requester_(kRequesterName, typed_message_router,
                    google_smart_card::MakeUnique<
                        google_smart_card::JsRequester::PpDelegateImpl>(
                        pp_instance, pp_core)) {}

BuiltInPinDialogServer::~BuiltInPinDialogServer() = default;

void BuiltInPinDialogServer::Detach() { js_requester_.Detach(); }

bool BuiltInPinDialogServer::RequestPin(std::string* pin) {
  const google_smart_card::GenericRequestResult request_result =
      js_requester_.PerformSyncRequest(/*payload=*/google_smart_card::Value());
  if (!request_result.is_successful()) return false;
  ExtractPinRequestResult(request_result, pin);
  return true;
}

}  // namespace smart_card_client
