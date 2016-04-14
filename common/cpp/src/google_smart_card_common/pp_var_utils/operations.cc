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

#include <google_smart_card_common/pp_var_utils/operations.h>

namespace google_smart_card {

pp::VarArray SliceVarArray(
    const pp::VarArray& var, uint32_t begin_index, uint32_t count) {
  GOOGLE_SMART_CARD_CHECK(begin_index + count <= var.GetLength());
  pp::VarArray result;
  result.SetLength(count);
  for (uint32_t index = 0; index < count; ++index)
    SetVarArrayItem(&result, index, var.Get(begin_index + index));
  return result;
}

}  // namespace google_smart_card
