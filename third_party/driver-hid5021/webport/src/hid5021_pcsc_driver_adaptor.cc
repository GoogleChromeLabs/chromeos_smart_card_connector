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

#include "third_party/driver-hid5021/webport/src/hid5021_pcsc_driver_adaptor.h"

#include <string>
#include <vector>

extern "C" {
#include "ifdhandler.h"
}

namespace google_smart_card {

Hid5021PcscDriverAdaptor::Hid5021PcscDriverAdaptor()
    : kFilePath_(DRIVER5021_SO_INSTALLATION_PATH),
      kFunctionPointers_({
          {"IFDHCloseChannel", reinterpret_cast<void*>(&IFDHCloseChannel)},
          {"IFDHControl", reinterpret_cast<void*>(&IFDHControl)},
          {"IFDHCreateChannel", reinterpret_cast<void*>(&IFDHCreateChannel)},
          {"IFDHGetCapabilities",
           reinterpret_cast<void*>(&IFDHGetCapabilities)},
          {"IFDHICCPresence", reinterpret_cast<void*>(&IFDHICCPresence)},
          {"IFDHPowerICC", reinterpret_cast<void*>(&IFDHPowerICC)},
          {"IFDHSetCapabilities",
           reinterpret_cast<void*>(&IFDHSetCapabilities)},
          {"IFDHSetProtocolParameters",
           reinterpret_cast<void*>(&IFDHSetProtocolParameters)},
          {"IFDHTransmitToICC", reinterpret_cast<void*>(&IFDHTransmitToICC)},
      }) {}

Hid5021PcscDriverAdaptor::~Hid5021PcscDriverAdaptor() = default;

const std::string& Hid5021PcscDriverAdaptor::GetDriverFilePath() const {
  return kFilePath_;
}

const std::vector<const Hid5021PcscDriverAdaptor::FunctionNameAndAddress>&
Hid5021PcscDriverAdaptor::GetFunctionPointersTable() const {
  return kFunctionPointers_;
}

}  // namespace google_smart_card
