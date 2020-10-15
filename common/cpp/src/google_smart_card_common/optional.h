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

#ifndef GOOGLE_SMART_CARD_COMMON_OPTIONAL_H_
#define GOOGLE_SMART_CARD_COMMON_OPTIONAL_H_

#include <memory>
#include <type_traits>
#include <utility>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

//
// This class is a replacement of the std::optional as it's not standardized yet
// (it'll probably be included into the C++17 standard only) and not supported
// by the PNaCl toolchain compilers.
//
// Note: The implementation is imperfect in terms of excessive allocations and
// deallocations (first of all, a dynamic allocation is always required for
// storing a value; moreover, operator=, when both sides have a contained
// value, destroys the left side's storage first and then, if necessary,
// allocates a new storage; there are several other similar places where
// allocations could be elided). But the current implementation's performance
// seems to be enough for our purposes.
//
template <typename T>
class optional final {
 public:
  optional() = default;

  optional(const optional& other)  // NOLINT
      : storage_(other ? new T(*other) : nullptr) {}

  optional(optional&& other) = default;  // NOLINT

  optional(const T& value)  // NOLINT
      : storage_(new T(value)) {}

  optional(T&& value)  // NOLINT
      : storage_(new T(std::move(value))) {}

  optional& operator=(const optional& other) { return *this = optional(other); }

  optional& operator=(optional&& other) = default;

  const T* operator->() const {
    GOOGLE_SMART_CARD_CHECK(storage_);
    return storage_.get();
  }

  T* operator->() {
    GOOGLE_SMART_CARD_CHECK(storage_);
    return storage_.get();
  }

  const T& operator*() const& { return *(operator->()); }

  T& operator*() & { return *(operator->()); }

  T&& operator*() && { return std::move(operator*()); }

  explicit operator bool() const { return !!storage_; }

  T& value() & { return operator*(); }

  const T& value() const& { return operator*(); }

  T&& value() && { return operator*(); }

  bool operator<(const optional& other) const {
    if (!*this || !other) return !*this && other;
    return value() < other.value();
  }

  bool operator>(const optional& other) const { return other < *this; }

  bool operator==(const optional& other) const {
    if (!*this || !other) return !*this == !other;
    return value() == other.value();
  }

 private:
  std::unique_ptr<T> storage_;
};

// Creates an optional object from the passed value.
template <typename T>
inline optional<typename std::decay<T>::type> make_optional(T&& value) {
  return optional<typename std::decay<T>::type>(std::forward<T>(value));
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_OPTIONAL_H_
