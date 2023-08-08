// Copyright 2022 Google Inc. All Rights Reserved.
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

#include "smart_card_connector_app/src/testing_smart_card_simulation.h"

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <google_smart_card_common/logging/hex_dumping.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_debug_dumping.h>

#include "common/cpp/src/public/value_builder.h"
#include "third_party/libusb/webport/src/libusb_js_proxy_data_model.h"

namespace google_smart_card {

using CardProfile = TestingSmartCardSimulation::CardProfile;
using CardType = TestingSmartCardSimulation::CardType;
using CcidIccStatus = TestingSmartCardSimulation::CcidIccStatus;
using DeviceType = TestingSmartCardSimulation::DeviceType;

const char* TestingSmartCardSimulation::kRequesterName = "libusb";

namespace {

uint8_t CalculateXor(const std::vector<uint8_t>& bytes) {
  uint8_t xor_value = 0;
  for (uint8_t byte : bytes)
    xor_value ^= byte;
  return xor_value;
}

LibusbJsDevice MakeLibusbJsDevice(
    const TestingSmartCardSimulation::Device& device) {
  LibusbJsDevice js_device;
  js_device.device_id = device.id;
  switch (device.type) {
    case DeviceType::kGemaltoPcTwinReader:
      // Numbers are for a real device.
      js_device.vendor_id = 0x08E6;
      js_device.product_id = 0x3437;
      js_device.version = 0x200;
      // TODO: Remove explicit `std::string` construction after switching to
      // feature-complete `std::optional` (after dropping NaCl support).
      js_device.product_name = std::string("USB SmartCard Reader");
      js_device.manufacturer_name = std::string("Gemalto");
      js_device.serial_number = std::string("E00E0000");  // redacted
      return js_device;
    case DeviceType::kDellSmartCardReaderKeyboard:
      // Numbers are for a real device.
      js_device.vendor_id = 0x413c;
      js_device.product_id = 0x2101;
      js_device.version = 0x201;
      // TODO: Remove explicit `std::string` construction after switching to
      // feature-complete `std::optional` (after dropping NaCl support).
      js_device.product_name = std::string("Dell Smart Card Reader Keyboard");
      js_device.manufacturer_name = std::string("Dell");
      js_device.serial_number = std::string("");
      return js_device;
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

std::vector<LibusbJsConfigurationDescriptor>
MakeLibusbJsConfigurationDescriptors(DeviceType device_type) {
  switch (device_type) {
    case DeviceType::kGemaltoPcTwinReader: {
      // Values are taken from a real device.

      LibusbJsEndpointDescriptor endpoint1;
      endpoint1.endpoint_address = 1;
      endpoint1.direction = LibusbJsDirection::kOut;
      endpoint1.type = LibusbJsEndpointType::kBulk;
      endpoint1.max_packet_size = 64;

      LibusbJsEndpointDescriptor endpoint2;
      endpoint2.endpoint_address = 0x82;
      endpoint2.direction = LibusbJsDirection::kIn;
      endpoint2.type = LibusbJsEndpointType::kBulk;
      endpoint2.max_packet_size = 64;

      LibusbJsEndpointDescriptor endpoint3;
      endpoint3.endpoint_address = 0x83;
      endpoint3.direction = LibusbJsDirection::kIn;
      endpoint3.type = LibusbJsEndpointType::kInterrupt;
      endpoint3.max_packet_size = 8;

      LibusbJsInterfaceDescriptor interface;
      interface.interface_number = 0;
      interface.interface_class = 0xB;
      interface.interface_subclass = 0;
      interface.interface_protocol = 0;
      interface.extra_data = std::vector<uint8_t>(
          {0x36, 0x21, 0x01, 0x01, 0x00, 0x07, 0x03, 0x00, 0x00, 0x00, 0xC0,
           0x12, 0x00, 0x00, 0xC0, 0x12, 0x00, 0x00, 0x00, 0x67, 0x32, 0x00,
           0x00, 0xCE, 0x99, 0x0C, 0x00, 0x35, 0xFE, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x01, 0x00,
           0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01});
      interface.endpoints.push_back(std::move(endpoint1));
      interface.endpoints.push_back(std::move(endpoint2));
      interface.endpoints.push_back(std::move(endpoint3));

      LibusbJsConfigurationDescriptor config;
      config.active = true;
      config.configuration_value = 1;
      config.interfaces.push_back(std::move(interface));

      return {config};
    }

    case DeviceType::kDellSmartCardReaderKeyboard: {
      // Values are taken from a real device.

      LibusbJsEndpointDescriptor endpoint1;
      endpoint1.endpoint_address = 1;
      endpoint1.direction = LibusbJsDirection::kOut;
      endpoint1.type = LibusbJsEndpointType::kBulk;
      endpoint1.max_packet_size = 64;

      LibusbJsEndpointDescriptor endpoint2;
      endpoint2.endpoint_address = 0x82;
      endpoint2.direction = LibusbJsDirection::kIn;
      endpoint2.type = LibusbJsEndpointType::kBulk;
      endpoint2.max_packet_size = 64;

      LibusbJsEndpointDescriptor endpoint3;
      endpoint3.endpoint_address = 0x83;
      endpoint3.direction = LibusbJsDirection::kIn;
      endpoint3.type = LibusbJsEndpointType::kInterrupt;
      endpoint3.max_packet_size = 8;

      // Note: in reality the device has another USB interface (with the number
      // "0" and the "Human Interface Device" class), but it's normally filtered
      // out by the JavaScript counterpart before reaching the C++ code.
      LibusbJsInterfaceDescriptor interface;
      interface.interface_number = 1;
      interface.interface_class = 0xB;
      interface.interface_subclass = 0;
      interface.interface_protocol = 0;
      interface.extra_data = std::vector<uint8_t>(
          {0x36, 0x21, 0x01, 0x01, 0x00, 0x07, 0x03, 0x00, 0x00, 0x00, 0xC0,
           0x12, 0x00, 0x00, 0xC0, 0x12, 0x00, 0x00, 0x00, 0x67, 0x32, 0x00,
           0x00, 0xCE, 0x99, 0x0C, 0x00, 0x35, 0xFE, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x01, 0x00,
           0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01});
      interface.endpoints.push_back(std::move(endpoint1));
      interface.endpoints.push_back(std::move(endpoint2));
      interface.endpoints.push_back(std::move(endpoint3));

      LibusbJsConfigurationDescriptor config;
      config.active = true;
      config.configuration_value = 1;
      config.interfaces.push_back(std::move(interface));

      return {config};
    }
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

bool DeviceInterfaceExists(DeviceType device_type, int64_t interface_number) {
  for (const auto& config : MakeLibusbJsConfigurationDescriptors(device_type)) {
    for (const auto& interface : config.interfaces) {
      if (interface.interface_number == interface_number)
        return true;
    }
  }
  return false;
}

bool DeviceEndpointExists(DeviceType device_type, uint8_t endpoint_address) {
  for (const auto& config : MakeLibusbJsConfigurationDescriptors(device_type)) {
    for (const auto& interface : config.interfaces) {
      for (const auto& endpoint : interface.endpoints) {
        if (endpoint.endpoint_address == endpoint_address)
          return true;
      }
    }
  }
  return false;
}

// Builds a fake response to the GET_DATA_RATES control transfer.
std::vector<uint8_t> MakeGetDataRatesResponse(DeviceType device_type) {
  switch (device_type) {
    case DeviceType::kGemaltoPcTwinReader: {
      // Values are taken from a real device.
      return {0x67, 0x32, 0x00, 0x00, 0xCE, 0x64, 0x00, 0x00, 0x9D, 0xC9, 0x00,
              0x00, 0x3A, 0x93, 0x01, 0x00, 0x74, 0x26, 0x03, 0x00, 0xE7, 0x4C,
              0x06, 0x00, 0xCE, 0x99, 0x0C, 0x00, 0xD7, 0x5C, 0x02, 0x00, 0x11,
              0xF0, 0x03, 0x00, 0x34, 0x43, 0x00, 0x00, 0x69, 0x86, 0x00, 0x00,
              0xD1, 0x0C, 0x01, 0x00, 0xA2, 0x19, 0x02, 0x00, 0x45, 0x33, 0x04,
              0x00, 0x8A, 0x66, 0x08, 0x00, 0x0B, 0xA0, 0x02, 0x00, 0x73, 0x30,
              0x00, 0x00, 0xE6, 0x60, 0x00, 0x00, 0xCC, 0xC1, 0x00, 0x00, 0x99,
              0x83, 0x01, 0x00, 0x32, 0x07, 0x03, 0x00, 0x63, 0x0E, 0x06, 0x00,
              0xB3, 0x22, 0x01, 0x00, 0x7F, 0xE4, 0x01, 0x00, 0x06, 0x50, 0x01,
              0x00, 0x36, 0x97, 0x00, 0x00, 0x04, 0xFC, 0x00, 0x00, 0x53, 0x28,
              0x00, 0x00, 0xA5, 0x50, 0x00, 0x00, 0x4A, 0xA1, 0x00, 0x00, 0x95,
              0x42, 0x01, 0x00, 0x29, 0x85, 0x02, 0x00, 0xF8, 0x78, 0x00, 0x00,
              0x3E, 0x49, 0x00, 0x00, 0x7C, 0x92, 0x00, 0x00, 0xF8, 0x24, 0x01,
              0x00, 0xF0, 0x49, 0x02, 0x00, 0xE0, 0x93, 0x04, 0x00, 0xC0, 0x27,
              0x09, 0x00, 0x74, 0xB7, 0x01, 0x00, 0x6C, 0xDC, 0x02, 0x00, 0xD4,
              0x30, 0x00, 0x00, 0xA8, 0x61, 0x00, 0x00, 0x50, 0xC3, 0x00, 0x00,
              0xA0, 0x86, 0x01, 0x00, 0x40, 0x0D, 0x03, 0x00, 0x80, 0x1A, 0x06,
              0x00, 0x48, 0xE8, 0x01, 0x00, 0xBA, 0xDB, 0x00, 0x00, 0x36, 0x6E,
              0x01, 0x00, 0x24, 0xF4, 0x00, 0x00, 0xDD, 0x6D, 0x00, 0x00, 0x1B,
              0xB7, 0x00, 0x00};
    }
    case DeviceType::kDellSmartCardReaderKeyboard: {
      // Values are taken from a real device.
      return {0x67, 0x32, 0x00, 0x00, 0xCE, 0x64, 0x00, 0x00, 0x9D, 0xC9, 0x00,
              0x00, 0x3A, 0x93, 0x01, 0x00, 0x74, 0x26, 0x03, 0x00, 0xE7, 0x4C,
              0x06, 0x00, 0xCE, 0x99, 0x0C, 0x00, 0xD7, 0x5C, 0x02, 0x00, 0x11,
              0xF0, 0x03, 0x00, 0x34, 0x43, 0x00, 0x00, 0x69, 0x86, 0x00, 0x00,
              0xD1, 0x0C, 0x01, 0x00, 0xA2, 0x19, 0x02, 0x00, 0x45, 0x33, 0x04,
              0x00, 0x8A, 0x66, 0x08, 0x00, 0x0B, 0xA0, 0x02, 0x00, 0x73, 0x30,
              0x00, 0x00, 0xE6, 0x60, 0x00, 0x00, 0xCC, 0xC1, 0x00, 0x00, 0x99,
              0x83, 0x01, 0x00, 0x32, 0x07, 0x03, 0x00, 0x63, 0x0E, 0x06, 0x00,
              0xB3, 0x22, 0x01, 0x00, 0x7F, 0xE4, 0x01, 0x00, 0x06, 0x50, 0x01,
              0x00, 0x36, 0x97, 0x00, 0x00, 0x04, 0xFC, 0x00, 0x00, 0x53, 0x28,
              0x00, 0x00, 0xA5, 0x50, 0x00, 0x00, 0x4A, 0xA1, 0x00, 0x00, 0x95,
              0x42, 0x01, 0x00, 0x29, 0x85, 0x02, 0x00, 0xF8, 0x78, 0x00, 0x00,
              0x3E, 0x49, 0x00, 0x00, 0x7C, 0x92, 0x00, 0x00, 0xF8, 0x24, 0x01,
              0x00, 0xF0, 0x49, 0x02, 0x00, 0xE0, 0x93, 0x04, 0x00, 0xC0, 0x27,
              0x09, 0x00, 0x74, 0xB7, 0x01, 0x00, 0x6C, 0xDC, 0x02, 0x00, 0xD4,
              0x30, 0x00, 0x00, 0xA8, 0x61, 0x00, 0x00, 0x50, 0xC3, 0x00, 0x00,
              0xA0, 0x86, 0x01, 0x00, 0x40, 0x0D, 0x03, 0x00, 0x80, 0x1A, 0x06,
              0x00, 0x48, 0xE8, 0x01, 0x00, 0xBA, 0xDB, 0x00, 0x00, 0x36, 0x6E,
              0x01, 0x00, 0x24, 0xF4, 0x00, 0x00, 0xDD, 0x6D, 0x00, 0x00, 0x1B,
              0xB7, 0x00, 0x00};
    }
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

// Builds a fake RDR_to_PC_SlotStatus message.
std::vector<uint8_t> MakeSlotStatusTransferReply(uint8_t sequence_number,
                                                 CcidIccStatus icc_status) {
  // The message format is per CCID specs.
  return {0x81,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          sequence_number,
          static_cast<uint8_t>(icc_status),
          0x00,
          0x00};
}

// Builds a fake RDR_to_PC_Escape message.
std::vector<uint8_t> MakeEscapeTransferReply(uint8_t sequence_number,
                                             CcidIccStatus icc_status) {
  // The message format is per CCID specs.
  // Currently, the status always says "escape command failed" and "no ICC
  // present".
  const uint8_t kStatusFailed = 0x40;
  const uint8_t status = kStatusFailed + static_cast<uint8_t>(icc_status);
  return {0x83,   0x00, 0x00, 0x00, 0x00, 0x00, sequence_number,
          status, 0x0A, 0x00};
}

// Builds a fake RDR_to_PC_DataBlock message.
std::vector<uint8_t> MakeDataBlockTransferReply(
    uint8_t sequence_number,
    CcidIccStatus icc_status,
    const std::vector<uint8_t>& data) {
  // The code below that encodes the data length only supports single-byte
  // length at the moment, for simplicity.
  GOOGLE_SMART_CARD_CHECK(data.size() < 256);
  // The message format is per CCID specs.
  std::vector<uint8_t> transfer_reply = {0x80,
                                         (uint8_t)data.size(),
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x00,
                                         sequence_number,
                                         static_cast<uint8_t>(icc_status),
                                         0x00,
                                         0x00};
  transfer_reply.insert(transfer_reply.end(), data.begin(), data.end());
  return transfer_reply;
}

// Builds a fake RDR_to_PC_DataBlock message for replying to
// PC_to_RDR_IccPowerOn.
std::vector<uint8_t> MakePowerOnTransferReply(uint8_t sequence_number,
                                              CcidIccStatus icc_status,
                                              optional<CardType> card_type) {
  std::vector<uint8_t> response_data;
  if (card_type)
    response_data = TestingSmartCardSimulation::GetCardAtr(*card_type);
  return MakeDataBlockTransferReply(sequence_number, icc_status, response_data);
}

// Builds a fake reply to the "SELECT" command APDU sent to the card. `p1`,
// `p2`, `le` and `command_data` refer to the fields defined in ISO/IEC 7816-3.
std::vector<uint8_t> HandleSelectCommandApdu(
    CardProfile card_profile,
    uint8_t p1,
    uint8_t p2,
    uint8_t le,
    const std::vector<uint8_t>& /*command_data*/) {
  switch (card_profile) {
    case CardProfile::kCharismathicsPiv: {
      // Verify PIV command parameters per NIST 800-73-4. For now we don't check
      // the command data, for simplicity.
      GOOGLE_SMART_CARD_CHECK(p1 == 0x04);
      GOOGLE_SMART_CARD_CHECK(p2 == 0x00);
      GOOGLE_SMART_CARD_CHECK(le == 0x00);
      // Reply with the application identifier, followed by the status bytes
      // that denote success: SW1=0x90, SW2=0x00.
      std::vector<uint8_t> apdu_response =
          TestingSmartCardSimulation::GetCardProfileApplicationIdentifier(
              card_profile);
      const uint8_t sw1 = 0x90;
      const uint8_t sw2 = 0x00;
      apdu_response.push_back(sw1);
      apdu_response.push_back(sw2);
      return apdu_response;
    }
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

// Builds a fake reply to the APDU (application protocol data unit) sent to the
// smart card applet.
std::vector<uint8_t> HandleApdu(CardProfile card_profile,
                                const std::vector<uint8_t>& apdu) {
  // The command, per ISO/IEC 7816-3, starts with a header in the following
  // format: CLA INS P1 P2.
  // * CLA: class byte.
  // * INS: instruction byte.
  // * P1 and P2: parameter bytes.
  GOOGLE_SMART_CARD_CHECK(apdu.size() >= 4);
  const uint8_t cla = apdu[0];
  const uint8_t ins = apdu[1];
  const uint8_t p1 = apdu[2];
  const uint8_t p2 = apdu[3];
  // The header is followed by the command body. It generally has the following
  // format: Lc, Data, Le.
  // * Lc: one byte; optional; denotes the request data size.
  // * Data: a sequence of Lc bytes.
  // * Le: one byte; optional; denotes the maximum expected response size.
  // All fields are optional, hence we need to disambiguate between cases "the
  // body starts from Lc" and "the body starts from Le". Per specs, we do that
  // based on the body length.
  // We only support single-byte Lc/Le values currently, for simplicity.
  uint8_t le;
  std::vector<uint8_t> data;
  if (apdu.size() == 4) {
    le = 0;
  } else if (apdu.size() == 5) {
    le = apdu.back();
  } else {
    uint8_t lc = apdu[4];
    GOOGLE_SMART_CARD_CHECK(lc + 5 <= apdu.size());
    data.assign(apdu.begin() + 5, apdu.begin() + 5 + lc);
    if (lc + 5 == apdu.size())
      le = 0;
    else if (lc + 6 == apdu.size())
      le = apdu.back();
    else
      GOOGLE_SMART_CARD_LOG_FATAL << "Failed to parse request command: "
                                  << HexDumpBytes(apdu);
  }
  // Determine the requested command. This is mostly following definitions in
  // ISO/IEC 7816-4, although particular profiles might have some differences.
  if (cla == 0x00 && ins == 0xA4) {
    // It's a "SELECT" command.
    return HandleSelectCommandApdu(card_profile, p1, p2, le, data);
  }
  GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected APDU: " << HexDumpBytes(apdu);
}

// Builds a fake RDR_to_PC_DataBlock message for replying to PC_to_RDR_XfrBlock.
std::vector<uint8_t> MakeXfrBlockTransferReply(
    uint8_t sequence_number,
    CcidIccStatus icc_status,
    optional<CardType> card_type,
    optional<CardProfile> card_profile,
    const std::vector<uint8_t>& request_data) {
  // The protocol details hardcoded in this function are based on ISO/IEC
  // 7816-3.
  GOOGLE_SMART_CARD_CHECK(card_type);
  GOOGLE_SMART_CARD_CHECK(!request_data.empty());
  if (request_data[0] == 0xFF) {
    // It's a PPS ("protocol and parameters selection") request.
    // For now we only support a particular (hardcoded) request.
    GOOGLE_SMART_CARD_CHECK(request_data ==
                            std::vector<uint8_t>({0xFF, 0x11, 0x96, 0x78}));
    // A successful PPS response is (commonly) equal to the request.
    const auto response_data = request_data;
    switch (*card_type) {
      case CardType::kCosmoId70:
        return MakeDataBlockTransferReply(sequence_number, icc_status,
                                          response_data);
    }
    GOOGLE_SMART_CARD_NOTREACHED;
  }
  if (request_data.size() >= 2 && request_data[0] == 0x00 &&
      request_data[1] == 0xC1) {
    // It's an IFS (maximum information field size) request: how many bytes can
    // the reader receive from the card at once.
    // For now we only support a particular (hardcoded) request.
    GOOGLE_SMART_CARD_CHECK(
        request_data == std::vector<uint8_t>({0x00, 0xC1, 0x01, 0xFE, 0x3E}));
    // A successful IFS response only differs from the request by setting the
    // 0x20 bit in the PCB (protocol control byte).
    auto response_data = request_data;
    response_data[1] |= 0x20;
    // Adjust the epilogue byte (a XOR checksum of other bytes) accordingly.
    response_data[4] ^= 0x20;
    switch (*card_type) {
      case CardType::kCosmoId70:
        return MakeDataBlockTransferReply(sequence_number, icc_status,
                                          response_data);
    }
    GOOGLE_SMART_CARD_NOTREACHED;
  }
  if (card_profile && request_data.size() >= 3 && request_data[0] == 0x00 &&
      request_data[1] == 0x00) {
    // It's a command block in the T=1 protocol.
    // The format according to the specs: NAD, PCB, LEN, INF, epilogue.
    // * NAD (node address byte): 1 byte; we assume it to be 0;
    // * PCB (protocol control byte): 1 byte; we assume it to be 0;
    // * LEN: 1 byte;
    // * INF: a sequence of LEN bytes;
    // * epilogue: we assume it to be a 1-byte LRC (longitudinal redundancy
    //   code), which is a XOR of all other bytes of the block.
    // Sanity-check the "LEN" field value.
    uint8_t information_length = request_data[2];
    GOOGLE_SMART_CARD_CHECK(information_length + 4 == request_data.size());
    // Extract the "INF" blob that contains the actual APDU of the command.
    const std::vector<uint8_t> apdu(request_data.begin() + 3,
                                    request_data.end() - 1);
    // Simulate the reply from the applet on the card.
    const std::vector<uint8_t> apdu_reply = HandleApdu(*card_profile, apdu);
    // Construct the reply data, which has the same format as the request. For
    // simplicity, we assume it to fit a single block.
    GOOGLE_SMART_CARD_CHECK(apdu_reply.size() <= 255);
    std::vector<uint8_t> response_data = {
        0x00, 0x00, static_cast<uint8_t>(apdu_reply.size())};
    response_data.insert(response_data.end(), apdu_reply.begin(),
                         apdu_reply.end());
    response_data.push_back(CalculateXor(response_data));
    return MakeDataBlockTransferReply(sequence_number, icc_status,
                                      response_data);
  }
  GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected XfrBlock transfer: "
                              << HexDumpBytes(request_data);
}

// Builds a fake RDR_to_PC_Parameters message for replying to
// PC_to_RDR_SetParameters.
std::vector<uint8_t> MakeParametersTransferReply(
    uint8_t sequence_number,
    CcidIccStatus icc_status,
    optional<CardType> card_type,
    const std::vector<uint8_t>& protocol_data_structure) {
  GOOGLE_SMART_CARD_CHECK(card_type);
  // For now we always simulate success, in which case the reply contains the
  // same "abProtocolDataStructure" as the request.
  // The code below that encodes the data length only supports single-byte
  // length at the moment, for simplicity.
  GOOGLE_SMART_CARD_CHECK(protocol_data_structure.size() < 256);
  // The message format is per CCID specs.
  std::vector<uint8_t> transfer_reply = {
      0x82,
      (uint8_t)protocol_data_structure.size(),
      0x00,
      0x00,
      0x00,
      0x00,
      sequence_number,
      static_cast<uint8_t>(icc_status),
      0x00,
      0x00};
  transfer_reply.insert(transfer_reply.end(), protocol_data_structure.begin(),
                        protocol_data_structure.end());
  return transfer_reply;
}

// Builds a RDR_to_PC_NotifySlotChange message.
std::vector<uint8_t> MakeNotifySlotChangeTransferReply(CcidIccStatus icc_status,
                                                       bool slot0_changed) {
  // The message format is per CCID specs. The status byte contains two bits per
  // each slot (we simulate only single-slot devices at the moment): the first
  // bit says whether a card is present, and the second bit whether the card was
  // inserted/removed since the last RDR_to_PC_NotifySlotChange.
  const uint8_t slot0_current_bit = icc_status != CcidIccStatus::kNotPresent;
  const uint8_t status_byte = slot0_current_bit + (slot0_changed << 1);
  return {0x50, status_byte};
}

void PostFakeJsResponse(RequestId request_id,
                        GenericRequestResult result,
                        TypedMessageRouter* typed_message_router) {
  ResponseMessageData response_data;
  response_data.request_id = request_id;
  if (result.is_successful()) {
    response_data.payload =
        ArrayValueBuilder().Add(std::move(result).TakePayload()).Get();
  } else {
    response_data.error_message = result.error_message();
  }

  TypedMessage response;
  response.type =
      GetResponseMessageType(TestingSmartCardSimulation::kRequesterName);
  response.data = ConvertToValueOrDie(std::move(response_data));

  Value response_value = ConvertToValueOrDie(std::move(response));

  std::string error_message;
  if (!typed_message_router->OnMessageReceived(std::move(response_value),
                                               &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Dispatching fake JS reply failed: "
                                << error_message;
  }
}

}  // namespace

TestingSmartCardSimulation::TestingSmartCardSimulation(
    TypedMessageRouter* typed_message_router)
    : typed_message_router_(typed_message_router),
      handler_(typed_message_router_) {}

TestingSmartCardSimulation::~TestingSmartCardSimulation() = default;

void TestingSmartCardSimulation::OnRequestToJs(RequestId request_id,
                                               Value request_payload) {
  // Make the debug dump in advance, before we know whether we need to crash,
  // because we can't dump the value after std::move()'ing it.
  const std::string payload_debug_dump = DebugDumpValueFull(request_payload);

  RemoteCallRequestPayload remote_call =
      ConvertFromValueOrDie<RemoteCallRequestPayload>(
          std::move(request_payload));
  optional<GenericRequestResult> response;
  if (remote_call.function_name == "listDevices") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.empty());
    response = handler_.ListDevices();
  } else if (remote_call.function_name == "getConfigurations") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 1);
    response = handler_.GetConfigurations(
        /*device_id=*/remote_call.arguments[0].GetInteger());
  } else if (remote_call.function_name == "openDeviceHandle") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 1);
    response = handler_.OpenDeviceHandle(
        /*device_id=*/remote_call.arguments[0].GetInteger());
  } else if (remote_call.function_name == "closeDeviceHandle") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 2);
    response = handler_.CloseDeviceHandle(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger());
  } else if (remote_call.function_name == "claimInterface") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    response = handler_.ClaimInterface(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        /*interface_number=*/remote_call.arguments[2].GetInteger());
  } else if (remote_call.function_name == "releaseInterface") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    response = handler_.ReleaseInterface(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        /*interface_number=*/remote_call.arguments[2].GetInteger());
  } else if (remote_call.function_name == "controlTransfer") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    response = handler_.ControlTransfer(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        ConvertFromValueOrDie<LibusbJsControlTransferParameters>(
            std::move(remote_call.arguments[2])));
  } else if (remote_call.function_name == "bulkTransfer") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    response = handler_.BulkTransfer(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        ConvertFromValueOrDie<LibusbJsGenericTransferParameters>(
            std::move(remote_call.arguments[2])));
  } else if (remote_call.function_name == "interruptTransfer") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    response = handler_.InterruptTransfer(
        request_id,
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        ConvertFromValueOrDie<LibusbJsGenericTransferParameters>(
            std::move(remote_call.arguments[2])));
  } else {
    GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected request: " << payload_debug_dump;
  }

  // Send a fake response if the handler returned any.
  if (response) {
    PostFakeJsResponse(request_id, std::move(*response), typed_message_router_);
  }
}

