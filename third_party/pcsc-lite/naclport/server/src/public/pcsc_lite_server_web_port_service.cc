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

#include "third_party/pcsc-lite/naclport/server/src/public/pcsc_lite_server_web_port_service.h"

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <thread>
#include <utility>

#include "common/cpp/src/public/ipc_emulation.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"

#include "server_sockets_manager.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/version.h>
#endif

// Include the C headers after all other includes, because the preprocessor
// definitions from them interfere with the symbols from the standard C++
// library.
extern "C" {
#include "winscard.h"
#include "debuglog.h"
#include "eventhandler.h"
#include "hotplug.h"
#include "readerfactory.h"
#include "sys_generic.h"
#include "winscard_svc.h"
}

// Old versions of Emscripten have buggy multi-threading - so bail out if the
// developer still hasn't updated their local Emscripten version.
#ifdef __EMSCRIPTEN__
#if __EMSCRIPTEN_major__ < 2 ||                                \
    (__EMSCRIPTEN_major__ == 2 && __EMSCRIPTEN_minor__ == 0 && \
     __EMSCRIPTEN_tiny__ < 31)
#error "Emscripten >=2.0.31 must be used"
#endif
#endif  // __EMSCRIPTEN__

namespace google_smart_card {

namespace {

PcscLiteServerWebPortService* g_pcsc_lite_server = nullptr;

// Path to the directory containing driver config files (in case of our webport,
// it's only CCID driver's one).
// The path must be relative, so that it works both inside the App/Extension as
// well as in unit tests executed natively.
constexpr char kDriverConfigPath[] =
    "executable-module-filesystem/pcsc/drivers";

constexpr char kLoggingPrefix[] = "[PC/SC-Lite NaCl port] ";

// Constants for message types that are sent to the JavaScript side. These
// strings must match the ones in reader-tracker.js.
constexpr char kReaderInitAddMessageType[] = "reader_init_add";
constexpr char kReaderFinishAddMessageType[] = "reader_finish_add";
constexpr char kReaderRemoveMessageType[] = "reader_remove";

// Message data for the message that notifies the JavaScript side that a reader
// is being added by the PC/SC-Lite daemon.
struct ReaderInitAddMessageData {
  std::string reader_name;
  int port;
  std::string device;
};

// Message data for the message that notifies the JavaScript side that a reader
// is completely added by the PC/SC-Lite daemon.
struct ReaderFinishAddMessageData {
  std::string reader_name;
  int port;
  std::string device;
  long return_code;
};

// Message data for the message that notifies the JavaScript side that a reader
// is removed by the PC/SC-Lite daemon.
struct ReaderRemoveMessageData {
  std::string reader_name;
  int port;
};

void PcscLiteServerDaemonThreadMain() {
  while (true) {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "[daemon thread] "
                                << "Waiting for the new connected clients...";
    optional<int> server_socket_file_descriptor =
        PcscLiteServerSocketsManager::GetInstance()->WaitAndPop();
    if (!server_socket_file_descriptor) {
      // A shutdown signal received.
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix
                                  << "[daemon thread] Shutting down...";
      break;
    }

    GOOGLE_SMART_CARD_LOG_DEBUG
        << kLoggingPrefix << "[daemon thread] A new "
        << "client was connected, starting a handler thread...";
    // Note: even though the CreateContextThread function accepts its
    // server_socket_file_descriptor argument by pointer, it doesn't store the
    // pointer itself anywhere - so it's safe to use a local variable here.
    uint32_t server_socket_file_descriptor_unsigned =
        static_cast<uint32_t>(server_socket_file_descriptor.value());
    // FIXME(emaxx): Deal with cases when CreateContextThread returns errors.
    // Looks like it may happen legitimately when the abusive client(s) request
    // to establish too many requests. Probably, some limitation should be
    // applied to all clients.
    GOOGLE_SMART_CARD_CHECK(
        ::CreateContextThread(&server_socket_file_descriptor_unsigned) ==
        SCARD_S_SUCCESS);
  }

  // Clean up the structures and threads owned by the third-party PC/SC-Lite
  // code. This follows the code in the "if (AraKiri)" block in the
  // `SVCServiceRunLoop()` function in pcsc-lite/src/src/pcscdaemon.c.
  HPStopHotPluggables();

  // TODO: Upstream's approach with a magic sleep is flaky: the background
  // thread might be still running after this point, causing crashes and memory
  // leaks in tests. Replace this with a proper waiting mechanism.
  int timeout_seconds = 10;
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
  timeout_seconds *= 2;
#endif
#if !defined(NDEBUG)
  timeout_seconds *= 2;
#endif
  SYS_Sleep(timeout_seconds);

  RFCleanupReaders();
  EHDeinitializeEventStructures();
  ContextsDeinitialize();
}

}  // namespace

template <>
StructValueDescriptor<ReaderInitAddMessageData>::Description
StructValueDescriptor<ReaderInitAddMessageData>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // reader-tracker.js.
  return Describe("ReaderInitAddMessageData")
      .WithField(&ReaderInitAddMessageData::reader_name, "readerName")
      .WithField(&ReaderInitAddMessageData::port, "port")
      .WithField(&ReaderInitAddMessageData::device, "device");
}

