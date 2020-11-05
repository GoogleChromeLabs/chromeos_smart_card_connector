// Copyright 2020 Google Inc. All Rights Reserved.
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

#include <stdint.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/requesting/request_receiver.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_integration_testing/integration_test_helper.h>
#include <google_smart_card_integration_testing/integration_test_service.h>

#include "chrome_certificate_provider/api_bridge.h"
#include "chrome_certificate_provider/types.h"

namespace gsc = google_smart_card;

namespace smart_card_client {
namespace chrome_certificate_provider {

namespace {

constexpr uint8_t kFakeCert1Der[] = {1, 2, 3};
constexpr Algorithm kFakeCert1Algorithms[] = {Algorithm::kRsassaPkcs1v15Sha256};
constexpr uint8_t kFakeCert2Der[] = {4};
constexpr Algorithm kFakeCert2Algorithms[] = {Algorithm::kRsassaPkcs1v15Sha512,
                                              Algorithm::kRsassaPkcs1v15Sha1};

ClientCertificateInfo GetFakeCert1() {
  ClientCertificateInfo info;
  info.certificate.assign(std::begin(kFakeCert1Der), std::end(kFakeCert1Der));
  info.supported_algorithms.assign(std::begin(kFakeCert1Algorithms),
                                   std::end(kFakeCert1Algorithms));
  return info;
}

ClientCertificateInfo GetFakeCert2() {
  ClientCertificateInfo info;
  info.certificate.assign(std::begin(kFakeCert2Der), std::end(kFakeCert2Der));
  info.supported_algorithms.assign(std::begin(kFakeCert2Algorithms),
                                   std::end(kFakeCert2Algorithms));
  return info;
}

void SetCertificatesOnBackgroundThread(
    std::weak_ptr<ApiBridge> api_bridge,
    const std::vector<ClientCertificateInfo>& certificates,
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::shared_ptr<ApiBridge> locked_api_bridge = api_bridge.lock();
  if (!locked_api_bridge)
    GOOGLE_SMART_CARD_LOG_FATAL << "ApiBridge already destroyed";
  locked_api_bridge->SetCertificates(certificates);
  result_callback(gsc::GenericRequestResult::CreateSuccessful(gsc::Value()));
}

}  // namespace

class ApiBridgeIntegrationTestHelper final : public gsc::IntegrationTestHelper {
 public:
  // IntegrationTestHelper:
  std::string GetName() const override;
  void SetUp(gsc::GlobalContext* global_context,
             pp::Instance* pp_instance,
             pp::Core* pp_core,
             gsc::TypedMessageRouter* typed_message_router,
             const pp::Var& data) override;
  void TearDown() override;
  void OnMessageFromJs(
      const pp::Var& data,
      gsc::RequestReceiver::ResultCallback result_callback) override;

 private:
  void ScheduleSetCertificatesCall(
      const std::vector<ClientCertificateInfo>& certificates,
      gsc::RequestReceiver::ResultCallback result_callback);

  std::shared_ptr<ApiBridge> api_bridge_;
};

const auto g_api_bridge_integration_test_helper =
    gsc::IntegrationTestService::RegisterHelper(
        gsc::MakeUnique<ApiBridgeIntegrationTestHelper>());

std::string ApiBridgeIntegrationTestHelper::GetName() const {
  return "ChromeCertificateProviderApiBridge";
}

void ApiBridgeIntegrationTestHelper::SetUp(
    gsc::GlobalContext* global_context,
    pp::Instance* pp_instance,
    pp::Core* /*pp_core*/,
    gsc::TypedMessageRouter* typed_message_router,
    const pp::Var& /*data*/) {
  api_bridge_ = std::make_shared<ApiBridge>(global_context,
                                            typed_message_router, pp_instance,
                                            /*request_handling_mutex=*/nullptr);
}

void ApiBridgeIntegrationTestHelper::TearDown() {
  api_bridge_->Detach();
  api_bridge_.reset();
}

void ApiBridgeIntegrationTestHelper::OnMessageFromJs(
    const pp::Var& data,
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::string command;
  std::string error_message;
  if (!gsc::VarAs(data, &command, &error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected message " << gsc::DumpVar(data);
  if (command == "setCertificates_empty") {
    ScheduleSetCertificatesCall(/*certificates=*/{}, result_callback);
  } else if (command == "setCertificates_fakeCerts") {
    ScheduleSetCertificatesCall({GetFakeCert1(), GetFakeCert2()},
                                result_callback);
  } else {
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown command " << command;
  }
}

void ApiBridgeIntegrationTestHelper::ScheduleSetCertificatesCall(
    const std::vector<ClientCertificateInfo>& certificates,
    gsc::RequestReceiver::ResultCallback result_callback) {
  // Post to a background thread, since the main thread isn't allowed to perform
  // blocking calls, which SetCertificates() is.
  std::thread(&SetCertificatesOnBackgroundThread, api_bridge_, certificates,
              result_callback)
      .detach();
}

}  // namespace chrome_certificate_provider
}  // namespace smart_card_client
