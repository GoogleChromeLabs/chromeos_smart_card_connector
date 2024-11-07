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

#include "common/cpp/src/public/value_conversion.h"

namespace ccp = smart_card_client::chrome_certificate_provider;

namespace google_smart_card {

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
                "RSASSA_PKCS1_v1_5_SHA512")
      .WithItem(ccp::Algorithm::kRsassaPssSha256, "RSASSA_PSS_SHA256")
      .WithItem(ccp::Algorithm::kRsassaPssSha384, "RSASSA_PSS_SHA384")
      .WithItem(ccp::Algorithm::kRsassaPssSha512, "RSASSA_PSS_SHA512");
}

template <>
EnumValueDescriptor<ccp::Error>::Description
EnumValueDescriptor<ccp::Error>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::Error")
      .WithItem(ccp::Error::kGeneral, "GENERAL_ERROR");
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

template <>
StructValueDescriptor<ccp::SignatureRequest>::Description
StructValueDescriptor<ccp::SignatureRequest>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::SignatureRequest")
      .WithField(&ccp::SignatureRequest::sign_request_id, "signRequestId")
      .WithField(&ccp::SignatureRequest::input, "input")
      .WithField(&ccp::SignatureRequest::algorithm, "algorithm")
      .WithField(&ccp::SignatureRequest::certificate, "certificate");
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

template <>
StructValueDescriptor<ccp::RequestPinResults>::Description
StructValueDescriptor<ccp::RequestPinResults>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.certificateProvider API.
  return Describe("chrome_certificate_provider::RequestPinResults")
      .WithField(&ccp::RequestPinResults::user_input, "userInput");
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

}  // namespace google_smart_card
