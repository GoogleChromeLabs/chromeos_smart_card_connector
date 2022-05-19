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

#ifndef GOOGLE_SMART_CARD_PCSC_LITE_SERVER_GLOBAL_H_
#define GOOGLE_SMART_CARD_PCSC_LITE_SERVER_GLOBAL_H_

#include <thread>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// This class runs the functionality of the PC/SC-Lite daemon.
//
// At most one instance of this class can exist at any given moment.
//
// This class is never destroyed, it's left hanging in the air during program
// shutdown (for safety reasons).
//
// Note: All methods except GetInstance are thread safe. Calls to GetInstance
//       concurrent to class construction or destruction are not thread safe.
class PcscLiteServerGlobal final {
 public:
  explicit PcscLiteServerGlobal(GlobalContext* global_context);
  PcscLiteServerGlobal(const PcscLiteServerGlobal&) = delete;
  PcscLiteServerGlobal& operator=(const PcscLiteServerGlobal&) = delete;
  ~PcscLiteServerGlobal();

  static const PcscLiteServerGlobal* GetInstance();

  // Performs all necessary PC/SC-Lite daemon initialization steps and starts
  // the daemon.
  //
  // The daemon thread never finishes. Therefore, it's not allowed to call this
  // function twice in a single process.
  //
  // Note that it is assumed that the libusb webport library and, in case of
  // compiling under Native Client, the nacl_io library have already been
  // initialized.
  void InitializeAndRunDaemonThread();

  // Shuts down the daemon thread; waits for the shutdown completion in a
  // blocking way.
  //
  // Must be called after `InitializeAndRunDaemonThread()`.
  void ShutDownAndWait();

  void PostReaderInitAddMessage(const char* reader_name,
                                int port,
                                const char* device) const;
  void PostReaderFinishAddMessage(const char* reader_name,
                                  int port,
                                  const char* device,
                                  long return_code) const;
  void PostReaderRemoveMessage(const char* reader_name, int port) const;

 private:
  void PostMessage(const char* type, Value message_data) const;

  GlobalContext* const global_context_;
  std::thread daemon_thread_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_PCSC_LITE_SERVER_GLOBAL_H_
