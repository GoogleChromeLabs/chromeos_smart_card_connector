// Copyright 2024 Google Inc. All Rights Reserved.
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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_CCID_CCID_PCSC_DRIVER_ADAPTOR_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_CCID_CCID_PCSC_DRIVER_ADAPTOR_H_

#include <string>
#include <vector>

#include "third_party/pcsc-lite/naclport/driver_interface/src/pcsc_driver_adaptor.h"

namespace google_smart_card {

// Implementation of the adaptor to allow plugging the CCID driver into our
// PC/SC-Lite webport.
class CcidPcscDriverAdaptor : public PcscDriverAdaptor {
 public:
  CcidPcscDriverAdaptor();
  ~CcidPcscDriverAdaptor() override;

  const std::string& GetDriverFilePath() const override;
  const std::vector<FunctionNameAndAddress>& GetFunctionPointersTable()
      const override;

 private:
  const std::string kFilePath_;
  const std::vector<FunctionNameAndAddress> kFunctionPointers_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_CCID_CCID_PCSC_DRIVER_ADAPTOR_H_
