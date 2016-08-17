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

// This file contains a replacement function for the original readerfactory.c
// PC/SC-Lite internal implementation.

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_pcsc_lite_server/global.h>

extern "C" {
#include "readerfactory.h"

LONG RFAddReaderOriginal(const char* reader_name, int port,
    const char* library, const char* device);
LONG RFRemoveReaderOriginal(const char* reader_name, int port);
}

namespace {

const char kReaderInitAddMessageType[] = "reader_init_add";
const char kReaderFinishAddMessageType[] = "reader_finish_add";
const char kReaderRemoveMessageType[] = "reader_remove";
const char kNameMessageKey[] = "readerName";
const char kPortMessageKey[] = "port";
const char kDeviceMessageKey[] = "device";
const char kReturnCodeMessageKey[] = "returnCode";

void post_message(const char* type, const pp::VarDictionary& message_data) {
  const google_smart_card::PPInstanceHolder* pp_instance_holder =
      google_smart_card::GetGlobalPPInstanceHolder();
  pp::Instance* const instance = pp_instance_holder->GetPPInstance();
  instance->PostMessage(
      google_smart_card::MakeTypedMessage(type, message_data));
}

}  // namespace

LONG RFAddReader(const char* reader_name, int port, const char* library,
    const char* device) {
  post_message(kReaderInitAddMessageType, google_smart_card::VarDictBuilder()
      .Add(kNameMessageKey, reader_name).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Result());

  LONG ret = RFAddReaderOriginal(reader_name, port, library, device);

  post_message(kReaderFinishAddMessageType, google_smart_card::VarDictBuilder()
      .Add(kNameMessageKey, reader_name).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Add(kReturnCodeMessageKey, ret).Result());

  return ret;
}

// This function is the hook function for the original one. The hook works via
// the #define trick (passed as an argument to the compiler via command line),
// so it actually works when the function is called from outside the file where
// it is defined, but not from inside (readerfactory). Sometimes it may get
// called from the inside, and that call won't be intercepted, but that is fine.
LONG RFRemoveReader(const char* reader_name, int port) {
  post_message(kReaderRemoveMessageType, google_smart_card::VarDictBuilder()
      .Add(kNameMessageKey, reader_name).Add(kPortMessageKey, port).Result());

  return RFRemoveReaderOriginal(reader_name, port);
}
