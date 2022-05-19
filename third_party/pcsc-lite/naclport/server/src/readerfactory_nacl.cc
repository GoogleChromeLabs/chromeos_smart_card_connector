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

#include "third_party/pcsc-lite/naclport/server/src/public/pcsc_lite_server_web_port_service.h"

#include <wintypes.h>

extern "C" {
#include "readerfactory.h"
}

extern "C" {
LONG RFAddReaderOriginal(const char* reader_name,
                         int port,
                         const char* library,
                         const char* device);
LONG RFRemoveReaderOriginal(const char* reader_name, int port);
}

LONG RFAddReader(const char* reader_name,
                 int port,
                 const char* library,
                 const char* device) {
  google_smart_card::PcscLiteServerWebPortService::GetInstance()
      ->PostReaderInitAddMessage(reader_name, port, device);

  LONG return_code = RFAddReaderOriginal(reader_name, port, library, device);

  google_smart_card::PcscLiteServerWebPortService::GetInstance()
      ->PostReaderFinishAddMessage(reader_name, port, device, return_code);

  return return_code;
}

// This function is the hook function for the original one. The hook works via
// the #define trick (passed as an argument to the compiler via command line),
// so it actually works when the function is called from outside the file where
// it is defined, but not from inside (readerfactory). Sometimes it may get
// called from the inside, and that call won't be intercepted, but that is fine.
LONG RFRemoveReader(const char* reader_name, int port) {
  google_smart_card::PcscLiteServerWebPortService::GetInstance()
      ->PostReaderRemoveMessage(reader_name, port);

  return RFRemoveReaderOriginal(reader_name, port);
}
