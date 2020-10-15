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

#ifndef GOOGLE_SMART_CARD_COMMON_THREAD_SAFE_UNIQUE_PTR_H_
#define GOOGLE_SMART_CARD_COMMON_THREAD_SAFE_UNIQUE_PTR_H_

#include <memory>
#include <mutex>
#include <utility>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

// Thread-safe analog of std::unique_ptr.
//
// The interface of this class is quite narrow, as the main task it solves is
// allowing safe simultaneous operations of these two kinds:
// * Operate with the value stored under the pointer;
// * Destroy the value and clear the pointer.
//
// A typical usage example:
//    ThreadSafeUniquePtr<Foo> ptr(new Foo);
//    ...
//    // one thread:
//    const ThreadSafeUniquePtr<Foo>::Locked locked_ptr = ptr.Lock();
//    if (locked_ptr) {
//      locked_ptr->DoSomething();
//      locked_ptr->DoSomethingElse();
//    }
//    ...
//    // another thread:
//    ptr.Reset();
//
// Note: this class has a bit different semantics than the couple of
// std::weak_ptr and std::shared_ptr do: the reset operation of this class
// blocks until all clients that locked the value finally unlock it; while in
// case of std::weak_ptr+std::shared_ptr the clients themselves may prolong the
// lifetime of the stored object.
//
// Note: the implementation is a bit sub-optimal as it doesn't allow
// simultaneous read-only locking from different thread. So the locking scope
// should be limited to be as narrow as possible. The better implementation of
// this class is possible (based on some form of readers-writers lock), but with
// the current use-cases in the codebase that is not necessary.
template <typename T>
class ThreadSafeUniquePtr final {
 public:
  class Locked final {
   public:
    Locked(Locked&&) = default;

    Locked(const Locked&) = delete;

    Locked& operator=(Locked&&) = default;

    Locked& operator=(const Locked&) = delete;

    T& operator*() const {
      GOOGLE_SMART_CARD_CHECK(object_);
      return *object_;
    }

    T* operator->() const {
      GOOGLE_SMART_CARD_CHECK(object_);
      return object_;
    }

    explicit operator bool() const { return object_; }

   private:
    friend class ThreadSafeUniquePtr;

    Locked(T* object, std::unique_lock<std::mutex> lock)
        : object_(object), lock_(std::move(lock)) {
      GOOGLE_SMART_CARD_CHECK(lock_);
    }

    T* const object_;
    std::unique_lock<std::mutex> lock_;
  };

  ThreadSafeUniquePtr() = default;
  ThreadSafeUniquePtr(const ThreadSafeUniquePtr&) = delete;
  ThreadSafeUniquePtr& operator=(const ThreadSafeUniquePtr&) = delete;

  explicit ThreadSafeUniquePtr(std::unique_ptr<T> value)
      : object_(std::move(value)) {}

  ~ThreadSafeUniquePtr() = default;

  void Reset() {
    const std::unique_lock<std::mutex> lock(mutex_);
    object_.reset();
  }

  Locked Lock() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return {object_.get(), std::move(lock)};
  }

  explicit operator bool() const {
    const std::unique_lock<std::mutex> lock(mutex_);
    return object_.get();
  }

 private:
  mutable std::mutex mutex_;
  std::unique_ptr<T> object_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_THREAD_SAFE_UNIQUE_PTR_H_
