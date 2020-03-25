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

// This file contains definitions that allow handling and printing logs received
// from an external message pipe.

#ifndef GOOGLE_SMART_CARD_COMMON_EXTERNAL_LOGS_PRINTER_H_
#define GOOGLE_SMART_CARD_COMMON_EXTERNAL_LOGS_PRINTER_H_

#include <google_smart_card_common/messaging/typed_message_listener.h>

namespace google_smart_card {

class ExternalLogsPrinter final : public TypedMessageListener {
 public:
  ExternalLogsPrinter(const std::string& listened_message_type);
  ExternalLogsPrinter(const ExternalLogsPrinter&) = delete;
  ExternalLogsPrinter& operator=(const ExternalLogsPrinter&) = delete;
  ~ExternalLogsPrinter() override;

  // TypedMessageListener:
  std::string GetListenedMessageType() const override;
  bool OnTypedMessageReceived(const pp::Var& data) override;

 private:
   const std::string listened_message_type_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_EXTERNAL_LOGS_PRINTER_H_
