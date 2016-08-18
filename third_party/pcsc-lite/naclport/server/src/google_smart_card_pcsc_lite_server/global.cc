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

#include <google_smart_card_pcsc_lite_server/global.h>

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <thread>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/pp_var_utils/construction.h>

extern "C" {
#include "winscard.h"
#include "debuglog.h"
#include "hotplug.h"
#include "powermgt_generic.h"
#include "readerfactory.h"
#include "winscard_svc.h"
}

#include "server_sockets_manager.h"
#include "socketpair_emulation.h"

const char kLoggingPrefix[] = "[PC/SC-Lite NaCl port] ";

namespace google_smart_card {

namespace {

void PcscLiteServerDaemonThreadMain() {
  // TODO(emaxx): Stop the event loop during pp::Instance destruction.
  while (true) {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "[daemon thread] " <<
        "Waiting for the new connected clients...";
    uint32_t server_socket_file_descriptor = static_cast<uint32_t>(
        PcscLiteServerSocketsManager::GetInstance()->WaitAndPop());

    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "[daemon thread] A new " <<
        "client was connected, starting a handler thread...";
    // Note: even though the CreateContextThread function accepts its
    // server_socket_file_descriptor argument by pointer, it doesn't store the
    // pointer itself anywhere - so it's safe to use a local variable here.
    GOOGLE_SMART_CARD_CHECK(
        CreateContextThread(&server_socket_file_descriptor) == SCARD_S_SUCCESS);
  }
}

PcscLiteServerGlobal* g_pcsc_lite_server = nullptr;

const char kReaderInitAddMessageType[] = "reader_init_add";
const char kReaderFinishAddMessageType[] = "reader_finish_add";
const char kReaderRemoveMessageType[] = "reader_remove";
const char kNameMessageKey[] = "readerName";
const char kPortMessageKey[] = "port";
const char kDeviceMessageKey[] = "device";
const char kReturnCodeMessageKey[] = "returnCode";

}  // namespace

PcscLiteServerGlobal::PcscLiteServerGlobal(pp::Instance* pp_instance)
      : pp_instance_(pp_instance) {
  GOOGLE_SMART_CARD_CHECK(!g_pcsc_lite_server);
  g_pcsc_lite_server = this;
}

PcscLiteServerGlobal::~PcscLiteServerGlobal() {
  GOOGLE_SMART_CARD_CHECK(g_pcsc_lite_server == this);
  g_pcsc_lite_server = nullptr;
}

void PcscLiteServerGlobal::Detach () {
  const std::unique_lock<std::mutex> lock(mutex_);
  pp_instance_ = nullptr;
}

void PcscLiteServerGlobal::InitializeAndRunPcscLiteServer() {
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Initialization...";

  SocketpairEmulationManager::CreateGlobalInstance();
  PcscLiteServerSocketsManager::CreateGlobalInstance();

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Setting up PC/SC-Lite " <<
      "logging...";
  DebugLogSetLogType(DEBUGLOG_SYSLOG_DEBUG);
#ifdef NDEBUG
  DebugLogSetLevel(PCSC_LOG_ERROR);
#else
  DebugLogSetLevel(PCSC_LOG_DEBUG);
  DebugLogSetCategory(DEBUG_CATEGORY_APDU | DEBUG_CATEGORY_SW);
#endif
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "PC/SC-Lite logging was " <<
      "set up.";

  LONG return_code;

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Allocating reader " <<
      "structures...";
  return_code = RFAllocateReaderSpace(0);
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Reader structures " <<
      "allocation finished with the following result: \"" <<
      pcsc_stringify_error(return_code) << "\".";
  GOOGLE_SMART_CARD_CHECK(return_code == SCARD_S_SUCCESS);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Performing initial hot " <<
      "plug drivers search...";
  return_code = HPSearchHotPluggables();
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Initial hot plug " <<
      "drivers search finished with the following result code: " <<
      return_code << ".";
  GOOGLE_SMART_CARD_CHECK(return_code == 0);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Registering for hot " <<
      "plug events...";
  // FIXME(emaxx): Currently this ends up on polling the libusb each second, as
  // it doesn't provide any way to subscribe for the device list change. But
  // it's possible to optimize this onto publisher-pattern-style implementation,
  // by handling the chrome.usb API events (see
  // <https://developer.chrome.com/apps/usb#Events>) and using them in a
  // replacement implementation of the currently used original hotplug_libusb.cc
  // source file.
  return_code = HPRegisterForHotplugEvents();
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Registering for hot " <<
      "plug events finished with the following result code: " << return_code <<
      ".";
  GOOGLE_SMART_CARD_CHECK(return_code == 0);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Allocating client " <<
      "structures...";
  return_code = ContextsInitialize(0, 0);
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Client structures " <<
      "allocation finished with the following result code: " << return_code <<
      "...";
  GOOGLE_SMART_CARD_CHECK(return_code == 1);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Waiting for the readers " <<
      "initialization...";
  RFWaitForReaderInit();
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Waiting for the readers " <<
      "initialization finished.";

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Registering for power " <<
      "events...";
  return_code = PMRegisterForPowerEvents();
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Registering for power " <<
      "events finished with the following result code: " << return_code << ".";
  GOOGLE_SMART_CARD_CHECK(return_code == 0);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Starting PC/SC-Lite " <<
      "daemon thread...";
  std::thread daemon_thread(PcscLiteServerDaemonThreadMain);
  daemon_thread.detach();
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "PC/SC-Lite daemon " <<
      "thread has started.";

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Initialization " <<
      "successfully finished.";
}

const PcscLiteServerGlobal* PcscLiteServerGlobal::GetInstance() {
  GOOGLE_SMART_CARD_CHECK(g_pcsc_lite_server);
  return g_pcsc_lite_server;
}

void PcscLiteServerGlobal::PostMessage(
    const char* type, const pp::VarDictionary& message_data) const {
  const std::unique_lock<std::mutex> lock(mutex_);
  if (pp_instance_)
    pp_instance_->PostMessage(MakeTypedMessage(type, message_data));
}

void PcscLiteServerGlobal::PostReaderInitAddMessage(const char* reader_name,
    int port, const char* device) const {
  PostMessage(kReaderInitAddMessageType, VarDictBuilder()
      .Add(kNameMessageKey, reader_name).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Result());
}

void PcscLiteServerGlobal::PostReaderFinishAddMessage(const char* reader_name,
    int port, const char* device, long return_code) const {
  PostMessage(kReaderFinishAddMessageType, VarDictBuilder()
      .Add(kNameMessageKey, reader_name).Add(kPortMessageKey, port)
      .Add(kDeviceMessageKey, device).Add(kReturnCodeMessageKey, return_code)
      .Result());
}

void PcscLiteServerGlobal::PostReaderRemoveMessage(const char* reader_name,
    int port) const {
  PostMessage(kReaderRemoveMessageType, VarDictBuilder()
      .Add(kNameMessageKey, reader_name).Add(kPortMessageKey, port).Result());
}

}  // namespace google_smart_card
