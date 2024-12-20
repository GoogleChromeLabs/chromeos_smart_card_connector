// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/cpp/src/public/logging/logging.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

namespace google_smart_card {

namespace {

bool ShouldLogWithSeverity(LogSeverity severity) {
#ifdef NDEBUG
  return severity > LogSeverity::kDebug;
#else
  (void)severity;  // this suppresses the "unused parameter" error
  return true;
#endif
}

std::string StringifyLogSeverity(LogSeverity severity) {
  switch (severity) {
    case LogSeverity::kDebug:
      return "DEBUG";
    case LogSeverity::kInfo:
      return "INFO";
    case LogSeverity::kWarning:
      return "WARNING";
    case LogSeverity::kError:
      return "ERROR";
    case LogSeverity::kFatal:
      return "FATAL";
    default:
      // This should be never reached, but it's easier to provide a safe
      // default here.
      return "";
  }
}

void EmitLogMessageToStderr(LogSeverity severity,
                            const std::string& message_text) {
  // Prepare the whole message in advance, so that we don't mess with other
  // threads writing to std::cerr too.
  std::ostringstream stream;
  stream << "[";
  stream << StringifyLogSeverity(severity) << "] " << message_text << std::endl;

  std::cerr << stream.str();
  std::cerr.flush();
}

void EmitLogMessage(LogSeverity severity, const std::string& message_text) {
  EmitLogMessageToStderr(severity, message_text);
}

}  // namespace

namespace internal {

LogMessage::LogMessage(LogSeverity severity) : severity_(severity) {}

LogMessage::~LogMessage() {
  if (ShouldLogWithSeverity(severity_)) {
    EmitLogMessage(severity_, stream_.str());
    if (severity_ == LogSeverity::kFatal) {
#if defined(__EMSCRIPTEN__)
      // Wait for some time before crashing, to leave a chance for the log
      // message with the crash reason to be delivered to the JavaScript side.
      // This is not a 100%-reliable solution, but the logging functionality in
      // the fatal error case is best-effort anyway.
      std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
      std::abort();
    }
  }
}

std::ostringstream& LogMessage::stream() {
  return stream_;
}

std::string MakeCheckFailedMessage(const std::string& stringified_condition,
                                   const std::string& file,
                                   int line,
                                   const std::string& function) {
  std::ostringstream stream;
  stream << "Check \"" << stringified_condition << "\" failed. File \"" << file
         << "\", line " << line << ", function \"" << function << "\"";
  return stream.str();
}

std::string MakeNotreachedMessage(const std::string& file,
                                  int line,
                                  const std::string& function) {
  std::ostringstream stream;
  stream << "NOTREACHED reached in file \"" << file << "\", line " << line
         << ", function \"" << function << "\"";
  return stream.str();
}

}  // namespace internal

}  // namespace google_smart_card
