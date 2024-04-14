// Copyright 2016 Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file is a replacement for the original dyn_*.c PC/SC-Lite internal
// implementation files.
//
// The original dyn_*.c files were responsible for the dynamic loading of the
// reader drivers.
//
// In this NaCl port, the only driver (CCID library, see the /third_party/ccid
// directory) is linked statically with the PC/SC-Lite server, so this file
// provides just a stubs that just pretend that the driver shared library is
// loaded and return pointers to the CCID driver functions instead of searching
// them in the shared library export table.

#include <cstring>
#include <string>

extern "C" {
#include "config.h"
#include "pcsclite.h"
#include "dyn_generic.h"
#include "ifdhandler.h"
#include "misc.h"

// "misc.h" defines a "min" macro which is incompatible with C++.
#undef min
}

#include "common/cpp/src/public/logging/logging.h"

namespace {

// This is a fake value that we supply instead of the dynamically loaded library
// handles which are used by pcsc-lite normally.
constexpr char kFakeLHandle[] = "fake_pcsclite_lhandle";

struct FunctionNameAndAddress {
  const char* name;
  void* address;
};

// Fake export table of the driver functions (the function pointers here point
// directly to the statically linked driver functions).
const FunctionNameAndAddress kDriverIfdhandlerFunctions[] = {
    {"IFDHCreateChannelByName",
     reinterpret_cast<void*>(&IFDHCreateChannelByName)},
    {"IFDHCreateChannel", reinterpret_cast<void*>(&IFDHCreateChannel)},
    {"IFDHCloseChannel", reinterpret_cast<void*>(&IFDHCloseChannel)},
    {"IFDHGetCapabilities", reinterpret_cast<void*>(&IFDHGetCapabilities)},
    {"IFDHSetCapabilities", reinterpret_cast<void*>(&IFDHSetCapabilities)},
    {"IFDHSetProtocolParameters",
     reinterpret_cast<void*>(&IFDHSetProtocolParameters)},
    {"IFDHPowerICC", reinterpret_cast<void*>(&IFDHPowerICC)},
    {"IFDHTransmitToICC", reinterpret_cast<void*>(&IFDHTransmitToICC)},
    {"IFDHControl", reinterpret_cast<void*>(&IFDHControl)},
    {"IFDHICCPresence", reinterpret_cast<void*>(&IFDHICCPresence)},
};

}  // namespace

INTERNAL void* DYN_LoadLibrary(const char* pcLibrary) {
  return static_cast<void*>(const_cast<char*>(kFakeLHandle));
}

INTERNAL LONG DYN_CloseLibrary(void* pvLHandle) {
  GOOGLE_SMART_CARD_CHECK(pvLHandle == kFakeLHandle);
  return SCARD_S_SUCCESS;
}

INTERNAL LONG DYN_GetAddress(void* pvLHandle,
                             void** pvFHandle,
                             const char* pcFunction,
                             bool /*mayfail*/) {
  GOOGLE_SMART_CARD_CHECK(pvLHandle == kFakeLHandle);

  std::string function = pcFunction;
  for (const FunctionNameAndAddress& driver_ifdhandler_function :
       kDriverIfdhandlerFunctions) {
    if (function == driver_ifdhandler_function.name) {
      *pvFHandle = driver_ifdhandler_function.address;
      return SCARD_S_SUCCESS;
    }
  }
  *pvFHandle = nullptr;
  return SCARD_F_UNKNOWN_ERROR;
}
