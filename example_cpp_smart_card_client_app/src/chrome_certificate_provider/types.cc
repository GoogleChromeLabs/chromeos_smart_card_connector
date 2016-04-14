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

#include "chrome_certificate_provider/types.h"

#include <google_smart_card_common/pp_var_utils/enum_converter.h>
#include <google_smart_card_common/pp_var_utils/struct_converter.h>

namespace scc = smart_card_client;
namespace ccp = scc::chrome_certificate_provider;
namespace gsc = google_smart_card;

namespace google_smart_card {

using HashConverter = EnumConverter<ccp::Hash, std::string>;

using CertificateInfoConverter = StructConverter<ccp::CertificateInfo>;

using SignRequestConverter = StructConverter<ccp::SignRequest>;

template <>
constexpr const char* HashConverter::GetEnumTypeName() {
  return "chrome_certificate_provider::Hash";
}

template <>
template <typename Callback>
void HashConverter::VisitCorrespondingPairs(Callback callback) {
  callback(ccp::Hash::kMd5Sha1, "MD5_SHA1");
  callback(ccp::Hash::kSha1, "SHA1");
  callback(ccp::Hash::kSha256, "SHA256");
  callback(ccp::Hash::kSha384, "SHA384");
  callback(ccp::Hash::kSha512, "SHA512");
}

template <>
constexpr const char* CertificateInfoConverter::GetStructTypeName() {
  return "chrome_certificate_provider::CertificateInfo";
}

template <>
template <typename Callback>
void CertificateInfoConverter::VisitFields(
    const ccp::CertificateInfo& value, Callback callback) {
  callback(&value.certificate, "certificate");
  callback(&value.supported_hashes, "supportedHashes");
}

template <>
constexpr const char* SignRequestConverter::GetStructTypeName() {
  return "chrome_certificate_provider::SignRequest";
}

template <>
template <typename Callback>
void SignRequestConverter::VisitFields(
    const ccp::SignRequest& value, Callback callback) {
  callback(&value.digest, "digest");
  callback(&value.hash, "hash");
  callback(&value.certificate, "certificate");
}

}  // namespace google_smart_card

namespace smart_card_client {

namespace chrome_certificate_provider {

bool VarAs(const pp::Var& var, Hash* result, std::string* error_message) {
  return gsc::HashConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(Hash value) {
  return gsc::HashConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var, CertificateInfo* result, std::string* error_message) {
  return gsc::CertificateInfoConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(const CertificateInfo& value) {
  return gsc::CertificateInfoConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var, SignRequest* result, std::string* error_message) {
  return gsc::SignRequestConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(const SignRequest& value) {
  return gsc::SignRequestConverter::ConvertToVar(value);
}

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client
