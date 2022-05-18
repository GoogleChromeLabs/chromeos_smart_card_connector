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

#include "global.h"

#include <sys/mount.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_debug_dumping.h>

#include "third_party/libusb/webport/src/public/libusb_web_port_service.h"

#ifdef __native_client__
#include <nacl_io/nacl_io.h>
#include <ppapi/cpp/module.h>
#include <ppapi_simple/ps_main.h>
#endif  // __native_client__

namespace google_smart_card {

namespace {

class FakeGlobalContext : public GlobalContext {
 public:
  explicit FakeGlobalContext(TypedMessageRouter* typed_message_router)
      : typed_message_router_(typed_message_router) {}
  ~FakeGlobalContext() = default;

  void PostMessageToJs(Value message) override {
    puts(DebugDumpValueFull(message).c_str());
    TypedMessage typed_message =
        ConvertFromValueOrDie<TypedMessage>(std::move(message));
    RequestMessageData request_message_data =
        ConvertFromValueOrDie<RequestMessageData>(
            std::move(typed_message.data));

    ResponseMessageData response_message_data;
    response_message_data.request_id = request_message_data.request_id;
    response_message_data.error_message = std::string("Fake error");
    TypedMessage response_typed_message;
    response_typed_message.type = "libusb::response";
    response_typed_message.data = ConvertToValueOrDie(std::move(response_message_data));

    typed_message_router_->OnMessageReceived(ConvertToValueOrDie(std::move(response_typed_message)));
  }
  bool IsMainEventLoopThread() const override { return false; }
  void ShutDown() override {}

 private:
  TypedMessageRouter* const typed_message_router_;
};

}  // namespace

TEST(PcscLiteServerGlobal, SmokeTest) {
  // chdir("out/cpp_unit_test_runner");

#ifdef __native_client__
  GOOGLE_SMART_CARD_CHECK(::umount("/") == 0);
  GOOGLE_SMART_CARD_CHECK(
      ::mount("/", "/", "httpfs", 0, "manifest=/nacl_io_manifest.txt") == 0);
#endif  // __native_client__

  TypedMessageRouter typed_message_router;
  FakeGlobalContext global_context(&typed_message_router);
  LibusbWebPortService libusb_web_port_service(&global_context,
                                               &typed_message_router);

  PcscLiteServerGlobal server(&global_context);
  server.InitializeAndRunDaemonThread();

  // std::this_thread::sleep_for(std::chrono::seconds(1));

  server.StopDaemonThreadAndWait();
}

}  // namespace google_smart_card