void TestingSmartCardSimulation::SetDevices(
    const std::vector<Device>& devices) {
  handler_.SetDevices(devices);
}

std::vector<uint8_t> TestingSmartCardSimulation::GetCardAtr(
    CardType card_type) {
  // The hardcoded constants are taken from real cards.
  switch (card_type) {
    case CardType::kCosmoId70:
      return {0x3B, 0xDB, 0x96, 0x00, 0x80, 0xB1, 0xFE, 0x45, 0x1F, 0x83, 0x00,
              0x31, 0xC0, 0x64, 0xC7, 0xFC, 0x10, 0x00, 0x01, 0x90, 0x00, 0x74};
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

std::vector<uint8_t>
TestingSmartCardSimulation::GetCardProfileApplicationIdentifier(
    CardProfile card_profile) {
  // The hardcoded constants are taken from real cards.
  switch (card_profile) {
    case CardProfile::kCharismathicsPiv:
      return {0x61, 0x5C, 0x4F, 0x0B, 0xA0, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00,
              0x10, 0x00, 0x01, 0x00, 0x79, 0x07, 0x4F, 0x05, 0xA0, 0x00, 0x00,
              0x03, 0x08, 0x50, 0x27, 0x50, 0x65, 0x72, 0x73, 0x6F, 0x6E, 0x61,
              0x6C, 0x5F, 0x49, 0x64, 0x65, 0x6E, 0x74, 0x69, 0x74, 0x79, 0x5F,
              0x61, 0x6E, 0x64, 0x5F, 0x56, 0x65, 0x72, 0x69, 0x66, 0x69, 0x63,
              0x61, 0x74, 0x69, 0x6F, 0x6E, 0x5F, 0x43, 0x61, 0x72, 0x64, 0x5F,
              0x50, 0x1A, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x63, 0x73,
              0x72, 0x63, 0x2E, 0x6E, 0x69, 0x73, 0x74, 0x2E, 0x67, 0x6F, 0x76,
              0x2F, 0x6E, 0x70, 0x69, 0x76, 0x70};
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

TestingSmartCardSimulation::ThreadSafeHandler::ThreadSafeHandler(
    TypedMessageRouter* typed_message_router)
    : typed_message_router_(typed_message_router) {}

TestingSmartCardSimulation::ThreadSafeHandler::~ThreadSafeHandler() = default;

void TestingSmartCardSimulation::ThreadSafeHandler::SetDevices(
    const std::vector<Device>& devices) {
  std::unique_lock<std::mutex> lock(mutex_);

  std::vector<DeviceState> old_device_states = device_states_;
  device_states_.clear();
  for (const auto& device : devices) {
    DeviceState state;
    // If this device was already present, reuse its state.
    for (const auto& old_state : old_device_states) {
      if (old_state.device.id == device.id)
        state = old_state;
    }
    // Apply the new `Device` value. This also triggers state transitions (e.g.,
    // whether a card is inserted) and notifications (e.g., replying to a
    // pending interrupt transfer).
    UpdateDeviceState(device, state);
    device_states_.push_back(state);
  }
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::ListDevices() {
  std::unique_lock<std::mutex> lock(mutex_);

  std::vector<LibusbJsDevice> js_devices;
  for (const auto& device_state : device_states_)
    js_devices.push_back(MakeLibusbJsDevice(device_state.device));
  return GenericRequestResult::CreateSuccessful(
      ConvertToValueOrDie(std::move(js_devices)));
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::GetConfigurations(
    int64_t device_id) {
  std::unique_lock<std::mutex> lock(mutex_);

  DeviceState* device_state = FindDeviceStateById(device_id);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");
  return GenericRequestResult::CreateSuccessful(ConvertToValueOrDie(
      MakeLibusbJsConfigurationDescriptors(device_state->device.type)));
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::OpenDeviceHandle(
    int64_t device_id) {
  std::unique_lock<std::mutex> lock(mutex_);

  DeviceState* device_state = FindDeviceStateById(device_id);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");
  if (device_state->opened_device_handle)
    return GenericRequestResult::CreateFailed("Device already opened");
  device_state->opened_device_handle = next_free_device_handle_;
  ++next_free_device_handle_;
  return GenericRequestResult::CreateSuccessful(
      Value(*device_state->opened_device_handle));
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::CloseDeviceHandle(
    int64_t device_id,
    int64_t device_handle) {
  std::unique_lock<std::mutex> lock(mutex_);

  DeviceState* device_state =
      FindDeviceStateByIdAndHandle(device_id, device_handle);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");
  GOOGLE_SMART_CARD_CHECK(device_state->opened_device_handle == device_handle);
  device_state->opened_device_handle = {};
  return GenericRequestResult::CreateSuccessful(Value());
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::ClaimInterface(
    int64_t device_id,
    int64_t device_handle,
    int64_t interface_number) {
  std::unique_lock<std::mutex> lock(mutex_);

  DeviceState* device_state =
      FindDeviceStateByIdAndHandle(device_id, device_handle);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");
  if (device_state->claimed_interfaces.count(interface_number))
    return GenericRequestResult::CreateFailed("Interface already claimed");
  if (!DeviceInterfaceExists(device_state->device.type, interface_number))
    return GenericRequestResult::CreateFailed("Interface doesn't exist");
  device_state->claimed_interfaces.insert(interface_number);
  return GenericRequestResult::CreateSuccessful(Value());
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::ReleaseInterface(
    int64_t device_id,
    int64_t device_handle,
    int64_t interface_number) {
  std::unique_lock<std::mutex> lock(mutex_);

  DeviceState* device_state =
      FindDeviceStateByIdAndHandle(device_id, device_handle);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");
  if (!device_state->claimed_interfaces.count(interface_number))
    return GenericRequestResult::CreateFailed("Interface not claimed");
  device_state->claimed_interfaces.erase(interface_number);
  return GenericRequestResult::CreateSuccessful(Value());
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::ControlTransfer(
    int64_t device_id,
    int64_t device_handle,
    LibusbJsControlTransferParameters params) {
  // Values defined in the CCID protocol specification.
  constexpr uint8_t kGetDataRatesRequest = 3;

  std::unique_lock<std::mutex> lock(mutex_);

  DeviceState* device_state =
      FindDeviceStateByIdAndHandle(device_id, device_handle);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");

  if (params.request_type == LibusbJsTransferRequestType::kClass &&
      params.recipient == LibusbJsTransferRecipient::kInterface &&
      params.request == kGetDataRatesRequest) {
    // GET_DATA_RATES request to the reader.
    GOOGLE_SMART_CARD_CHECK(!params.data_to_send);
    LibusbJsTransferResult result;
    result.received_data = MakeGetDataRatesResponse(device_state->device.type);
    if (*params.length_to_receive < result.received_data->size())
      return GenericRequestResult::CreateFailed("Transfer overflow");
    return GenericRequestResult::CreateSuccessful(
        ConvertToValueOrDie(std::move(result)));
  }

  GOOGLE_SMART_CARD_LOG_FATAL << "Unknown control command: request="
                              << params.request;
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::BulkTransfer(
    int64_t device_id,
    int64_t device_handle,
    LibusbJsGenericTransferParameters params) {
  std::unique_lock<std::mutex> lock(mutex_);

  DeviceState* device_state =
      FindDeviceStateByIdAndHandle(device_id, device_handle);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");
  if (!DeviceEndpointExists(device_state->device.type, params.endpoint_address))
    return GenericRequestResult::CreateFailed("Unknown endpoint");
  if (params.data_to_send)
    return HandleOutputBulkTransfer(*params.data_to_send, *device_state);
  return HandleInputBulkTransfer(*params.length_to_receive, *device_state);
}

optional<GenericRequestResult>
TestingSmartCardSimulation::ThreadSafeHandler::InterruptTransfer(
    RequestId request_id,
    int64_t device_id,
    int64_t device_handle,
    LibusbJsGenericTransferParameters params) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* device_state =
      FindDeviceStateByIdAndHandle(device_id, device_handle);
  if (!device_state)
    return GenericRequestResult::CreateFailed("Unknown device");
  if (!DeviceEndpointExists(device_state->device.type,
                            params.endpoint_address)) {
    return GenericRequestResult::CreateFailed("Unknown endpoint");
  }
  // Don't reply immediately: the transfer will be resolved once a card
  // insertion/removal device event is simulated.
  device_state->pending_interrupt_transfers.push(request_id);
  return {};
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::HandleOutputBulkTransfer(
    const std::vector<uint8_t>& data_to_send,
    DeviceState& device_state) {
  // The message format is per CCID specs.
  // Extract the command's sequence number ("bSeq").
  if (data_to_send.size() < 7)
    GOOGLE_SMART_CARD_LOG_FATAL << "Missing bulk transfer sequence number";
  uint8_t sequence_number = data_to_send[6];

  switch (data_to_send[0]) {
    case 0x61: {
      // It's a PC_to_RDR_SetParameters request to the reader. Parse the
      // "abProtocolDataStructure" field (which, per specs, starts from offset
      // 10).
      GOOGLE_SMART_CARD_CHECK(data_to_send.size() >= 10);
      const std::vector<uint8_t> protocol_data_structure(
          data_to_send.begin() + 10, data_to_send.end());
      // Prepare a RDR_to_PC_Parameters reply for the next input bulk transfer.
      device_state.next_bulk_transfer_reply = MakeParametersTransferReply(
          sequence_number, device_state.icc_status,
          device_state.device.card_type, protocol_data_structure);
      return GenericRequestResult::CreateSuccessful(
          ConvertToValueOrDie(LibusbJsTransferResult()));
    }
    case 0x62: {
      // It's a PC_to_RDR_IccPowerOn request to the reader. If the card was
      // present and inactive, it needs to be transitioned into "active" state.
      if (device_state.icc_status == CcidIccStatus::kPresentInactive)
        device_state.icc_status = CcidIccStatus::kPresentActive;
      // Prepare a RDR_to_PC_DataBlock reply for the next input bulk transfer.
      device_state.next_bulk_transfer_reply =
          MakePowerOnTransferReply(sequence_number, device_state.icc_status,
                                   device_state.device.card_type);
      return GenericRequestResult::CreateSuccessful(
          ConvertToValueOrDie(LibusbJsTransferResult()));
    }
    case 0x63: {
      // It's a PC_to_RDR_IccPowerOff request to the reader. If the card was
      // present, it needs to be transitioned into "inactive" state.
      if (device_state.icc_status == CcidIccStatus::kPresentActive)
        device_state.icc_status = CcidIccStatus::kPresentInactive;
      // Prepare a RDR_to_PC_SlotStatus reply for the next input bulk transfer.
      device_state.next_bulk_transfer_reply =
          MakeSlotStatusTransferReply(sequence_number, device_state.icc_status);
      return GenericRequestResult::CreateSuccessful(
          ConvertToValueOrDie(LibusbJsTransferResult()));
    }
    case 0x65: {
      // It's a PC_to_RDR_GetSlotStatus request to the reader. Prepare a
      // RDR_to_PC_SlotStatus reply for the next input bulk transfer.
      device_state.next_bulk_transfer_reply =
          MakeSlotStatusTransferReply(sequence_number, device_state.icc_status);
      return GenericRequestResult::CreateSuccessful(
          ConvertToValueOrDie(LibusbJsTransferResult()));
    }
    case 0x6B: {
      // It's a PC_to_RDR_Escape request to the reader. Prepare a
      // RDR_to_PC_Escape reply for the next input bulk transfer.
      device_state.next_bulk_transfer_reply =
          MakeEscapeTransferReply(sequence_number, device_state.icc_status);
      return GenericRequestResult::CreateSuccessful(
          ConvertToValueOrDie(LibusbJsTransferResult()));
    }
    case 0x6F: {
      // It's a PC_to_RDR_XfrBlock request to the reader. Parse the "abData"
      // field (which, per specs, starts from offset 10).
      GOOGLE_SMART_CARD_CHECK(data_to_send.size() >= 10);
      const std::vector<uint8_t> request_data(data_to_send.begin() + 10,
                                              data_to_send.end());
      // Prepare a RDR_to_PC_DataBlock reply for the next input bulk transfer.
      device_state.next_bulk_transfer_reply = MakeXfrBlockTransferReply(
          sequence_number, device_state.icc_status,
          device_state.device.card_type, device_state.device.card_profile,
          request_data);
      return GenericRequestResult::CreateSuccessful(
          ConvertToValueOrDie(LibusbJsTransferResult()));
    }
  }
  // Unknown command.
  GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected output bulk transfer: "
                              << HexDumpBytes(data_to_send);
}

GenericRequestResult
TestingSmartCardSimulation::ThreadSafeHandler::HandleInputBulkTransfer(
    int64_t length_to_receive,
    DeviceState& device_state) {
  if (device_state.next_bulk_transfer_reply.empty()) {
    // Unexpected command - we have no reply prepared.
    GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected input bulk transfer";
  }
  if (device_state.next_bulk_transfer_reply.size() > length_to_receive)
    return GenericRequestResult::CreateFailed("Transfer overflow");
  LibusbJsTransferResult result;
  result.received_data = std::move(device_state.next_bulk_transfer_reply);
  device_state.next_bulk_transfer_reply.clear();
  return GenericRequestResult::CreateSuccessful(
      ConvertToValueOrDie(std::move(result)));
}

TestingSmartCardSimulation::DeviceState*
TestingSmartCardSimulation::ThreadSafeHandler::FindDeviceStateById(
    int64_t device_id) {
  for (auto& device_state : device_states_) {
    if (device_state.device.id == device_id)
      return &device_state;
  }
  return nullptr;
}

TestingSmartCardSimulation::DeviceState*
TestingSmartCardSimulation::ThreadSafeHandler::FindDeviceStateByIdAndHandle(
    int64_t device_id,
    int64_t device_handle) {
  DeviceState* device_state = FindDeviceStateById(device_id);
  if (device_state && device_state->opened_device_handle == device_handle)
    return device_state;
  return nullptr;
}

void TestingSmartCardSimulation::ThreadSafeHandler::UpdateDeviceState(
    const Device& device,
    DeviceState& device_state) {
  // Special handling for transitioning from the old `device_state.device` to
  // the new `device`.
  if (device_state.icc_status == CcidIccStatus::kNotPresent &&
      device.card_type) {
    // Simulate card insertion.
    device_state.icc_status = CcidIccStatus::kPresentInactive;
    NotifySlotChange(device_state);
  } else if (device_state.icc_status != CcidIccStatus::kNotPresent &&
             !device.card_type) {
    // Simulate card removal.
    device_state.icc_status = CcidIccStatus::kNotPresent;
    NotifySlotChange(device_state);
  }

  // Apply the whole `device`, including the fields that didn't require special
  // handling above.
  device_state.device = device;
}

void TestingSmartCardSimulation::ThreadSafeHandler::NotifySlotChange(
    DeviceState& device_state) {
  if (device_state.pending_interrupt_transfers.empty())
    return;

  const RequestId request_id = device_state.pending_interrupt_transfers.front();
  device_state.pending_interrupt_transfers.pop();

  // Resolve the interrupt transfer with a RDR_to_PC_NotifySlotChange message.
  std::vector<uint8_t> transfer_result = MakeNotifySlotChangeTransferReply(
      device_state.icc_status, /*slot0_changed=*/true);
  PostFakeJsResponse(
      request_id,
      GenericRequestResult::CreateSuccessful(Value(std::move(transfer_result))),
      typed_message_router_);
}

}  // namespace google_smart_card
