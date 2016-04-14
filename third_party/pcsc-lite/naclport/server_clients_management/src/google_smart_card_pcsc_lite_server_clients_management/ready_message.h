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

// This file contains functions for creating "ready" messages intended for
// notifying the remote side when the initialization is complete.
//
// See common/messaging/typed_message.h file for the description of notion of
// "message type" and "message data".

#ifndef GOOGLE_SMART_CARD_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_READY_MESSAGE_H_
#define GOOGLE_SMART_CARD_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_READY_MESSAGE_H_

#include <string>

#include <ppapi/cpp/var.h>

namespace google_smart_card {

// Returns the message type for the "PC/SC-Lite ready" message.
std::string GetPcscLiteServerReadyMessageType();

// Constructs the "PC/SC-Lite ready" message data (it's actually an empty
// dictionary, but the function is here for the uniformity purpose).
pp::Var MakePcscLiteServerReadyMessageData();

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_READY_MESSAGE_H_
