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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUESTER_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUESTER_H_

#include <string>

#include <google_smart_card_common/requesting/async_request.h>
#include <google_smart_card_common/requesting/async_requests_storage.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// An abstract requester which is an entity for sending some requests and
// receiving their results.
//
// Note that it's generally safe to keep the AsyncRequest objects returned by
// the methods of this class even after destroying of the requester (however,
// all requests that were still waiting for their responses will be anyway
// marked as failed).
class Requester {
 public:
  // Creates the requester with the specified name.
  //
  // The requester name exists for tagging the sent requests in some way so that
  // the appropriate request handler can be picked on the other end (see e.g.
  // the common/requesting/request_receiver.h file) and that the response can
  // return back to this requester properly. So, generally, the requester names
  // have to be unique.
  explicit Requester(const std::string& name);

  // Destroys the requester.
  //
  // All requests still waiting for the responses are marked as failed
  // immediately.
  virtual ~Requester();

  std::string name() const { return name_; }

  // Detaches the requester, which may prevent it from sending new requests (new
  // requests may immediately finish with the RequestResultStatus::kFailed
  // state) and/or from receiving results of already sent ones.
  //
  // This function is safe to be called from any thread.
  virtual void Detach() {}

  // Starts an asynchronous request with the given payload and the given
  // callback, which will be executed once the request finishes (either
  // successfully or not).
  //
  // Note: the callback may be executed on a different thread than the thread
  // used for sending the request.
  //
  // Note: It's also possible that the callback is executed synchronously during
  // this method call (e.g. when a fatal error occurred that prevents the
  // implementation from starting the asynchronous request).
  GenericAsyncRequest StartAsyncRequest(Value payload,
                                        GenericAsyncRequestCallback callback);

  // Starts an asynchronous request with the given payload and the given
  // callback, which will be executed once the request finishes (either
  // successfully or not).
  //
  // The resulting GenericAsyncRequest object will be assigned into the
  // async_request argument. This allows to provide the consumer with the
  // GenericAsyncRequest object before the callback is executed (that can
  // simplify the consumer's logic in some cases).
  //
  // Note: the callback may be executed on a different thread than the thread
  // used for sending the request.
  //
  // Note: It's also possible that the callback is executed synchronously during
  // this method call (e.g. when a fatal error occurred that prevents the
  // implementation from starting the asynchronous request).
  virtual void StartAsyncRequest(Value payload,
                                 GenericAsyncRequestCallback callback,
                                 GenericAsyncRequest* async_request) = 0;

  // Performs a synchronous request, blocking current thread until the result is
  // received.
  //
  // It's guaranteed that the returned result cannot have the
  // RequestResultStatus::kCanceled state.
  virtual GenericRequestResult PerformSyncRequest(Value payload);

 protected:
  // Creates and stores internally a new asynchronous request state, returning
  // its public proxy object (GenericAsyncRequest object) and its generated
  // request identifier.
  GenericAsyncRequest CreateAsyncRequest(GenericAsyncRequestCallback callback,
                                         RequestId* request_id);

  // Finds the request state by the specified request identifier and sets its
  // result (which, in turn, runs the callback if it has not been executed yet).
  //
  // After calling this function, the request state is not tracked by this
  // requester anymore.
  //
  // Returns whether the request with the specified identifier was found.
  bool SetAsyncRequestResult(RequestId request_id,
                             GenericRequestResult request_result);

 private:
  const std::string name_;
  GenericAsyncRequestsStorage async_requests_storage_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUESTER_H_
