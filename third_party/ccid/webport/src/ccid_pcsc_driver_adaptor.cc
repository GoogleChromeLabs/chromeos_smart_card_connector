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

#include "third_party/ccid/webport/src/ccid_pcsc_driver_adaptor.h"

#include <string>
#include <vector>

extern "C" {
#include "ifdhandler.h"
}

namespace google_smart_card {

namespace {

// Constructed as the concatenation of:
// * path where the Info.plist config is installed at
//   //smart_card_connector_app/build/executable_module/Makefile;
// * the "Linux" string;
// * the file name passed via "--target" to create_Info_plist.pl at
//   ../build/Makefile.
constexpr char kDriverFilePath[] =
    "executable-module-filesystem/pcsc/drivers/ifd-ccid.bundle/Contents/Linux/"
    "libccid.so";

}  // namespace

CcidPcscDriverAdaptor::CcidPcscDriverAdaptor()
    : kFilePath_(kDriverFilePath),
      kFunctionPointers_({
          {"IFDHCloseChannel", reinterpret_cast<void*>(&IFDHCloseChannel)},
          {"IFDHControl", reinterpret_cast<void*>(&IFDHControl)},
          {"IFDHCreateChannel", reinterpret_cast<void*>(&IFDHCreateChannel)},
          {"IFDHCreateChannelByName",
           reinterpret_cast<void*>(&IFDHCreateChannelByName)},
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

CcidPcscDriverAdaptor::~CcidPcscDriverAdaptor() = default;

const std::string& CcidPcscDriverAdaptor::GetDriverFilePath() const {
  return kFilePath_;
}

const std::vector<CcidPcscDriverAdaptor::FunctionNameAndAddress>&
CcidPcscDriverAdaptor::GetFunctionPointersTable() const {
  return kFunctionPointers_;
}

}  // namespace google_smart_card
