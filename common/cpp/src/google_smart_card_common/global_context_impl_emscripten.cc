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

#include <memory>
#include <thread>

#include <emscripten/threading.h>
#include <emscripten/val.h>

#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_emscripten_val_conversion.h>

namespace google_smart_card {

static_assert(
    sizeof(int) >= sizeof(void*),
    "|int| is too small - cannot fit |void*| to pass data across threads");

GlobalContextImplEmscripten::GlobalContextImplEmscripten(
    std::thread::id main_thread_id,
    emscripten::val post_message_callback)
    : main_thread_id_(main_thread_id),
      post_message_callback_(post_message_callback) {}

GlobalContextImplEmscripten::~GlobalContextImplEmscripten() = default;

void GlobalContextImplEmscripten::PostMessageToJs(Value message) {
  // Post a task to the main thread, since all other threads are running in Web
  // Workers that don't have access to DOM, and aren't allowed to execute
  // `post_message_callback_`.
  // Implementation-wise, we have to use the Emscripten's
  // `emscripten_async_run_in_main_runtime_thread()` function, which has a very
  // low-level interface. Some notes:
  // 1. Only a static function is supported, therefore we schedule the
  //    "trampoline" function that redirects to the normal class method.
  // 2. Only very few primitive argument types are supported, therefore we do
  //    reinterpret_cast on all arguments. Pointers are casted to int (the
  //    static_assert above makes sure that the `int` type is of sufficient
  //    size); later, the trampoline function casts them back to pointers.
  // 3. `EM_FUNC_SIG_VII` means "the scheduled function has the void(int, int)
  //    signature".
  // 4. In order to address the case when `this` might get destroyed before the
  //    async job gets executed, we pass an `std::weak_ptr` to it.
  // 5. It's crucial to send `Value`, as opposed to constructing
  //    `emscripten::val` here on the background thread, in order to avoid
  //    internal Emscripten errors:
  //    <https://github.com/emscripten-core/emscripten/issues/12749>.
  using SelfWeakPtr = std::weak_ptr<GlobalContextImplEmscripten>;
  std::unique_ptr<SelfWeakPtr> this_weak_ptr =
      MakeUnique<SelfWeakPtr>(shared_from_this());
  std::unique_ptr<Value> message_ptr = MakeUnique<Value>(std::move(message));
  emscripten_async_run_in_main_runtime_thread(
      EM_FUNC_SIG_VII,
      &GlobalContextImplEmscripten::PostMessageOnMainThreadTrampoline,
      reinterpret_cast<int>(this_weak_ptr.release()),
      reinterpret_cast<int>(message_ptr.release()));
}

bool GlobalContextImplEmscripten::IsMainEventLoopThread() const {
  return std::this_thread::get_id() == main_thread_id_;
}

void GlobalContextImplEmscripten::ShutDown() {
  GOOGLE_SMART_CARD_CHECK(IsMainEventLoopThread());
  post_message_callback_ = emscripten::val::undefined();
}

// static
void GlobalContextImplEmscripten::PostMessageOnMainThreadTrampoline(
    int raw_this_weak_ptr,
    int raw_value_ptr) {
  using SelfWeakPtr = std::weak_ptr<GlobalContextImplEmscripten>;
  using SelfSharedPtr = std::shared_ptr<GlobalContextImplEmscripten>;
  // Note: These `unique_ptr`s must be constructed before any returning from the
  // function, in order to not leak the memory.
  // Correctness of these reinterpret_cast's is discussed in
  // `PostMessageToJs()`.
  std::unique_ptr<SelfWeakPtr> this_weak_ptr(
      reinterpret_cast<SelfWeakPtr*>(raw_this_weak_ptr));
  std::unique_ptr<Value> message(reinterpret_cast<Value*>(raw_value_ptr));

  SelfSharedPtr this_shared_ptr = this_weak_ptr->lock();
  if (!this_shared_ptr) {
    // `this` got already destroyed before the asynchronous job was started.
    return;
  }
  this_shared_ptr->PostMessageOnMainThread(std::move(*message));
}

void GlobalContextImplEmscripten::PostMessageOnMainThread(Value message) {
  GOOGLE_SMART_CARD_CHECK(IsMainEventLoopThread());
  // Note: No mutexes are needed, since this code is guaranteed to run on the
  // main thread.
  if (!post_message_callback_.isUndefined())
    post_message_callback_(ConvertValueToEmscriptenValOrDie(message));
}

}  // namespace google_smart_card
