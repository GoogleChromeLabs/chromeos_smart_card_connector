// Copyright 2021 Google Inc.
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

#ifndef GOOGLE_SMART_CARD_COMMON_CPP_ATTRIBUTES_H_
#define GOOGLE_SMART_CARD_COMMON_CPP_ATTRIBUTES_H_

// Use this to annotate function declarations that the caller must check the
// returned value.
#if defined(COMPILER_GCC) || defined(__clang__)
#define GOOGLE_SMART_CARD_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define GOOGLE_SMART_CARD_WARN_UNUSED_RESULT
#endif

#endif  // GOOGLE_SMART_CARD_COMMON_CPP_ATTRIBUTES_H_
