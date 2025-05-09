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

// This file contains replacement functions for the original readerfactory.c
// PC/SC-Lite internal implementation.

#include "third_party/pcsc-lite/webport/server/src/public/pcsc_lite_server_web_port_service.h"

#include <wintypes.h>

extern "C" {
#include "readerfactory.h"
}

#include "common/cpp/src/public/logging/logging.h"
#include "third_party/pcsc-lite/webport/server/src/public/pcsc_lite_server_web_port_service.h"

// Original PC/SC-Lite's `RFAddReader()` and `RFRemoveReader()` functions. Our
// interceptors below eventually call into these.
extern "C" LONG RFAddReaderOriginal(const char* reader_name,
                                    int port,
                                    const char* library,
                                    const char* device);
extern "C" LONG RFRemoveReaderOriginal(const char* reader_name,
                                       int port,
                                       int flags);

namespace google_smart_card {

// This function is the hook function for the original one. The hook works via
// the #define trick (passed as an argument to the compiler via command line).
extern "C" LONG RFAddReader(const char* reader_name,
                            int port,
                            const char* library,
                            const char* device) {
  // Notify UI about the reader being initialized.
  PcscLiteServerWebPortService::GetInstance()->PostReaderInitAddMessage(
      reader_name, port, device);

  // Call back into the original PC/SC-Lite `RFAddReader()` implementation,
  // which requests the driver to initialize the reader.
  LONG return_code = RFAddReaderOriginal(reader_name, port, library, device);

  // Notify UI about the reader initialization result.
  PcscLiteServerWebPortService::GetInstance()->PostReaderFinishAddMessage(
      reader_name, port, device, return_code);

  if (return_code != SCARD_S_SUCCESS) {
    // In case the reader error is transient, attempt to mitigate it.
    PcscLiteServerWebPortService::GetInstance()->AttemptMitigateReaderError(
        device);
  }

  return return_code;
}

// This function is the hook function for the original one. The hook works via
// the #define trick (passed as an argument to the compiler via command line),
// so it actually works when the function is called from outside the file where
// it is defined, but not from inside (readerfactory). Sometimes it may get
// called from the inside, and that call won't be intercepted, but that is fine.
extern "C" LONG RFRemoveReader(const char* reader_name, int port, int flags) {
  // Notify UI about the reader removal.
  PcscLiteServerWebPortService::GetInstance()->PostReaderRemoveMessage(
      reader_name, port);

  // Call back into the original PC/SC-Lite `RFRemoveReader()` implementation.
  return RFRemoveReaderOriginal(reader_name, port, flags);
}

}  // namespace google_smart_card
