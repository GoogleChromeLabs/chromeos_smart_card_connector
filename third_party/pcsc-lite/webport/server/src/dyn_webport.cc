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
#include "misc.h"
#include "pcsclite.h"
#include "wintypes.h"

// "misc.h" defines a "min" macro which is incompatible with C++.
#undef min
}

#include "common/cpp/src/public/logging/logging.h"
#include "third_party/pcsc-lite/webport/driver_interface/src/pcsc_driver_adaptor.h"
#include "third_party/pcsc-lite/webport/server/src/public/pcsc_lite_server_web_port_service.h"

namespace google_smart_card {

// Stub for the function defined in PC/SC-Lite dyn_generic.h.
//
// Its real implementation loads a shared library with a driver, but our
// implementation here simply identifies the PcscDriverAdaptor object in an
// in-memory data structure (in Smart Card Connector we link against drivers
// statically).
extern "C" INTERNAL void* DYN_LoadLibrary(const char* pcLibrary) {
  return PcscLiteServerWebPortService::GetInstance()->FindDriverByFilePath(
      pcLibrary);
}

// Stub for the function defined in PC/SC-Lite dyn_generic.h.
//
// Its real implementation unloads the shared library that's loaded by
// `DYN_LoadLibrary()`; in Smart Card Connector we don't need to do that (see
// that function's comment above).
extern "C" INTERNAL LONG DYN_CloseLibrary(void* pvLHandle) {
  GOOGLE_SMART_CARD_CHECK(pvLHandle);
  return SCARD_S_SUCCESS;
}

// Stub for the function defined in PC/SC-Lite dyn_generic.h.
//
// Its real implementation returns a pointer for the given function name in the
// given shared library; as we link statically against the driver in Smart Card
// Connector, we only need to traverse a hardcoded map from names to function
// addresses.
extern "C" INTERNAL LONG DYN_GetAddress(void* pvLHandle,
                                        void** pvFHandle,
                                        const char* pcFunction,
                                        bool /*mayfail*/) {
  GOOGLE_SMART_CARD_CHECK(pvLHandle);
  // This cast is correct - see the implementation of `DYN_LoadLibrary()` above.
  const PcscDriverAdaptor* const driver_adaptor =
      reinterpret_cast<const PcscDriverAdaptor*>(pvLHandle);

  for (const PcscDriverAdaptor::FunctionNameAndAddress& pointer :
       driver_adaptor->GetFunctionPointersTable()) {
    if (pointer.name == pcFunction) {
      *pvFHandle = pointer.address;
      return SCARD_S_SUCCESS;
    }
  }
  *pvFHandle = nullptr;
  return SCARD_F_UNKNOWN_ERROR;
}

}  // namespace google_smart_card
