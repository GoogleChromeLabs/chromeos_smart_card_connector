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

#ifndef GOOGLE_SMART_CARD_COMMON_LOGGING_FUNCTION_CALL_TRACER_H_
#define GOOGLE_SMART_CARD_COMMON_LOGGING_FUNCTION_CALL_TRACER_H_

#include <string>
#include <vector>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>

namespace google_smart_card {

// Helper class for implementing function call tracing - i.e. emitting debug log
// messages for function calls: first with the function input arguments, then -
// with the function return value and the values of its output arguments.
//
// TODO(emaxx): Add assertions that the class is used correctly (i.e. the
// methods are called in a valid order and valid number of times).
class FunctionCallTracer final {
 public:
  explicit FunctionCallTracer(const std::string& function_name,
                              const std::string& logging_prefix = "",
                              LogSeverity log_severity = LogSeverity::kDebug);
  FunctionCallTracer(const FunctionCallTracer&) = delete;
  FunctionCallTracer& operator=(const FunctionCallTracer&) = delete;
  ~FunctionCallTracer();

  void AddPassedArg(const std::string& name, const std::string& dumped_value);

  void AddReturnValue(const std::string& dumped_value);

  void AddReturnedArg(const std::string& name, const std::string& dumped_value);

  void LogEntrance() const;
  void LogEntrance(const std::string& logging_prefix) const;

  void LogExit() const;
  void LogExit(const std::string& logging_prefix) const;

 private:
  struct ArgNameWithValue {
    ArgNameWithValue(const std::string& name, const std::string& dumped_value);

    std::string name;
    std::string dumped_value;
  };

  static std::string DumpArgs(const std::vector<ArgNameWithValue>& args);

  const std::string function_name_;
  const std::string logging_prefix_;
  const LogSeverity log_severity_;
  std::vector<ArgNameWithValue> passed_args_;
  optional<std::string> dumped_return_value_;
  std::vector<ArgNameWithValue> returned_args_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_LOGGING_FUNCTION_CALL_TRACER_H_
