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

#ifndef GOOGLE_SMART_CARD_COMMON_GLOBAL_CONTEXT_IMPL_NACL_H_
#define GOOGLE_SMART_CARD_COMMON_GLOBAL_CONTEXT_IMPL_NACL_H_

#ifndef __native_client__
#error "This file should only be used in Native Client builds"
#endif  // __native_client__

#include <mutex>

#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Implementation of the GlobalContext interface for the Native Client
// environment.
class GlobalContextImplNacl final : public GlobalContext {
 public:
  GlobalContextImplNacl(pp::Core* pp_core, pp::Instance* pp_instance);
  GlobalContextImplNacl(const GlobalContextImplNacl&) = delete;
  GlobalContextImplNacl& operator=(const GlobalContextImplNacl&) = delete;
  ~GlobalContextImplNacl() override;

  // GlobalContext:
  void PostMessageToJs(Value message) override;
  bool IsMainEventLoopThread() const override;
  void ShutDown() override;

 private:
  pp::Core* const pp_core_;

  // The mutex that protects access to `pp_instance_`.
  std::mutex mutex_;
  pp::Instance* pp_instance_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_GLOBAL_CONTEXT_IMPL_NACL_H_
