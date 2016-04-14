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

#include <string>

#include <gtest/gtest.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

const char kTypeMessageKey[] = "type";
const char kDataMessageKey[] = "data";
const char kSampleType[] = "sample type";
const char kSampleData[] = "sample value";

namespace google_smart_card {

TEST(MessagingTypedMessageTest, CorrectTypedMessageParsing) {
  const pp::VarDictionary var = VarDictBuilder()
      .Add(kTypeMessageKey, kSampleType)
      .Add(kDataMessageKey, kSampleData)
      .Result();

  std::string type;
  pp::Var data;
  ASSERT_TRUE(ParseTypedMessage(var, &type, &data));
  EXPECT_EQ(kSampleType, type);
  EXPECT_EQ(kSampleData, VarAs<std::string>(data));
}

TEST(MessagingTypedMessageTest, BadTypedMessageParsing) {
  std::string type;
  pp::Var data;
  EXPECT_FALSE(ParseTypedMessage(pp::Var(), &type, &data));
  EXPECT_FALSE(ParseTypedMessage(pp::VarDictionary(), &type, &data));
  EXPECT_FALSE(ParseTypedMessage(
      VarDictBuilder().Add(kTypeMessageKey, kSampleType).Result(),
      &type,
      &data));
  EXPECT_FALSE(ParseTypedMessage(
      VarDictBuilder().Add(kDataMessageKey, kSampleData).Result(),
      &type,
      &data));
  EXPECT_FALSE(ParseTypedMessage(
      VarDictBuilder()
          .Add(kTypeMessageKey, 123)
          .Add(kDataMessageKey, kSampleData)
          .Result(),
      &type, &data));
}

TEST(MessagingTypedMessageTest, TypedMessageMaking) {
  const pp::Var var = MakeTypedMessage(kSampleType, kSampleData);

  const pp::VarDictionary var_dict = VarAs<pp::VarDictionary>(var);
  EXPECT_EQ(kSampleType,
            GetVarDictValueAs<std::string>(var_dict, kTypeMessageKey));
  EXPECT_EQ(kSampleData,
            GetVarDictValueAs<std::string>(var_dict, kDataMessageKey));
}

}  // namespace google_smart_card
