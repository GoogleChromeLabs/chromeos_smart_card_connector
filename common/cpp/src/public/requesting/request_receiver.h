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

#ifndef GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_REQUESTING_REQUEST_RECEIVER_H_
#define GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_REQUESTING_REQUEST_RECEIVER_H_

#include <functional>
#include <memory>
#include <string>

#include "common/cpp/src/public/requesting/request_id.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/value.h"

namespace google_smart_card {

class RequestHandler;

// The abstract class for a receiver of incoming requests.
//
// This class is responsible for:
// 1. Receiving of incoming requests from some communication channel (this part
//    has to be implemented by subclasses and linked to the HandleRequest
//    method).
// 2. Delegating the request execution to the associated handler, with providing
//    a callback that should be called with the request result (see the
//    HandleRequest method and the ResultCallback definition).
// 3. Sending the request result back to the sender, using the same
//    communication channel (see the PostResult method).
//
// This class has a shared_ptr-managed lifetime. That allows to implement the
// callbacks passed to request handler in such way that they can be safely
// called after the request receiver is destroyed (in which case the request
// result is just thrown away). FIXME(emaxx): Drop this requirement using the
// WeakPtrFactory concept (inspired by the Chromium source code).
class RequestReceiver : public std::enable_shared_from_this<RequestReceiver> {
 public:
  // Callback that sends the request result.
  //
  // Note that this callback is allowed to be called from any thread and at any
  // moment of time - even after the corresponding RequestReceiver object has
  // been destroyed.
  using ResultCallback = std::function<void(GenericRequestResult)>;

  // Creates the request receiver with the specified name and request handler.
  //
  // The name allows to handle only those requests that were sent from the
  // appropriate requester (see e.g. the
  // google_smart_card_common/requesting/requester.h file), and also to send the
  // request result back to the requester. So, generally, the request receiver
  // names have to be unique.
  RequestReceiver(const std::string& name, RequestHandler* handler);

  RequestReceiver(const RequestReceiver&) = delete;
  RequestReceiver& operator=(const RequestReceiver&) = delete;

  virtual ~RequestReceiver();

  std::string name() const;

 protected:
  // Posts the request result back to the request sender.
  //
  // The implementation must be thread-safe.
  virtual void PostResult(RequestId request_id,
                          GenericRequestResult request_result) = 0;

  // Runs the associated request handler with the specified request and with
  // created ResultCallback functor.
  void HandleRequest(RequestId request_id, Value payload);

 private:
  class ResultCallbackImpl final {
   public:
    ResultCallbackImpl(RequestId request_id,
                       std::weak_ptr<RequestReceiver> request_receiver);
    ResultCallbackImpl(const ResultCallbackImpl&) = default;
    ResultCallbackImpl& operator=(const ResultCallbackImpl&) = delete;
    ~ResultCallbackImpl();

    void operator()(GenericRequestResult request_result);

   private:
    RequestId request_id_;
    const std::weak_ptr<RequestReceiver> request_receiver_;
  };

  ResultCallback MakeResultCallback(RequestId request_id);

  const std::string name_;
  RequestHandler* const handler_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_REQUESTING_REQUEST_RECEIVER_H_
