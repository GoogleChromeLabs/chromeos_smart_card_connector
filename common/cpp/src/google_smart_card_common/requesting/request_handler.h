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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_HANDLER_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_HANDLER_H_

#include <google_smart_card_common/requesting/request_receiver.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// The abstract class for a handler of incoming requests.
//
// The handler is the component that is actually executing the requests.
//
// In order to receive the requests, the handler has to be associated with one
// or multiple request receivers (see the
// google_smart_card_common/requesting/request_receiver.h file).
class RequestHandler {
 public:
  virtual ~RequestHandler() = default;

  // Handles the request with the specified payload, and with the specified
  // callback that can be used to send the request results back.
  //
  // Note that, generally speaking, this function should not block for a very
  // long period of time (and probably not do any waiting on the incoming
  // Pepper messages at all), as the request receiver calls this method
  // synchronously and, depending on the type of channel it uses, may lead to
  // freezes and deadlocks.
  virtual void HandleRequest(
      Value payload, RequestReceiver::ResultCallback result_callback) = 0;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_HANDLER_H_
