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

#include <iostream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>
#include <ppapi_simple/ps_main.h>

class GTestEventListener final : public testing::EmptyTestEventListener {
 public:
  void OnTestProgramStart(const testing::UnitTest& /*unit_test*/) override {
    pp::VarDictionary message;
    message.Set("type", "testing_started");
    PostMessage(message);
  }

  void OnTestStart(const testing::TestInfo& test_info) override {
    ++test_count_;
    current_test_case_name_ = test_info.test_case_name();
    current_test_name_ = test_info.name();

    pp::VarDictionary message;
    message.Set("type", "test_started");
    message.Set("test_case_name", current_test_case_name_);
    message.Set("test_name", current_test_name_);
    PostMessage(message);
  }

  void OnTestPartResult(
      const testing::TestPartResult& test_part_result) override {
    if (test_part_result.failed()) {
      pp::VarDictionary message;
      message.Set("type", "test_part_failed");
      message.Set("test_case_name", current_test_case_name_);
      message.Set("test_name", current_test_name_);
      message.Set("file_name", test_part_result.file_name());
      message.Set("line_number", test_part_result.line_number());
      message.Set("summary", test_part_result.summary());
      PostMessage(message);
    }
  }

  void OnTestEnd(const testing::TestInfo& test_info) override {
    pp::VarDictionary message;
    message.Set("type", "test_finished");
    message.Set("test_case_name", current_test_case_name_);
    message.Set("test_name", current_test_name_);
    message.Set("ok", !test_info.result()->Failed());
    PostMessage(message);

    if (test_info.result()->Failed()) ++failed_test_count_;
    current_test_case_name_.clear();
    current_test_name_.clear();
  }

  void OnTestProgramEnd(const testing::UnitTest& /*unit_test*/) override {
    pp::VarDictionary message;
    message.Set("type", "testing_finished");
    message.Set("test_count", test_count_);
    message.Set("failed_test_count", failed_test_count_);
    PostMessage(message);
  }

 private:
  static void PostMessage(const pp::Var& message) {
    pp::Instance(PSGetInstanceId()).PostMessage(message);
  }

  std::string current_test_case_name_;
  std::string current_test_name_;
  int test_count_ = 0;
  int failed_test_count_ = 0;
};

int TestMain(int argc, char* argv[]) {
  // FIXME(emaxx): For some reason, the output to stderr doesn't pass to the
  // console when running from Chrome.
  std::cerr.rdbuf(std::cout.rdbuf());

  testing::InitGoogleTest(&argc, argv);
  testing::InitGoogleMock(&argc, argv);
  if (PSGetInstanceId() != 0) {
    testing::UnitTest::GetInstance()->listeners().Append(
        new GTestEventListener);
  }
  return RUN_ALL_TESTS();
}

PPAPI_SIMPLE_REGISTER_MAIN(TestMain);
