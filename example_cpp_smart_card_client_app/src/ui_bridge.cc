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

#include "ui_bridge.h"

#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/cpp/src/public/value_debug_dumping.h"

namespace gsc = google_smart_card;

namespace smart_card_client {

namespace {

constexpr char kIncomingMessageType[] = "ui_backend";
constexpr char kOutgoingMessageType[] = "ui";

void ProcessMessageFromUi(
    gsc::Value data,
    std::weak_ptr<MessageFromUiHandler> message_from_ui_handler,
    std::shared_ptr<std::mutex> request_handling_mutex) {
  std::unique_lock<std::mutex> lock;
  if (request_handling_mutex)
    lock = std::unique_lock<std::mutex>(*request_handling_mutex);

  GOOGLE_SMART_CARD_LOG_DEBUG << "Processing message from UI: "
                              << gsc::DebugDumpValueSanitized(data);
  std::shared_ptr<MessageFromUiHandler> locked_handler =
      message_from_ui_handler.lock();
  if (!locked_handler) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "Ignoring message from UI: module shut down";
    return;
  }
  locked_handler->HandleMessageFromUi(std::move(data));
}

}  // namespace

UiBridge::UiBridge(gsc::GlobalContext* global_context,
                   gsc::TypedMessageRouter* typed_message_router,
                   std::shared_ptr<std::mutex> request_handling_mutex)
    : global_context_(global_context),
      typed_message_router_(typed_message_router),
      request_handling_mutex_(request_handling_mutex) {
  GOOGLE_SMART_CARD_CHECK(global_context_);
  GOOGLE_SMART_CARD_CHECK(typed_message_router_);
  typed_message_router->AddRoute(this);
}

UiBridge::~UiBridge() {
  ShutDown();
}

void UiBridge::ShutDown() {
  gsc::TypedMessageRouter* const typed_message_router =
      typed_message_router_.exchange(nullptr);
  if (typed_message_router)
    typed_message_router->RemoveRoute(this);
}

void UiBridge::SetHandler(std::weak_ptr<MessageFromUiHandler> handler) {
  message_from_ui_handler_ = handler;
}

void UiBridge::RemoveHandler() {
  message_from_ui_handler_.reset();
}

void UiBridge::SendMessageToUi(gsc::Value message) {
  gsc::TypedMessage typed_message;
  typed_message.type = kOutgoingMessageType;
  typed_message.data = std::move(message);
  gsc::Value typed_message_value =
      gsc::ConvertToValueOrDie(std::move(typed_message));
  global_context_->PostMessageToJs(std::move(typed_message_value));
}

std::string UiBridge::GetListenedMessageType() const {
  return kIncomingMessageType;
}

bool UiBridge::OnTypedMessageReceived(gsc::Value data) {
  std::thread(&ProcessMessageFromUi, std::move(data), message_from_ui_handler_,
              request_handling_mutex_)
      .detach();
  return true;
}

}  // namespace smart_card_client
