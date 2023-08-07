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

#ifndef GOOGLE_SMART_CARD_INTEGRATION_TESTING_INTEGRATION_TEST_HELPER_H_
#define GOOGLE_SMART_CARD_INTEGRATION_TESTING_INTEGRATION_TEST_HELPER_H_

#include <functional>
#include <string>

#include "common/cpp/src/google_smart_card_common/global_context.h"
#include "common/cpp/src/google_smart_card_common/messaging/typed_message_router.h"
#include "common/cpp/src/google_smart_card_common/requesting/request_receiver.h"
#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

// Abstract interface of a C/C++ helper used for a JavaScript-and-C++
// integration test.
//
// Intended usage scenario:
// 1. Write the main test scenario in a JavaScript file.
// 2. The test's needed C/C++ counterpart is implemented as a subclass of this
//    interface.
// 3. In the helper's .cc file, a global object is created that constructs and
//    registers an instance of this helper:
//
//      const auto g_foo_helper = IntegrationTestService::RegisterHelper(
//          MakeUnique<FooHelper>());
//
// Note: We're explicitly violating the Google C++ Style Guide by using global
// objects with nontrivial constructors; we're doing it because it allows to
// avoid maintaining a source file that would enumerate all helpers. A similar
// trick is actually used by Googletest internally.
class IntegrationTestHelper {
 public:
  virtual ~IntegrationTestHelper();

  virtual std::string GetName() const = 0;
  virtual void SetUp(GlobalContext* global_context,
                     TypedMessageRouter* typed_message_router,
                     Value data,
                     RequestReceiver::ResultCallback result_callback);
  virtual void TearDown(std::function<void()> completion_callback);
  virtual void OnMessageFromJs(
      Value data,
      RequestReceiver::ResultCallback result_callback) = 0;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_INTEGRATION_TESTING_INTEGRATION_TEST_HELPER_H_
