// Copyright 2016 Google Inc. All Rights Reserved.
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

// This file contains implementation of the NaCl module entry point
// pp::CreateModule, which creates an instance of class inherited from the
// pp::Instance class (see
// <https://developer.chrome.com/native-client/devguide/coding/application-structure#native-client-modules-a-closer-look>
// for reference).

#include <string>
#include <utility>

#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/global_context_impl_nacl.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

#include "application.h"

namespace scc = smart_card_client;
namespace gsc = google_smart_card;

namespace smart_card_client {

namespace {

// This class contains the actual NaCl module implementation.
//
// The implementation presented here is a wrapper around the `Application` class
// that is a toolchain-independent skeleton (it can be compiled under both
// Emscripten/WebAssembly and Native Client).
class PpInstance final : public pp::Instance {
 public:
  // The constructor that is executed during the NaCl module startup.
  //
  // The implementation presented here does the following:
  // * creates a `google_smart_card::GlobalContextImplNacl` class instance that
  //   is used by the Application class to perform operations that has to be
  //   done differently for Emscripten/WebAssembly and Native Client builds;
  // * creates a `google_smart_card::TypedMessageRouter` class instance that is
  //   used for handling messages received from the JavaScript side (see the
  //   `HandleMessage()` method);
  // * creates an `Application` class instance that contains the core
  //   functionality that is toolchain-independent (i.e., works in the same way
  //   under both Emscripten/WebAssembly and Native Client).
  explicit PpInstance(PP_Instance instance)
      : pp::Instance(instance),
        global_context_(
            new gsc::GlobalContextImplNacl(pp::Module::Get()->core(), this)),
        application_(global_context_.get(), &typed_message_router_) {}

  // The destructor that is executed when the NaCl framework is about to destroy
  // the NaCl module (though, actually, it's not guaranteed to be executed at
  // all - the NaCl module can be just shut down by the browser).
  ~PpInstance() override {
    // Intentionally leak `global_context_` without destroying it, because there
    // might still be background threads that access it.
    global_context_->DisableJsCommunication();
    global_context_.release();
  }

  // This method is called with each message received by the NaCl module from
  // the JavaScript side.
  //
  // All the messages are processed through the
  // `google_smart_card::TypedMessageRouter` class instance, which routes them
  // to the objects that subscribed for receiving them. The routing is based on
  // the "type" key of the message (for the description of the typed messages,
  // see header
  // common/cpp/src/google_smart_card_common/messaging/typed_message.h).
  //
  // See the `Application` class for the routes added into the router.
  void HandleMessage(const pp::Var& message) override {
    std::string error_message;
    gsc::optional<gsc::Value> message_value =
        gsc::ConvertPpVarToValue(message, &error_message);
    if (!message_value) {
      GOOGLE_SMART_CARD_LOG_FATAL
          << "Unexpected JS message received - cannot parse: " << error_message;
    }
    if (!typed_message_router_.OnMessageReceived(std::move(*message_value),
                                                 &error_message)) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Failure while handling JS message: "
                                  << error_message;
    }
  }

 private:
  // Global context that proxies webport-specific operations.
  //
  // The stored pointer is leaked intentionally in the class destructor - see
  // its comment for the justification.
  std::unique_ptr<gsc::GlobalContextImplNacl> global_context_;
  // Router of the incoming typed messages that passes incoming messages to the
  // appropriate handlers according the the special type field of the message
  // (see common/cpp/src/google_smart_card_common/messaging/typed_message.h).
  gsc::TypedMessageRouter typed_message_router_;
  // The core application functionality that is toolchain-independent.
  Application application_;
};

// This class represents the NaCl module for the NaCl framework.
//
// Note that potentially the NaCl framework can request creating multiple
// pp::Instance objects through this module object, though, actually, this never
// happens with the current NaCl framework (and with no exact plans to change it
// in the future: see <http://crbug.com/385783>).
class PpModule final : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) override {
    return new PpInstance(instance);
  }
};

}  // namespace

}  // namespace smart_card_client

namespace pp {

// Entry point of the NaCl module, that is called by the NaCl framework when the
// module is being loaded.
Module* CreateModule() {
  return new scc::PpModule;
}

}  // namespace pp
