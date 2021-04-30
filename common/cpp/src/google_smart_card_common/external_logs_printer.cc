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

#include <google_smart_card_common/external_logs_printer.h>

#include <iostream>
#include <string>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

namespace {

// The structure represents the message handled by the external logs printer.
struct ExternalLogMessageData {
  std::string formatted_log_message;
};

void PrintExternalLogMessage(const ExternalLogMessageData& message_data) {
  // Note: Intentionally not using the GOOGLE_SMART_CARD_LOG*() macros and
  // related here, in order to avoid delivering them to the JS side and
  // therefore likely duplicating them.
  std::cerr << message_data.formatted_log_message;
  std::cerr.flush();
}

}  // namespace

template <>
StructValueDescriptor<ExternalLogMessageData>::Description
StructValueDescriptor<ExternalLogMessageData>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // //common/js/src/logging/log-buffer-forwarder.js.
  return Describe("ExternalLogMessageData")
      .WithField(&ExternalLogMessageData::formatted_log_message,
                 "formatted_log_message");
}

ExternalLogsPrinter::ExternalLogsPrinter(
    const std::string& listened_message_type)
    : listened_message_type_(listened_message_type) {}

ExternalLogsPrinter::~ExternalLogsPrinter() = default;

std::string ExternalLogsPrinter::GetListenedMessageType() const {
  return listened_message_type_;
}

bool ExternalLogsPrinter::OnTypedMessageReceived(Value data) {
  PrintExternalLogMessage(
      ConvertFromValueOrDie<ExternalLogMessageData>(std::move(data)));
  return true;
}

}  // namespace google_smart_card
