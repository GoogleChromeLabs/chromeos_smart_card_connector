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

// This file contains definitions of the C++ analogs for the
// chrome.certificateProvider API types. For the chrome.certificateProvider API
// documentation, please refer to:
// <https://developer.chrome.com/extensions/certificateProvider>.
//
// Also there are function overloads defined that perform the conversion between
// values of these types and Pepper values (which correspond to the JavaScript
// values used with chrome.usb API).

#ifndef SMART_CARD_CLIENT_CHROME_CERTIFICATE_PROVIDER_TYPES_H_
#define SMART_CARD_CLIENT_CHROME_CERTIFICATE_PROVIDER_TYPES_H_

#include <stdint.h>

#include <string>
#include <vector>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

namespace smart_card_client {

namespace chrome_certificate_provider {

// Enumerate that corresponds to different hash algorithms.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#type-Algorithm>.
enum class Algorithm {
  kRsassaPkcs1v15Md5Sha1,
  kRsassaPkcs1v15Sha1,
  kRsassaPkcs1v15Sha384,
  kRsassaPkcs1v15Sha256,
  kRsassaPkcs1v15Sha512,
};

// Enumerate that corresponds to types of errors that the extension can report.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#type-Error>.
enum class Error {
  kGeneral,
};

// Enumerate that corresponds to the type of the PIN dialog.
//
// For the corresponding original JavaScript definition, refer to the
// "requestType" parameter definition:
// <https://developer.chrome.com/extensions/certificateProvider#method-requestPin>.
enum class PinRequestType {
  kPin,
  kPuk,
};

// Enumerate that corresponds to an error that has to be displayed in the PIN
// dialog.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#type-PinRequestErrorType>.
enum class PinRequestErrorType {
  kInvalidPin,
  kInvalidPuk,
  kMaxAttemptsExceeded,
  kUnknownError,
};

// Structure containing a certificate description.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#type-ClientCertificateInfo>.
struct ClientCertificateInfo {
  std::vector<uint8_t> certificate;
  std::vector<Algorithm> supported_algorithms;
};

// Structure containing the parameters for the
// chrome.certificateProvider.setCertificates() function.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#method-setCertificates>.
struct SetCertificatesDetails {
  google_smart_card::optional<int> certificates_request_id;
  google_smart_card::optional<Error> error;
  std::vector<ClientCertificateInfo> client_certificates;
};

// Structure containing the data of the signature request.
//
// Note that either |input| or |digest| will be set (but not both
// simultaneously).
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#event-onSignatureRequested>
// and
// <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>.
struct SignatureRequest {
  int sign_request_id;
  std::vector<uint8_t> input;
  std::vector<uint8_t> digest;  // only used with the legacy API
  Algorithm algorithm;
  std::vector<uint8_t> certificate;
};

// Structure containing the parameters for the
// chrome.certificateProvider.requestPin() function.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#method-requestPin>.
struct RequestPinOptions {
  int sign_request_id;
  google_smart_card::optional<PinRequestType> request_type;
  google_smart_card::optional<PinRequestErrorType> error_type;
  google_smart_card::optional<int> attempts_left;
};

// Structure containing the results returned from the
// chrome.certificateProvider.requestPin() function call.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#method-requestPin>.
struct RequestPinResults {
  google_smart_card::optional<std::string> user_input;
};

// Structure containing the parameters for the
// chrome.certificateProvider.stopPinRequest() function.
//
// For the corresponding original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#method-stopPinRequest>.
struct StopPinRequestOptions {
  int sign_request_id;
  google_smart_card::optional<PinRequestErrorType> error_type;
};

bool VarAs(const pp::Var& var, Algorithm* result, std::string* error_message);
pp::Var MakeVar(Algorithm value);

bool VarAs(const pp::Var& var, Error* result, std::string* error_message);
pp::Var MakeVar(Error value);

bool VarAs(const pp::Var& var, PinRequestType* result,
           std::string* error_message);
pp::Var MakeVar(PinRequestType value);

bool VarAs(const pp::Var& var, PinRequestErrorType* result,
           std::string* error_message);
pp::Var MakeVar(PinRequestErrorType value);

bool VarAs(const pp::Var& var, ClientCertificateInfo* result,
           std::string* error_message);
pp::Var MakeVar(const ClientCertificateInfo& value);

bool VarAs(const pp::Var& var, SetCertificatesDetails* result,
           std::string* error_message);
pp::Var MakeVar(const SetCertificatesDetails& value);

bool VarAs(const pp::Var& var, SignatureRequest* result,
           std::string* error_message);
pp::Var MakeVar(const SignatureRequest& value);

bool VarAs(const pp::Var& var, RequestPinOptions* result,
           std::string* error_message);
pp::Var MakeVar(const RequestPinOptions& value);

bool VarAs(const pp::Var& var, RequestPinResults* result,
           std::string* error_message);
pp::Var MakeVar(const RequestPinResults& value);

bool VarAs(const pp::Var& var, StopPinRequestOptions* result,
           std::string* error_message);
pp::Var MakeVar(const StopPinRequestOptions& value);

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client

#endif  // SMART_CARD_CLIENT_CHROME_CERTIFICATE_PROVIDER_TYPES_H_
