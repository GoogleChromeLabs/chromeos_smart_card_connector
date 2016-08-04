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

// TODO: These includes need to come before readerfactory.h otherwise compiler
//       goes nuts. Why is that?
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <google_smart_card_common/pp_var_utils/construction.h>

extern "C" {
#include "readerfactory.h"

LONG RFAddReaderServer(const char *readerNameLong, int port,
    const char *library, const char *device);
LONG RFRemoveReaderServer(const char *readerName, int port);
}

#include <google_smart_card_common/logging/syslog/syslog.h>

using namespace google_smart_card;

const char kTypeMessageKey[] = "type";
const char kDataMessageKey[] = "data";
const char kAddReaderMessageType[] = "reader_add";
const char kRemoveReaderMessageType[] = "reader_remove";
const char kNameMessageKey[] = "readerName";
const char kPortMessageKey[] = "port";
const char kDeviceMessageKey[] = "device";
const char kReturnCodeMessageKey[] = "returnCode";

void post_message(const char* type, const pp::VarDictionary& message_data)
{
  pp::VarDictionary message;
  message.Set(kTypeMessageKey, type);
  message.Set(kDataMessageKey, message_data);

  const pp::Module* const pp_module = pp::Module::Get();
  if (pp_module) {
    const pp::Module::InstanceMap pp_instance_map =
        pp_module->current_instances();
    for (const auto& instance_map_item : pp_instance_map) {
      pp::Instance* const instance = instance_map_item.second;

      if (instance) instance->PostMessage(message);
    }
  }
}

LONG RFAddReader(const char *readerNameLong, int port, const char *library,
    const char *device)
{
  post_message(kAddReaderMessageType, VarDictBuilder()
      .Add(kNameMessageKey, readerNameLong).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Result());

  LONG ret = RFAddReaderServer(readerNameLong, port, library, device);

  // TODO: Testing code, to be removed at a later time (failing reader).
  if (std::string("SCM Microsystems Inc. SCR 3310") == readerNameLong) {
    post_message(kAddReaderMessageType, VarDictBuilder()
        .Add(kNameMessageKey, readerNameLong).Add(kPortMessageKey, port)
        .Add(kDeviceMessageKey, device).Add(kReturnCodeMessageKey, SCARD_E_INVALID_VALUE).Result());
    return ret;
  }

  post_message(kAddReaderMessageType, VarDictBuilder()
      .Add(kNameMessageKey, readerNameLong).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Add(kReturnCodeMessageKey, ret).Result());

  return ret;
}

LONG RFRemoveReader(const char *readerName, int port)
{
  LONG ret = RFRemoveReaderServer(readerName, port);

  if (ret == SCARD_S_SUCCESS) {
    post_message(kRemoveReaderMessageType, VarDictBuilder()
        .Add(kNameMessageKey, readerName).Add(kPortMessageKey, port).Result());
  }

  return ret;
}
