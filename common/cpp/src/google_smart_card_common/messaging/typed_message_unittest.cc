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

#include <google_smart_card_common/messaging/typed_message.h>

#include <string>

#include <gtest/gtest.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

namespace google_smart_card {

namespace {

constexpr char kTypeMessageKey[] = "type";
constexpr char kDataMessageKey[] = "data";
constexpr char kSampleType[] = "sample type";
constexpr char kSampleData[] = "sample value";

}  // namespace

TEST(MessagingTypedMessageTest, CorrectTypedMessageParsing) {
  const pp::VarDictionary var = VarDictBuilder()
                                    .Add(kTypeMessageKey, kSampleType)
                                    .Add(kDataMessageKey, kSampleData)
                                    .Result();

  TypedMessage typed_message;
  std::string error_message;
  ASSERT_TRUE(VarAs(var, &typed_message, &error_message));
  EXPECT_TRUE(error_message.empty());
  EXPECT_EQ(kSampleType, typed_message.type);
  EXPECT_EQ(kSampleData, VarAs<std::string>(typed_message.data));
}

TEST(MessagingTypedMessageTest, BadTypedMessageParsing) {
  TypedMessage typed_message;
  {
    std::string error_message;
    EXPECT_FALSE(VarAs(pp::Var(), &typed_message, &error_message));
    EXPECT_FALSE(error_message.empty());
  }
  {
    std::string error_message;
    EXPECT_FALSE(VarAs(pp::VarDictionary(), &typed_message, &error_message));
    EXPECT_FALSE(error_message.empty());
  }
  {
    std::string error_message;
    EXPECT_FALSE(
        VarAs(VarDictBuilder().Add(kTypeMessageKey, kSampleType).Result(),
              &typed_message, &error_message));
    EXPECT_FALSE(error_message.empty());
  }
  {
    std::string error_message;
    EXPECT_FALSE(
        VarAs(VarDictBuilder().Add(kDataMessageKey, kSampleData).Result(),
              &typed_message, &error_message));
    EXPECT_FALSE(error_message.empty());
  }
  {
    std::string error_message;
    EXPECT_FALSE(VarAs(VarDictBuilder()
                           .Add(kTypeMessageKey, 123)
                           .Add(kDataMessageKey, kSampleData)
                           .Result(),
                       &typed_message, &error_message));
    EXPECT_FALSE(error_message.empty());
  }
}

TEST(MessagingTypedMessageTest, TypedMessageMaking) {
  TypedMessage typed_message;
  typed_message.type = kSampleType;
  typed_message.data = kSampleData;
  const pp::Var var = MakeVar(typed_message);

  const pp::VarDictionary var_dict = VarAs<pp::VarDictionary>(var);
  EXPECT_EQ(kSampleType,
            GetVarDictValueAs<std::string>(var_dict, kTypeMessageKey));
  EXPECT_EQ(kSampleData,
            GetVarDictValueAs<std::string>(var_dict, kDataMessageKey));
}

}  // namespace google_smart_card
