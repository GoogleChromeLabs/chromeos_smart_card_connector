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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_ID_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_ID_H_

#include <stdint.h>

namespace google_smart_card {

// Request identifier type.
//
// Request identifiers are used when serializing requests into messages and for
// tracking of the sent requests.
using RequestId = int64_t;

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUEST_ID_H_
