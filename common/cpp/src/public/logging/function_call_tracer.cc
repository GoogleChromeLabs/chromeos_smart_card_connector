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

#include "common/cpp/src/public/logging/function_call_tracer.h"

#include <stdint.h>

#include <atomic>

namespace google_smart_card {

namespace {

int64_t GenerateFunctionCallId() {
  static std::atomic<int64_t> next_function_call_id(0);
  return ++next_function_call_id;
}

}  // namespace

FunctionCallTracer::FunctionCallTracer(const std::string& function_name,
                                       const std::string& logging_prefix,
                                       LogSeverity log_severity)
    : function_call_id_(GenerateFunctionCallId()),
      function_name_(function_name),
      logging_prefix_(logging_prefix),
      log_severity_(log_severity) {}

FunctionCallTracer::~FunctionCallTracer() = default;

void FunctionCallTracer::AddPassedArg(const std::string& name,
                                      const std::string& dumped_value) {
  passed_args_.emplace_back(name, dumped_value);
}

void FunctionCallTracer::AddReturnValue(const std::string& dumped_value) {
  GOOGLE_SMART_CARD_CHECK(!dumped_return_value_);
  dumped_return_value_ = dumped_value;
}

void FunctionCallTracer::AddReturnedArg(const std::string& name,
                                        const std::string& dumped_value) {
  returned_args_.emplace_back(name, dumped_value);
}

void FunctionCallTracer::LogEntrance() const {
  GOOGLE_SMART_CARD_LOG(log_severity_)
      << logging_prefix_ << function_name_ << "#" << function_call_id_ << "("
      << DumpArgs(passed_args_) << "): called...";
}

void FunctionCallTracer::LogExit() const {
  std::string results_part;
  if (dumped_return_value_)
    results_part = *dumped_return_value_;
  if (!returned_args_.empty()) {
    if (!results_part.empty())
      results_part += ", ";
    results_part += DumpArgs(returned_args_);
  }

  GOOGLE_SMART_CARD_LOG(log_severity_)
      << logging_prefix_ << function_name_ << "#" << function_call_id_
      << ": returning" << (results_part.empty() ? "" : " ") << results_part;
}

FunctionCallTracer::ArgNameWithValue::ArgNameWithValue(
    const std::string& name,
    const std::string& dumped_value)
    : name(name), dumped_value(dumped_value) {}

// static
std::string FunctionCallTracer::DumpArgs(
    const std::vector<ArgNameWithValue>& args) {
  std::string result;
  for (const auto& arg : args) {
    if (!result.empty())
      result += ", ";
    result += arg.name;
    result += "=";
    result += arg.dumped_value;
  }
  return result;
}

}  // namespace google_smart_card
