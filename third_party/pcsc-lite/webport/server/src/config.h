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

// This file is a manually built replacement for the Autotools-generated config
// file.

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CONFIG_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CONFIG_H_

// Enable GNU-specific extensions of Standard Library, as they're called by
// PC/SC-Lite's original code. This define must precede any include of a
// standard header, which is why we have to replicate it here (the original
// PC/SC-Lite's config.h doesn't include standard headers).
// In some build configurations this macro is already predefined, hence
// wrap this code into an ifndef.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CONFIG_H_
