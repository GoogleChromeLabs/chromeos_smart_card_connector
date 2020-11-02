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
#include <google_smart_card_common/value_conversion.h>

namespace scc = smart_card_client;
namespace ccp = scc::chrome_certificate_provider;
namespace gsc = google_smart_card;

namespace google_smart_card {

using AlgorithmConverter = EnumConverter<ccp::Algorithm, std::string>;
using ErrorConverter = EnumConverter<ccp::Error, std::string>;
using PinRequestTypeConverter = EnumConverter<ccp::PinRequestType, std::string>;
using PinRequestErrorTypeConverter =
    EnumConverter<ccp::PinRequestErrorType, std::string>;
using ClientCertificateInfoConverter =
    StructConverter<ccp::ClientCertificateInfo>;
using SetCertificatesDetailsConverter =
    StructConverter<ccp::SetCertificatesDetails>;
using SignatureRequestConverter = StructConverter<ccp::SignatureRequest>;
using RequestPinOptionsConverter = StructConverter<ccp::RequestPinOptions>;
using RequestPinResultsConverter = StructConverter<ccp::RequestPinResults>;
using StopPinRequestOptionsConverter =
    StructConverter<ccp::StopPinRequestOptions>;

template <>
EnumValueDescriptor<ccp::Algorithm>::Description
EnumValueDescriptor<ccp::Algorithm>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::Algorithm")
      .WithItem(ccp::Algorithm::kRsassaPkcs1v15Md5Sha1,
                "RSASSA_PKCS1_v1_5_MD5_SHA1")
      .WithItem(ccp::Algorithm::kRsassaPkcs1v15Sha1, "RSASSA_PKCS1_v1_5_SHA1")
      .WithItem(ccp::Algorithm::kRsassaPkcs1v15Sha256,
                "RSASSA_PKCS1_v1_5_SHA256")
      .WithItem(ccp::Algorithm::kRsassaPkcs1v15Sha384,
                "RSASSA_PKCS1_v1_5_SHA384")
      .WithItem(ccp::Algorithm::kRsassaPkcs1v15Sha512,
                "RSASSA_PKCS1_v1_5_SHA512");
}

// static
template <>
constexpr const char* AlgorithmConverter::GetEnumTypeName() {
  return "chrome_certificate_provider::Algorithm";
}

// static
template <>
template <typename Callback>
void AlgorithmConverter::VisitCorrespondingPairs(Callback callback) {
  callback(ccp::Algorithm::kRsassaPkcs1v15Md5Sha1,
           "RSASSA_PKCS1_v1_5_MD5_SHA1");
  callback(ccp::Algorithm::kRsassaPkcs1v15Sha1, "RSASSA_PKCS1_v1_5_SHA1");
  callback(ccp::Algorithm::kRsassaPkcs1v15Sha256, "RSASSA_PKCS1_v1_5_SHA256");
  callback(ccp::Algorithm::kRsassaPkcs1v15Sha384, "RSASSA_PKCS1_v1_5_SHA384");
  callback(ccp::Algorithm::kRsassaPkcs1v15Sha512, "RSASSA_PKCS1_v1_5_SHA512");
}

template <>
EnumValueDescriptor<ccp::Error>::Description
EnumValueDescriptor<ccp::Error>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::Error")
      .WithItem(ccp::Error::kGeneral, "GENERAL_ERROR");
}

// static
template <>
constexpr const char* ErrorConverter::GetEnumTypeName() {
  return "chrome_certificate_provider::Error";
}

// static
template <>
template <typename Callback>
void ErrorConverter::VisitCorrespondingPairs(Callback callback) {
  callback(ccp::Error::kGeneral, "GENERAL_ERROR");
}

template <>
EnumValueDescriptor<ccp::PinRequestType>::Description
EnumValueDescriptor<ccp::PinRequestType>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::PinRequestType")
      .WithItem(ccp::PinRequestType::kPin, "PIN")
      .WithItem(ccp::PinRequestType::kPuk, "PUK");
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

template <>
EnumValueDescriptor<ccp::PinRequestErrorType>::Description
EnumValueDescriptor<ccp::PinRequestErrorType>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::PinRequestErrorType")
      .WithItem(ccp::PinRequestErrorType::kInvalidPin, "INVALID_PIN")
      .WithItem(ccp::PinRequestErrorType::kInvalidPuk, "INVALID_PUK")
      .WithItem(ccp::PinRequestErrorType::kMaxAttemptsExceeded,
                "MAX_ATTEMPTS_EXCEEDED")
      .WithItem(ccp::PinRequestErrorType::kUnknownError, "UNKNOWN_ERROR");
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
  callback(ccp::PinRequestErrorType::kMaxAttemptsExceeded,
           "MAX_ATTEMPTS_EXCEEDED");
  callback(ccp::PinRequestErrorType::kUnknownError, "UNKNOWN_ERROR");
}

template <>
StructValueDescriptor<ccp::ClientCertificateInfo>::Description
StructValueDescriptor<ccp::ClientCertificateInfo>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // bridge-backend.js.
  return Describe("chrome_certificate_provider::ClientCertificateInfo")
      .WithField(&ccp::ClientCertificateInfo::certificate, "certificate")
      .WithField(&ccp::ClientCertificateInfo::supported_algorithms,
                 "supportedAlgorithms");
}

// static
template <>
constexpr const char* ClientCertificateInfoConverter::GetStructTypeName() {
  return "chrome_certificate_provider::ClientCertificateInfo";
}

// static
template <>
template <typename Callback>
void ClientCertificateInfoConverter::VisitFields(
    const ccp::ClientCertificateInfo& value, Callback callback) {
  callback(&value.certificate, "certificate");
  callback(&value.supported_algorithms, "supportedAlgorithms");
}

