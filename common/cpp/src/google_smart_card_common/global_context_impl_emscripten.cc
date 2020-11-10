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

#include <google_smart_card_common/global_context_impl_emscripten.h>

#include <mutex>
#include <thread>

#include <emscripten/val.h>

#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_emscripten_val_conversion.h>

namespace google_smart_card {

GlobalContextImplEmscripten::GlobalContextImplEmscripten(
    std::thread::id main_thread_id,
    emscripten::val post_message_callback)
    : main_thread_id_(main_thread_id),
      post_message_callback_(post_message_callback) {}

GlobalContextImplEmscripten::~GlobalContextImplEmscripten() = default;

void GlobalContextImplEmscripten::PostMessageToJs(Value message) {
  // Converting the value before entering the mutex, in order to minimize the
  // time spent under the lock.
  const emscripten::val val = ConvertValueToEmscriptenVal(message);

  const std::unique_lock<std::mutex> lock(mutex_);
  if (!post_message_callback_.isUndefined())
    post_message_callback_(val);
}

bool GlobalContextImplEmscripten::IsMainEventLoopThread() const {
  return std::this_thread::get_id() == main_thread_id_;
}

void GlobalContextImplEmscripten::DisableJsCommunication() {
  const std::unique_lock<std::mutex> lock(mutex_);
  post_message_callback_ = emscripten::val::undefined();
}

}  // namespace google_smart_card
