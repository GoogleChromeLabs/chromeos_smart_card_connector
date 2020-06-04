// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/utility/completion_callback_factory.h"


class TestModuleInstance : public pp::Instance {
 public:
  explicit TestModuleInstance(PP_Instance instance)
      : pp::Instance(instance), callback_factory_(this) {}

 private:
  virtual void HandleMessage(const pp::Var& var_message) {
    std::string msg = var_message.AsString();
    if (msg == "exit") {
      exit(0);
    } else if (msg == "ping") {
      pp::Module::Get()->core()->CallOnMainThread(
          0, callback_factory_.NewCallback(&TestModuleInstance::Pong), 0);
    } else if (msg == "fault") {
      __builtin_trap();
    }
  }

  void Pong(int32_t) {
    PostMessage("pong");
  }

 private:
  pp::CompletionCallbackFactory<TestModuleInstance> callback_factory_;
};

class TestModuleModule : public pp::Module {
 public:
  TestModuleModule() : pp::Module() {}
  virtual ~TestModuleModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new TestModuleInstance(instance);
  }
};

namespace pp {
Module* CreateModule() { return new TestModuleModule(); }
}  // namespace pp
