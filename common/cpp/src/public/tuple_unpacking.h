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

#ifndef GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_TUPLE_UNPACKING_H_
#define GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_TUPLE_UNPACKING_H_

#include <stddef.h>

namespace google_smart_card {

// Helper template class that holds a compile-time integer sequence as its
// template parameter pack.
template <size_t... Sequence>
struct ArgIndexes {};

namespace internal {

template <size_t ArgCounter, size_t... Sequence>
struct ArgIndexesGenerator
    : ArgIndexesGenerator<ArgCounter - 1, ArgCounter - 1, Sequence...> {};

template <size_t... Sequence>
struct ArgIndexesGenerator<0, Sequence...> {
  using result = ArgIndexes<Sequence...>;
};

}  // namespace internal

// Generate compile-time sequence of argument indexes that can be used for the
// extraction of std::tuple elements.
//
// In essence, MakeArgIndexes<X> is ArgIndexes<0, 1, 2, ..., X - 1>.
template <size_t ArgCount>
using MakeArgIndexes = typename internal::ArgIndexesGenerator<ArgCount>::result;

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_TUPLE_UNPACKING_H_