template <>
StructValueDescriptor<ReaderFinishAddMessageData>::Description
StructValueDescriptor<ReaderFinishAddMessageData>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // reader-tracker.js.
  return Describe("ReaderFinishAddMessageData")
      .WithField(&ReaderFinishAddMessageData::reader_name, "readerName")
      .WithField(&ReaderFinishAddMessageData::port, "port")
      .WithField(&ReaderFinishAddMessageData::device, "device")
      .WithField(&ReaderFinishAddMessageData::return_code, "returnCode");
}

template <>
StructValueDescriptor<ReaderRemoveMessageData>::Description
StructValueDescriptor<ReaderRemoveMessageData>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // reader-tracker.js.
  return Describe("ReaderRemoveMessageData")
      .WithField(&ReaderRemoveMessageData::reader_name, "readerName")
      .WithField(&ReaderRemoveMessageData::port, "port");
}

PcscLiteServerWebPortService::PcscLiteServerWebPortService(
    GlobalContext* global_context)
    : global_context_(global_context) {
  GOOGLE_SMART_CARD_CHECK(!g_pcsc_lite_server);
  g_pcsc_lite_server = this;
}

PcscLiteServerWebPortService::~PcscLiteServerWebPortService() {
  // If the daemon thread is joinable, it means `ShutDownAndWait()` wasn't
  // called, which is a violation of the contract.
  GOOGLE_SMART_CARD_CHECK(!daemon_thread_.joinable());

  GOOGLE_SMART_CARD_CHECK(g_pcsc_lite_server == this);
  g_pcsc_lite_server = nullptr;
}

// static
const PcscLiteServerWebPortService*
PcscLiteServerWebPortService::GetInstance() {
  GOOGLE_SMART_CARD_CHECK(g_pcsc_lite_server);
  return g_pcsc_lite_server;
}

