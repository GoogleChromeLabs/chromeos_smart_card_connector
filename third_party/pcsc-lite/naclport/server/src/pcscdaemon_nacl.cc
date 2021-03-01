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

// This file is a replacement for the original pcscdaemon.c PC/SC-Lite internal
// implementation file.
//
// The original file was responsible for the daemon startup (handled by the main
// function), command line configuration parameters parsing and starting the
// daemon main run loop.
//
// In this NaCl port the responsibilities are split in a different way:
// 1. The daemon main function is not existing in this NaCl port;
// 2. Command-line parameters are not parsed, but the corresponding
//    configuration variables are stubbed with reasonable default values;
// 3. The daemon main run loop is provided by a separate function defined in
//    google_smart_card_pcsc_lite_server/global.h.
//
// This file is responsible for the part 2.

// Auto-exit-after-inactivity feature is disabled.
char AutoExit = 0;

// Adding device serial numbers into reader names is enabled.
char Add_Serial_In_Name = 1;

// Adding interface numbers into reader names is disabled.
char Add_Interface_In_Name = 0;

// Don't force periodic readers polling (unless any of the loaded reader drivers
// requires the polling).
int HPForceReaderPolling = 0;