template <>
StructValueDescriptor<ccp::SetCertificatesDetails>::Description
StructValueDescriptor<ccp::SetCertificatesDetails>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::SetCertificatesDetails")
      .WithField(&ccp::SetCertificatesDetails::certificates_request_id,
                 "certificatesRequestId")
      .WithField(&ccp::SetCertificatesDetails::error, "error")
      .WithField(&ccp::SetCertificatesDetails::client_certificates,
                 "clientCertificates");
}

// static
template <>
constexpr const char* SetCertificatesDetailsConverter::GetStructTypeName() {
  return "chrome_certificate_provider::SetCertificatesDetails";
}

// static
template <>
template <typename Callback>
void SetCertificatesDetailsConverter::VisitFields(
    const ccp::SetCertificatesDetails& value, Callback callback) {
  callback(&value.certificates_request_id, "certificatesRequestId");
  callback(&value.error, "error");
  callback(&value.client_certificates, "clientCertificates");
}

template <>
StructValueDescriptor<ccp::SignatureRequest>::Description
StructValueDescriptor<ccp::SignatureRequest>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::SignatureRequest")
      .WithField(&ccp::SignatureRequest::sign_request_id, "signRequestId")
      .WithField(&ccp::SignatureRequest::input, "input")
      .WithField(&ccp::SignatureRequest::digest, "digest")
      .WithField(&ccp::SignatureRequest::algorithm, "algorithm")
      .WithField(&ccp::SignatureRequest::certificate, "certificate");
}

// static
template <>
constexpr const char* SignatureRequestConverter::GetStructTypeName() {
  return "chrome_certificate_provider::SignatureRequest";
}

// static
template <>
template <typename Callback>
void SignatureRequestConverter::VisitFields(const ccp::SignatureRequest& value,
                                            Callback callback) {
  callback(&value.sign_request_id, "signRequestId");
  callback(&value.input, "input");
  callback(&value.digest, "digest");
  callback(&value.algorithm, "algorithm");
  callback(&value.certificate, "certificate");
}

template <>
StructValueDescriptor<ccp::RequestPinOptions>::Description
StructValueDescriptor<ccp::RequestPinOptions>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::RequestPinOptions")
      .WithField(&ccp::RequestPinOptions::sign_request_id, "signRequestId")
      .WithField(&ccp::RequestPinOptions::request_type, "requestType")
      .WithField(&ccp::RequestPinOptions::error_type, "errorType")
      .WithField(&ccp::RequestPinOptions::attempts_left, "attemptsLeft");
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

template <>
StructValueDescriptor<ccp::RequestPinResults>::Description
StructValueDescriptor<ccp::RequestPinResults>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::RequestPinResults")
      .WithField(&ccp::RequestPinResults::user_input, "userInput");
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

template <>
StructValueDescriptor<ccp::StopPinRequestOptions>::Description
StructValueDescriptor<ccp::StopPinRequestOptions>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::StopPinRequestOptions")
      .WithField(&ccp::StopPinRequestOptions::sign_request_id, "signRequestId")
      .WithField(&ccp::StopPinRequestOptions::error_type, "errorType");
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

bool VarAs(const pp::Var& var, Algorithm* result, std::string* error_message) {
  return gsc::AlgorithmConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(Algorithm value) {
  return gsc::AlgorithmConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, Error* result, std::string* error_message) {
  return gsc::ErrorConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(Error value) {
  return gsc::ErrorConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, PinRequestType* result,
           std::string* error_message) {
  return gsc::PinRequestTypeConverter::ConvertFromVar(var, result,
                                                      error_message);
}

pp::Var MakeVar(PinRequestType value) {
  return gsc::PinRequestTypeConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, PinRequestErrorType* result,
           std::string* error_message) {
  return gsc::PinRequestErrorTypeConverter::ConvertFromVar(var, result,
                                                           error_message);
}

pp::Var MakeVar(PinRequestErrorType value) {
  return gsc::PinRequestErrorTypeConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, ClientCertificateInfo* result,
           std::string* error_message) {
  return gsc::ClientCertificateInfoConverter::ConvertFromVar(var, result,
                                                             error_message);
}

pp::Var MakeVar(const ClientCertificateInfo& value) {
  return gsc::ClientCertificateInfoConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, SetCertificatesDetails* result,
           std::string* error_message) {
  return gsc::SetCertificatesDetailsConverter::ConvertFromVar(var, result,
                                                              error_message);
}

pp::Var MakeVar(const SetCertificatesDetails& value) {
  return gsc::SetCertificatesDetailsConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, SignatureRequest* result,
           std::string* error_message) {
  return gsc::SignatureRequestConverter::ConvertFromVar(var, result,
                                                        error_message);
}

pp::Var MakeVar(const SignatureRequest& value) {
  return gsc::SignatureRequestConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, RequestPinOptions* result,
           std::string* error_message) {
  return gsc::RequestPinOptionsConverter::ConvertFromVar(var, result,
                                                         error_message);
}

pp::Var MakeVar(const RequestPinOptions& value) {
  return gsc::RequestPinOptionsConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, RequestPinResults* result,
           std::string* error_message) {
  return gsc::RequestPinResultsConverter::ConvertFromVar(var, result,
                                                         error_message);
}

pp::Var MakeVar(const RequestPinResults& value) {
  return gsc::RequestPinResultsConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, StopPinRequestOptions* result,
           std::string* error_message) {
  return gsc::StopPinRequestOptionsConverter::ConvertFromVar(var, result,
                                                             error_message);
}

pp::Var MakeVar(const StopPinRequestOptions& value) {
  return gsc::StopPinRequestOptionsConverter::ConvertToVar(value);
}

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client
