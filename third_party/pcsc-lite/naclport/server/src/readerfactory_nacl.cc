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

#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_pcsc_lite_server/global.h>

extern "C" {
#include "readerfactory.h"

LONG RFAddReaderOriginal(const char *readerNameLong, int port,
    const char *library, const char *device);
LONG RFRemoveReaderOriginal(const char *readerName, int port);
}

namespace {

const char kTypeMessageKey[] = "type";
const char kDataMessageKey[] = "data";
const char kReaderInitAddMessageType[] = "reader_init_add";
const char kReaderFinishAddMessageType[] = "reader_finish_add";
const char kReaderRemoveMessageType[] = "reader_remove";
const char kNameMessageKey[] = "readerName";
const char kPortMessageKey[] = "port";
const char kDeviceMessageKey[] = "device";
const char kReturnCodeMessageKey[] = "returnCode";

void post_message(const char* type, const pp::VarDictionary& message_data)
{
  pp::VarDictionary message;
  message.Set(kTypeMessageKey, type);
  message.Set(kDataMessageKey, message_data);

  const google_smart_card::PPInstanceHolder* pp_instance_holder =
      google_smart_card::GetGlobalPPInstanceHolder();
  pp::Instance* const instance = pp_instance_holder->GetPPInstance();
  instance->PostMessage(message);
}

}  // namespace

LONG RFAddReader(const char *readerNameLong, int port, const char *library,
    const char *device)
{
  post_message(kReaderInitAddMessageType, google_smart_card::VarDictBuilder()
      .Add(kNameMessageKey, readerNameLong).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Result());

  LONG ret = RFAddReaderOriginal(readerNameLong, port, library, device);

  post_message(kReaderFinishAddMessageType, google_smart_card::VarDictBuilder()
      .Add(kNameMessageKey, readerNameLong).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Add(kReturnCodeMessageKey, ret).Result());

  return ret;
}

// TODO(isandrk): This function (hook) is sometimes not called because of how
//     the hook is set up, but that's fine. Write a better explanation.
LONG RFRemoveReader(const char *readerName, int port)
{
  post_message(kReaderRemoveMessageType, google_smart_card::VarDictBuilder()
      .Add(kNameMessageKey, readerName).Add(kPortMessageKey, port).Result());

  return RFRemoveReaderOriginal(readerName, port);
}
