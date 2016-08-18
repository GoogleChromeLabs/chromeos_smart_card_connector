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

#include <mutex>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var_dictionary.h>

namespace google_smart_card {

// This class contains a pointer to the pp::Instance which enables it to provide
// some specific Post*Message functionality.
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
  explicit PcscLiteServerGlobal(pp::Instance* pp_instance);
  ~PcscLiteServerGlobal();

  // Detaches from the Pepper instance which prevents making any further
  // requests through it.
  //
  // After this function call, the PcscLiteServerGlobal::PostReader*Message
  // functions are still allowed to be called, but they will do nothing instead
  // of performing the real requests.
  //
  // This function is primarily intended to be used during the Pepper instance
  // shutdown process, for preventing the situations when some other threads
  // currently calling PcscLiteServerGlobal::PostReader*Message functions try to
  // access the destroyed pp::Instance object or some other associated objects.
  //
  // This function is safe to be called from any thread.
  void Detach();

  static const PcscLiteServerGlobal* GetInstance();

  // Performs all necessary PC/SC-Lite NaCl port initialization steps and starts
  // the PC/SC-Lite daemon.
  //
  // The daemon thread never finishes. Therefore, it's not allowed to call this
  // function twice in a single process.
  //
  // Note that it is assumed that nacl_io and libusb NaCl port libraries have
  // already been initialized.
  void InitializeAndRunDaemonThread();

  void PostReaderInitAddMessage(const char* reader_name, int port,
                                const char* device) const;
  void PostReaderFinishAddMessage(const char* reader_name, int port,
                                  const char* device, long return_code) const;
  void PostReaderRemoveMessage(const char* reader_name, int port) const;

 private:
  void PostMessage(
      const char* type, const pp::VarDictionary& message_data) const;

  mutable std::mutex mutex_;
  pp::Instance* pp_instance_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_PCSC_LITE_SERVER_GLOBAL_H_
