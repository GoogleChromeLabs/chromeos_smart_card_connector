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

// This file contains definitions for describing a result of request.

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_RESULT_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_RESULT_H_

#include <string>
#include <utility>

#include "common/cpp/src/google_smart_card_common/logging/logging.h"
#include "common/cpp/src/google_smart_card_common/optional.h"
#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

// Request status that describes the request outcome, which can be either of the
// following:
// * successfully finished,
// * failed due to some error,
// * canceled by consumer's request.
enum class RequestResultStatus {
  kSucceeded,
  kFailed,
  kCanceled,
};

namespace internal {

extern const char kRequestCanceledErrorMessage[];

}  // namespace internal

// Request result consists of:
// * request result status (RequestResultStatus enum value),
// * error message (only when the status is RequestResultStatus::kFailed or
//   RequestResultStatus::kCanceled),
// * request result payload (only when the status is
//   RequestResultStatus::kSucceeded).
template <typename PayloadType>
class RequestResult final {
 public:
  RequestResult() = default;
  RequestResult(RequestResult&&) = default;
  RequestResult& operator=(RequestResult&&) = default;
  ~RequestResult() = default;

  static RequestResult CreateSuccessful(PayloadType payload) {
    return RequestResult(PayloadType(std::move(payload)));
  }

  static RequestResult CreateFailed(const std::string& error_message) {
    return CreateUnsuccessful(RequestResultStatus::kFailed, error_message);
  }

  static RequestResult CreateCanceled() {
    return CreateUnsuccessful(RequestResultStatus::kCanceled,
                              internal::kRequestCanceledErrorMessage);
  }

  static RequestResult CreateUnsuccessful(RequestResultStatus status,
                                          const std::string& error_message) {
    return RequestResult(status, error_message);
  }

  RequestResultStatus status() const {
    CheckInitialized();
    return *status_;
  }

  bool is_successful() const {
    CheckInitialized();
    return *status_ == RequestResultStatus::kSucceeded;
  }

  std::string error_message() const {
    CheckInitialized();
    GOOGLE_SMART_CARD_CHECK(*status_ == RequestResultStatus::kFailed ||
                            *status_ == RequestResultStatus::kCanceled);
    GOOGLE_SMART_CARD_CHECK(error_message_);
    return *error_message_;
  }

  const PayloadType& payload() const {
    CheckInitialized();
    GOOGLE_SMART_CARD_CHECK(*status_ == RequestResultStatus::kSucceeded);
    GOOGLE_SMART_CARD_CHECK(payload_);
    return *payload_;
  }

  // Extracts and returns the payload.
  // Uses the "&&" ref-qualifier, so that it's only possible to use it like:
  //   std::move(request_result).TakePayload()
  // This form explicitly says that the request result isn't usable after this
  // point; some tools like clang-tidy also catch bugs if the variable will be
  // used again after this point.
  PayloadType TakePayload() && {
    CheckInitialized();
    GOOGLE_SMART_CARD_CHECK(*status_ == RequestResultStatus::kSucceeded);
    GOOGLE_SMART_CARD_CHECK(payload_);
    return std::move(*payload_);
  }

 private:
  explicit RequestResult(PayloadType payload)
      : status_(RequestResultStatus::kSucceeded),
        payload_(std::move(payload)) {}

  RequestResult(RequestResultStatus status, const std::string& error_message)
      : status_(status), error_message_(error_message) {
    GOOGLE_SMART_CARD_CHECK(status != RequestResultStatus::kSucceeded);
  }

  void CheckInitialized() const {
    if (!status_) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Trying to access an uninitialized "
                                  << "request result";
    }
  }

  optional<RequestResultStatus> status_;
  optional<std::string> error_message_;
  optional<PayloadType> payload_;
};

using GenericRequestResult = RequestResult<Value>;

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_RESULT_H_
