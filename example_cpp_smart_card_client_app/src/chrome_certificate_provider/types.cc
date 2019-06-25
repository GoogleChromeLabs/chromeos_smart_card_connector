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
using PinRequestTypeConverter = EnumConverter<ccp::PinRequestType, std::string>;
using PinRequestErrorTypeConverter = EnumConverter<
    ccp::PinRequestErrorType, std::string>;
using CertificateInfoConverter = StructConverter<ccp::CertificateInfo>;
using SignRequestConverter = StructConverter<ccp::SignRequest>;
using RequestPinOptionsConverter = StructConverter<ccp::RequestPinOptions>;
using RequestPinResultsConverter = StructConverter<ccp::RequestPinResults>;
using StopPinRequestOptionsConverter = StructConverter<ccp::StopPinRequestOptions>;

// static
template <>
constexpr const char* HashConverter::GetEnumTypeName() {
  return "chrome_certificate_provider::Hash";
}

// static
template <>
template <typename Callback>
void HashConverter::VisitCorrespondingPairs(Callback callback) {
  callback(ccp::Hash::kMd5Sha1, "MD5_SHA1");
  callback(ccp::Hash::kSha1, "SHA1");
  callback(ccp::Hash::kSha256, "SHA256");
  callback(ccp::Hash::kSha384, "SHA384");
  callback(ccp::Hash::kSha512, "SHA512");
}

// static
template <>
constexpr const char* PinRequestTypeConverter::GetEnumTypeName() {
  return "chrome_certificate_provider::PinRequestType";
}

// static
template <>
template <typename Callback>
void PinRequestTypeConverter::VisitCorrespondingPairs(Callback callback) {
  callback(ccp::PinRequestType::kPin, "PIN");
  callback(ccp::PinRequestType::kPuk, "PUK");
}

// static
template <>
constexpr const char* PinRequestErrorTypeConverter::GetEnumTypeName() {
  return "chrome_certificate_provider::PinRequestErrorType";
}

// static
template <>
template <typename Callback>
void PinRequestErrorTypeConverter::VisitCorrespondingPairs(Callback callback) {
  callback(ccp::PinRequestErrorType::kInvalidPin, "INVALID_PIN");
  callback(ccp::PinRequestErrorType::kInvalidPuk, "INVALID_PUK");
  callback(ccp::PinRequestErrorType::kMaxAttemptsExceeded, "MAX_ATTEMPTS_EXCEEDED");
  callback(ccp::PinRequestErrorType::kUnknownError, "UNKNOWN_ERROR");
}

// static
template <>
constexpr const char* CertificateInfoConverter::GetStructTypeName() {
  return "chrome_certificate_provider::CertificateInfo";
}

// static
template <>
template <typename Callback>
void CertificateInfoConverter::VisitFields(
    const ccp::CertificateInfo& value, Callback callback) {
  callback(&value.certificate, "certificate");
  callback(&value.supported_hashes, "supportedHashes");
}

// static
template <>
constexpr const char* SignRequestConverter::GetStructTypeName() {
  return "chrome_certificate_provider::SignRequest";
}

// static
template <>
template <typename Callback>
void SignRequestConverter::VisitFields(
    const ccp::SignRequest& value, Callback callback) {
  callback(&value.sign_request_id, "signRequestId");
  callback(&value.digest, "digest");
  callback(&value.hash, "hash");
  callback(&value.certificate, "certificate");
}

// static
template <>
constexpr const char* RequestPinOptionsConverter::GetStructTypeName() {
  return "chrome_certificate_provider::RequestPinOptions";
}

// static
template <>
template <typename Callback>
void RequestPinOptionsConverter::VisitFields(
    const ccp::RequestPinOptions& value, Callback callback) {
  callback(&value.sign_request_id, "signRequestId");
  callback(&value.request_type, "requestType");
  callback(&value.error_type, "errorType");
  callback(&value.attempts_left, "attemptsLeft");
}

// static
template <>
constexpr const char* RequestPinResultsConverter::GetStructTypeName() {
  return "chrome_certificate_provider::RequestPinResults";
}

// static
template <>
template <typename Callback>
void RequestPinResultsConverter::VisitFields(
    const ccp::RequestPinResults& value, Callback callback) {
  callback(&value.user_input, "userInput");
}

// static
template <>
constexpr const char* StopPinRequestOptionsConverter::GetStructTypeName() {
  return "chrome_certificate_provider::StopPinRequestOptions";
}

// static
template <>
template <typename Callback>
void StopPinRequestOptionsConverter::VisitFields(
    const ccp::StopPinRequestOptions& value, Callback callback) {
  callback(&value.sign_request_id, "signRequestId");
  callback(&value.error_type, "errorType");
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

bool VarAs(const pp::Var& var, PinRequestType* result,
           std::string* error_message) {
  return gsc::PinRequestTypeConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(PinRequestType value) {
  return gsc::PinRequestTypeConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, PinRequestErrorType* result,
           std::string* error_message) {
  return gsc::PinRequestErrorTypeConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(PinRequestErrorType value) {
  return gsc::PinRequestErrorTypeConverter::ConvertToVar(value);
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

bool VarAs(const pp::Var& var, RequestPinOptions* result,
           std::string* error_message) {
  return gsc::RequestPinOptionsConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(const RequestPinOptions& value) {
  return gsc::RequestPinOptionsConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, RequestPinResults* result,
           std::string* error_message) {
  return gsc::RequestPinResultsConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(const RequestPinResults& value) {
  return gsc::RequestPinResultsConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, StopPinRequestOptions* result,
           std::string* error_message) {
  return gsc::StopPinRequestOptionsConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(const StopPinRequestOptions& value) {
  return gsc::StopPinRequestOptionsConverter::ConvertToVar(value);
}

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client
