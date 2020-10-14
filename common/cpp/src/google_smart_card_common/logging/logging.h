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

// This file contains definitions related to the logging stuff.
//
// This logging library is built in the spirit of the Chromium logging library,
// but with a few simplifications and with some changes that make sense for the
// case of execution under NaCl.
//
// All emitted log messages appear, basically, in two different places:
// * in the stderr stream (which is usually tied to the browser's stderr);
// * in the JavaScript Console of the page that the NaCl module is attached to.

#ifndef GOOGLE_SMART_CARD_COMMON_LOGGING_LOGGING_H_
#define GOOGLE_SMART_CARD_COMMON_LOGGING_LOGGING_H_

#include <cstdlib>
#include <sstream>
#include <string>

namespace google_smart_card {

// This enumerate contains all supported logging severity levels.
//
// The levels are listed in the increasing order of severity.
enum class LogSeverity { kDebug, kInfo, kWarning, kError, kFatal };

namespace internal {

class LogMessage final {
 public:
  explicit LogMessage(LogSeverity severity);
  LogMessage(const LogMessage&) = delete;
  LogMessage& operator=(const LogMessage&) = delete;
  ~LogMessage();

  std::ostringstream& stream();

 private:
  LogSeverity severity_;
  std::ostringstream stream_;
};

std::string MakeCheckFailedMessage(const std::string& stringified_condition,
                                   const std::string& file, int line,
                                   const std::string& function);

std::string MakeNotreachedHitMessage(const std::string& file, int line,
                                     const std::string& function);

#define GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY(severity) \
  ::google_smart_card::internal::LogMessage(severity).stream()

#define GOOGLE_SMART_CARD_INTERNAL_LOG_DISABLING while (false)

}  // namespace internal

// This macro definition emits a log message at the specified severity level.
//
// In Release builds, logging at the LogSeverity::kDebug level is disabled
// (note that, however, the arguments _are_ still calculated during the run
// time).
//
// Logging a message at the FATAL severity level causes the program termination.
#define GOOGLE_SMART_CARD_LOG(severity) \
  GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY(severity)

//
// Series of the definitions for printing log messages with different severity
// levels. The definitions are named as GOOGLE_SMART_CARD_LOG_[level], where
// level can be any of the following: DEBUG, INFO, WARNING, ERROR, FATAL.
//
// Each macro acts as a stream-like object: it can be populated using
// operator<<, and the built message is emitted at the end of expression.
//
// In Release builds, logging on DEBUG level is disabled (and the arguments are
// _not_ even calculated during run time in that case).
//
// Logging a message at the FATAL severity level causes the program termination.
//
// Usage examples:
//    GOOGLE_SMART_CARD_LOG_DEBUG << "This is logged only in Debug build";
//    GOOGLE_SMART_CARD_LOG_ERROR << "This error is always logged";
//    if (!sanity_check)
//      GOOGLE_SMART_CARD_LOG_FATAL << "Fatal error, terminating...";
//

#ifdef NDEBUG
#define GOOGLE_SMART_CARD_LOG_DEBUG                 \
  GOOGLE_SMART_CARD_INTERNAL_LOG_DISABLING          \
  GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY( \
      ::google_smart_card::LogSeverity::kDebug)
#else
#define GOOGLE_SMART_CARD_LOG_DEBUG                 \
  GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY( \
      ::google_smart_card::LogSeverity::kDebug)
#endif

#define GOOGLE_SMART_CARD_LOG_INFO                  \
  GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY( \
      ::google_smart_card::LogSeverity::kInfo)

#define GOOGLE_SMART_CARD_LOG_WARNING               \
  GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY( \
      ::google_smart_card::LogSeverity::kWarning)

#define GOOGLE_SMART_CARD_LOG_ERROR                 \
  GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY( \
      ::google_smart_card::LogSeverity::kError)

#define GOOGLE_SMART_CARD_LOG_FATAL                 \
  GOOGLE_SMART_CARD_INTERNAL_LOGGING_WITH_SEVERITY( \
      ::google_smart_card::LogSeverity::kFatal)

// Evaluates the specified condition and, if it has a falsy value, then emits a
// FATAL message (containing the stringified condition).
//
// Usage example:
//    GOOGLE_SMART_CARD_CHECK(number >= 0);
#define GOOGLE_SMART_CARD_CHECK(condition)                          \
  do {                                                              \
    if (!(condition)) {                                             \
      GOOGLE_SMART_CARD_LOG_FATAL                                   \
          << ::google_smart_card::internal::MakeCheckFailedMessage( \
                 #condition, __FILE__, __LINE__, __func__);         \
      std::abort();                                                 \
    }                                                               \
  } while (false)

// Emits a FATAL message with the special message.
//
// Should be used as an assertion that some place of code can be never reached
// (or, for example, for suppressing the compiler warnings about missing return
// values in some complex cases).
//
// Usage example:
//    if (number % 2 == 0)
//      return 0;
//    if (number % 2 == 1)
//      return 1;
//    GOOGLE_SMART_CARD_NOTREACHED;
#define GOOGLE_SMART_CARD_NOTREACHED                                \
  do {                                                              \
    GOOGLE_SMART_CARD_LOG_FATAL                                     \
        << ::google_smart_card::internal::MakeNotreachedHitMessage( \
               __FILE__, __LINE__, __func__);                       \
    std::abort();                                                   \
  } while (false)

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_LOGGING_LOGGING_H_
