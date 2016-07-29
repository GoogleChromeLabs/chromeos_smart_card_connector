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

#include <google_smart_card_common/logging/logging.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

#ifdef __native_client__
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#endif  // __native_client__

const char kTypeMessageKey[] = "type";
const char kMessageType[] = "log_message";
const char kDataMessageKey[] = "data";
const char kDataLogLevelMessageKey[] = "log_level";
const char kDataTextMessageKey[] = "text";

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

void EmitLogMessageToStderr(
    LogSeverity severity, const std::string& message_text) {
  // Prepare the whole message in advance, so that we don't mess with other
  // threads writing to std::cerr too.
  std::ostringstream stream;
  stream << "[NaCl module " << StringifyLogSeverity(severity) << "] " <<
      message_text << std::endl;

  std::cerr << stream.str();
  std::cerr.flush();
}

#ifdef __native_client__

std::string GetGoogLogLevelByLogSeverity(LogSeverity severity) {
  switch (severity) {
    case LogSeverity::kDebug:
      return "FINE";
    case LogSeverity::kInfo:
      return "INFO";
    case LogSeverity::kWarning:
      return "WARNING";
    case LogSeverity::kError:
      return "WARNING";
    case LogSeverity::kFatal:
      return "SEVERE";
    default:
      // This should be never reached, but it's easier to provide a safe
      // default here.
      return "FINE";
  }
}

std::string CleanupLogMessageTextForVar(const std::string& message_text) {
  // Note that even though this duplicates the CleanupStringForVar function
  // functionality, it's not used here, because the logging implementation
  // intentionally has no dependencies on any other code in this project.
  const char kPlaceholder = '_';
  std::string result = message_text;
  std::replace_if(
      result.begin(),
      result.end(),
      [](char c) {
        return !std::isprint(static_cast<unsigned char>(c));
      },
      kPlaceholder);
  return result;
}

void EmitLogMessageToJavaScript(
    LogSeverity severity, const std::string& message_text) {
  pp::VarDictionary message_data;
  message_data.Set(
      kDataLogLevelMessageKey, GetGoogLogLevelByLogSeverity(severity));
  message_data.Set(
      kDataTextMessageKey, CleanupLogMessageTextForVar(message_text));
  pp::VarDictionary message;
  message.Set(kTypeMessageKey, kMessageType);
  message.Set(kDataMessageKey, message_data);

  const pp::Module* const pp_module = pp::Module::Get();
  if (pp_module) {
    const pp::Module::InstanceMap pp_instance_map =
        pp_module->current_instances();
    for (const std::pair<PP_Instance, pp::Instance*>& instance_map_item :
         pp_instance_map) {
      pp::Instance* const instance = instance_map_item.second;
      if (instance)
        instance->PostMessage(message);
    }
  }
}

#endif  // __native_client__

void EmitLogMessage(LogSeverity severity, const std::string& message_text) {
  EmitLogMessageToStderr(severity, message_text);
#ifdef __native_client__
  EmitLogMessageToJavaScript(severity, message_text);
#endif  // __native_client__
}

}  // namespace

namespace internal {

LogMessage::LogMessage(LogSeverity severity)
    : severity_(severity) {}

LogMessage::~LogMessage() {
  if (ShouldLogWithSeverity(severity_)) {
    EmitLogMessage(severity_, stream_.str());
    if (severity_ == LogSeverity::kFatal)
      std::abort();
  }
}

std::ostringstream& LogMessage::stream() {
  return stream_;
}

std::string MakeCheckFailedMessage(
    const std::string& stringified_condition,
    const std::string& file,
    int line,
    const std::string& function) {
  std::ostringstream stream;
  stream << "Check \"" << stringified_condition << "\" failed. File \"" <<
      file << "\", line " << line << ", function \"" << function << "\"";
  return stream.str();
}

std::string MakeNotreachedHitMessage(
    const std::string& file, int line, const std::string& function) {
  std::ostringstream stream;
  stream << "NOTREACHED hit at file \"" << file << "\", line " << line <<
      ", function \"" << function << "\"";
  return stream.str();
}

}  // namespace internal

}  // namespace google_smart_card
