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

#include <google_smart_card_common/global_context_impl_nacl.h>

#include <mutex>

#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>

#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

namespace google_smart_card {

GlobalContextImplNacl::GlobalContextImplNacl(pp::Core* pp_core,
                                             pp::Instance* pp_instance)
    : pp_core_(pp_core), pp_instance_(pp_instance) {}

GlobalContextImplNacl::~GlobalContextImplNacl() = default;

void GlobalContextImplNacl::DetachFromPpInstance() {
  const std::unique_lock<std::mutex> lock(mutex_);
  pp_instance_ = nullptr;
}

bool GlobalContextImplNacl::PostMessageToJs(const Value& message) {
  const std::unique_lock<std::mutex> lock(mutex_);
  if (!pp_instance_) return false;
  pp_instance_->PostMessage(ConvertValueToPpVar(message));
  return true;
}

bool GlobalContextImplNacl::IsMainEventLoopThread() const {
  return pp_core_->IsMainThread();
}

}  // namespace google_smart_card
