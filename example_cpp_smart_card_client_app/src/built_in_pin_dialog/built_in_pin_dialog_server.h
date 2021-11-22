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

#ifndef SMART_CARD_CLIENT_BUILT_IN_PIN_DIALOG_BUILT_IN_PIN_DIALOG_SERVER_H_
#define SMART_CARD_CLIENT_BUILT_IN_PIN_DIALOG_BUILT_IN_PIN_DIALOG_SERVER_H_

#include <string>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/js_requester.h>

namespace smart_card_client {

// This class allows to request the built-in PIN dialog.
//
// NOTE: This should only be used for the PIN requests that aren't associated
// with signature requests made by Chrome, since for those the
// chrome.certificateProvider.requestPin() method should be used.
//
// Implementation notes:
// * A PIN request is sent to the JavaScript side as a message with a generated
//   request id. Response from the JavaScript side, once the PIN dialog
//   finishes, is received as an incoming message containing the request id,
//   whether the dialog finished successfully and, if yes, the data entered by
//   user.
// * Using request ids allows to operate with multiple PIN requests at the same
//   time.
class BuiltInPinDialogServer final {
 public:
  // Creates the object and an internal JsRequester object, which adds a route
  // into the specified TypedMessageRouter for receiving the request responses.
  // `global_context` - must outlive `this`.
  BuiltInPinDialogServer(
      google_smart_card::GlobalContext* global_context,
      google_smart_card::TypedMessageRouter* typed_message_router);

  BuiltInPinDialogServer(const BuiltInPinDialogServer&) = delete;
  BuiltInPinDialogServer& operator=(const BuiltInPinDialogServer&) = delete;

  ~BuiltInPinDialogServer();

  // Stops sending any further requests to the JavaScript side and prevents
  // receiving responses from it.
  //
  // This function is primarily intended to be used during the executable
  // shutdown process, for preventing the situations when some other threads
  // currently executing PIN requests would trigger accesses to already
  // destroyed objects.
  //
  // This function is safe to be called from any thread.
  void ShutDown();

  // Sends a PIN request and waits for the response being received.
  //
  // Returns whether the PIN dialog finished successfully, and, if yes, returns
  // the PIN entered by user through the pin output argument.
  //
  // Note that this function must not be called from the main thread, because
  // otherwise it would block receiving of the incoming messages and,
  // consequently, it would lock forever. (Actually, the validity of the current
  // thread is asserted inside.)
  bool RequestPin(std::string* pin);

 private:
  // Requester that is used for sending the requests and waiting for their
  // responses.
  google_smart_card::JsRequester js_requester_;
};

}  // namespace smart_card_client

#endif  // SMART_CARD_CLIENT_BUILT_IN_PIN_DIALOG_BUILT_IN_PIN_DIALOG_SERVER_H_
