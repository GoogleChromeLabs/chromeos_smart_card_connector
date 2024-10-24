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

#ifndef GOOGLE_SMART_CARD_PCSC_LITE_SERVER_WEB_PORT_SERVICE_H_
#define GOOGLE_SMART_CARD_PCSC_LITE_SERVER_WEB_PORT_SERVICE_H_

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/value.h"
#include "third_party/libusb/webport/src/public/libusb_web_port_service.h"
#include "third_party/pcsc-lite/naclport/driver_interface/src/pcsc_driver_adaptor.h"

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
class PcscLiteServerWebPortService final {
 public:
  PcscLiteServerWebPortService(
      GlobalContext* global_context,
      LibusbWebPortService* libusb_web_port_service,
      std::vector<std::unique_ptr<PcscDriverAdaptor>> drivers);
  PcscLiteServerWebPortService(const PcscLiteServerWebPortService&) = delete;
  PcscLiteServerWebPortService& operator=(const PcscLiteServerWebPortService&) =
      delete;
  ~PcscLiteServerWebPortService();

  static PcscLiteServerWebPortService* GetInstance();

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

  // Returns the driver with the specified .so file path, or `nullptr` if
  // there's none found.
  PcscDriverAdaptor* FindDriverByFilePath(const std::string& driver_file_path);

  void PostReaderInitAddMessage(const char* reader_name,
                                int port,
                                const char* device) const;
  void PostReaderFinishAddMessage(const char* reader_name,
                                  int port,
                                  const char* device,
                                  long return_code) const;
  void PostReaderRemoveMessage(const char* reader_name, int port) const;

  // Attempts to mitigate the reader initialization error via a retry and,
  // optionally, resetting the USB device.
  //
  // This is mainly to work around issues shortly before/after the ChromeOS
  // user login, session lock and unlock, whenever the Smart Card Connector is
  // installed both in-session and on the Login/Lock Screen. (Most of the time
  // there's only one instance that's actively doing USB communication: the
  // in-session instance does this by reacting to the `chrome.loginState` state
  // changes, and the Login/Lock Screen instance is force-killed by Chrome when
  // a session becomes active. However, there's often a short period of time
  // when both instances are alive and try to concurrently access USB.)
  //
  // There are two types of transient issues: (a) failure to connect to the
  // device, and (b) receiving an unexpected USB transfer packet that was
  // originally replying to the other instance's request.
  //
  // Simply retrying the reader initialization mitigates most of issues,
  // however sometimes "b" causes the reader to end in an unresponsive state -
  // this is why there's a second mitigation of resetting the USB device.
  void AttemptMitigateReaderError(const std::string& pcsc_device_string);

 private:
  void PostMessage(const char* type, Value message_data) const;

  GlobalContext* const global_context_;
  LibusbWebPortService* const libusb_web_port_service_;
  const std::vector<std::unique_ptr<PcscDriverAdaptor>> drivers_;
  std::thread daemon_thread_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_PCSC_LITE_SERVER_WEB_PORT_SERVICE_H_
