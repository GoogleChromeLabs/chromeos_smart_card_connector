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

#ifndef GOOGLE_SMART_CARD_COMMON_GLOBAL_CONTEXT_IMPL_EMSCRIPTEN_H_
#define GOOGLE_SMART_CARD_COMMON_GLOBAL_CONTEXT_IMPL_EMSCRIPTEN_H_

#ifndef __EMSCRIPTEN__
#error "This file should only be used in Emscripten builds"
#endif  // __EMSCRIPTEN__

#include <mutex>
#include <thread>

#include <emscripten/val.h>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Implementation of the GlobalContext interface for the Emscripten
// (WebAssembly) environment.
class GlobalContextImplEmscripten final : public GlobalContext {
 public:
  // `post_message_callback` - JavaScript callback that will be called for
  // posting a message.
  GlobalContextImplEmscripten(std::thread::id main_thread_id,
                              emscripten::val post_message_callback);
  GlobalContextImplEmscripten(const GlobalContextImplEmscripten&) = delete;
  GlobalContextImplEmscripten& operator=(const GlobalContextImplEmscripten&) =
      delete;
  ~GlobalContextImplEmscripten() override;

  // GlobalContext:
  void PostMessageToJs(Value message) override;
  bool IsMainEventLoopThread() const override;
  void DisableJsCommunication() override;

 private:
  const std::thread::id main_thread_id_;

  // The mutex that protects access to `post_message_callback_`.
  std::mutex mutex_;
  emscripten::val post_message_callback_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_GLOBAL_CONTEXT_IMPL_EMSCRIPTEN_H_