void PcscLiteServerWebPortService::InitializeAndRunDaemonThread() {
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Initialization...";

  IpcEmulation::CreateGlobalInstance();
  PcscLiteServerSocketsManager::CreateGlobalInstance();

  ::SYS_InitRandom();

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Setting up PC/SC-Lite "
                              << "logging...";
  ::DebugLogSetLogType(DEBUGLOG_SYSLOG_DEBUG);
#ifdef NDEBUG
  ::DebugLogSetLevel(PCSC_LOG_ERROR);
#else
  ::DebugLogSetLevel(PCSC_LOG_DEBUG);
  ::DebugLogSetCategory(DEBUG_CATEGORY_APDU | DEBUG_CATEGORY_SW);
#endif
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "PC/SC-Lite logging was "
                              << "set up.";

  LONG return_code;

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Allocating reader "
                              << "structures...";
  return_code = ::RFAllocateReaderSpace(0);
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Reader structures "
      << "allocation finished with the following result: \""
      << ::pcsc_stringify_error(return_code) << "\".";
  GOOGLE_SMART_CARD_CHECK(return_code == SCARD_S_SUCCESS);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Performing initial hot "
                              << "plug drivers search...";
  return_code = ::HPSearchHotPluggables(kDriverConfigPath);
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Initial hot plug "
      << "drivers search finished with the following result code: "
      << return_code << ".";
  GOOGLE_SMART_CARD_CHECK(return_code == 0);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Registering for hot "
                              << "plug events...";
  // FIXME(emaxx): Currently this ends up on polling the libusb each second, as
  // it doesn't provide any way to subscribe for the device list change. But
  // it's possible to optimize this onto publisher-pattern-style implementation,
  // by handling the chrome.usb API events (see
  // <https://developer.chrome.com/apps/usb#Events>) and using them in a
  // replacement implementation of the currently used original hotplug_libusb.c
  // source file.
  return_code = ::HPRegisterForHotplugEvents(kDriverConfigPath);
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Registering for hot "
      << "plug events finished with the following result code: " << return_code
      << ".";
  GOOGLE_SMART_CARD_CHECK(return_code == 0);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Allocating client "
                              << "structures...";
  return_code = ::ContextsInitialize(0, 0);
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Client structures "
      << "allocation finished with the following result code: " << return_code
      << "...";
  GOOGLE_SMART_CARD_CHECK(return_code == 1);

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Waiting for the readers "
                              << "initialization...";
  ::RFWaitForReaderInit();
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Waiting for the readers "
                              << "initialization finished.";

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Starting PC/SC-Lite "
                              << "daemon thread...";
  daemon_thread_ = std::thread(PcscLiteServerDaemonThreadMain);
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "PC/SC-Lite daemon "
                              << "thread has started.";

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Initialization "
                              << "successfully finished.";
}

void PcscLiteServerWebPortService::ShutDownAndWait() {
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Shutting down the PC/SC-Lite daemon thread...";
  // This notifies the daemon thread to shut down.
  PcscLiteServerSocketsManager::GetInstance()->ShutDown();
  daemon_thread_.join();
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix
                              << "The PC/SC-Lite daemon thread shut down.";

  // Shut down the global state created in `InitializeAndRunDaemonThread()`.
  PcscLiteServerSocketsManager::DestroyGlobalInstance();
  IpcEmulation::DestroyGlobalInstance();
}

void PcscLiteServerWebPortService::PostReaderInitAddMessage(
    const char* reader_name,
    int port,
    const char* device) const {
  ReaderInitAddMessageData message_data;
  message_data.reader_name = reader_name;
  message_data.port = port;
  message_data.device = device;
  PostMessage(kReaderInitAddMessageType,
              ConvertToValueOrDie(std::move(message_data)));
}

void PcscLiteServerWebPortService::PostReaderFinishAddMessage(
    const char* reader_name,
    int port,
    const char* device,
    long return_code) const {
  ReaderFinishAddMessageData message_data;
  message_data.reader_name = reader_name;
  message_data.port = port;
  message_data.device = device;
  message_data.return_code = return_code;
  PostMessage(kReaderFinishAddMessageType,
              ConvertToValueOrDie(std::move(message_data)));
}

void PcscLiteServerWebPortService::PostReaderRemoveMessage(
    const char* reader_name,
    int port) const {
  ReaderRemoveMessageData message_data;
  message_data.reader_name = reader_name;
  message_data.port = port;
  PostMessage(kReaderRemoveMessageType,
              ConvertToValueOrDie(std::move(message_data)));
}

void PcscLiteServerWebPortService::PostMessage(const char* type,
                                               Value message_data) const {
  TypedMessage typed_message;
  typed_message.type = type;
  typed_message.data = std::move(message_data);
  global_context_->PostMessageToJs(
      ConvertToValueOrDie(std::move(typed_message)));
}

}  // namespace google_smart_card
