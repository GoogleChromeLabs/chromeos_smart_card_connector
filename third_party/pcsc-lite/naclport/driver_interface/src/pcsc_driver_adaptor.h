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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_DRIVER_INTERFACE_PCSC_DRIVER_ADAPTOR_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_DRIVER_INTERFACE_PCSC_DRIVER_ADAPTOR_H_

#include <string>
#include <vector>

namespace google_smart_card {

// Represents a driver for smart card readers, to be plugged into our webport of
// PC/SC-Lite.
//
// This replaces the shared library loading mechanism that's used by PC/SC-Lite
// on *nix systems. In our webport, the drivers are all linked statically
// together with the PC/SC-Lite.
class PcscDriverAdaptor {
 public:
  // Represents the driver's exported function.
  struct FunctionNameAndAddress {
    std::string name;
    void* address;
  };

  virtual ~PcscDriverAdaptor() = default;

  // Returns the path to the driver .so file.
  //
  // This is expected to exactly match the string that PC/SC-Lite constructs,
  // based on the Info.plist config file location and the "CFBundleExecutable"
  // value in it.
  virtual const std::string& GetDriverFilePath() const = 0;

  // Returns to the driver's exported functions table.
  virtual const std::vector<FunctionNameAndAddress>& GetFunctionPointersTable()
      const = 0;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_DRIVER_INTERFACE_PCSC_DRIVER_ADAPTOR_H_
