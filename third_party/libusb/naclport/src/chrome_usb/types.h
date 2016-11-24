// Copyright 2016 Google Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

// This file contains definitions of the C++ analogs for the chrome.usb API
// types. For the chrome.usb API documentation, please refer to:
// <https://developer.chrome.com/apps/usb>.
//
// Note that some of the types defined here have no specific name in the
// chrome.usb documentation (like the "type" field type of the
// "EndpointDescriptor" type).
//
// Also there are function overloads defined that perform the conversion between
// values of these types and Pepper values (which correspond to the JavaScript
// values used with chrome.usb API).
//
// FIXME(emaxx): Think about adding a space for all unrecognized structure
// fields, as currently any change in chrome.usb API that adds a new required
// field to any input type will break communication with this library.

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_TYPES_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_TYPES_H_

#include <stdint.h>

#include <functional>
#include <string>
#include <vector>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array_buffer.h>

#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/requesting/request_result.h>

namespace google_smart_card {

namespace chrome_usb {

//
// Following are the definitions of the analogs of the types defined in
// chrome.usb API and the types that represent arguments of the chrome.usb API
// methods.
//

enum class Direction {
  kIn,
  kOut,
};

struct Device {
  bool operator==(const Device& other) const;

  int64_t device;
  int64_t vendor_id;
  int64_t product_id;
  optional<int64_t> version;
  std::string product_name;
  std::string manufacturer_name;
  std::string serial_number;
};

struct ConnectionHandle {
  bool operator==(const ConnectionHandle& other) const;

  int64_t handle;
  int64_t vendor_id;
  int64_t product_id;
};

enum class EndpointDescriptorType {
  kControl,
  kInterrupt,
  kIsochronous,
  kBulk,
};

enum class EndpointDescriptorSynchronization {
  kAsynchronous,
  kAdaptive,
  kSynchronous,
};

enum class EndpointDescriptorUsage {
  kData,
  kFeedback,
  kExplicitFeedback,
  kPeriodic,
  kNotification,
};

struct EndpointDescriptor {
  int64_t address;
  EndpointDescriptorType type;
  Direction direction;
  int64_t maximum_packet_size;
  optional<EndpointDescriptorSynchronization> synchronization;
  optional<EndpointDescriptorUsage> usage;
  optional<int64_t> polling_interval;
  pp::VarArrayBuffer extra_data;
};

struct InterfaceDescriptor {
  int64_t interface_number;
  int64_t alternate_setting;
  int64_t interface_class;
  int64_t interface_subclass;
  int64_t interface_protocol;
  optional<std::string> description;
  std::vector<EndpointDescriptor> endpoints;
  pp::VarArrayBuffer extra_data;
};

struct ConfigDescriptor {
  bool active;
  int64_t configuration_value;
  optional<std::string> description;
  bool self_powered;
  bool remote_wakeup;
  int64_t max_power;
  std::vector<InterfaceDescriptor> interfaces;
  pp::VarArrayBuffer extra_data;
};

struct GenericTransferInfo {
  Direction direction;
  int64_t endpoint;
  optional<int64_t> length;
  optional<pp::VarArrayBuffer> data;
  optional<int64_t> timeout;
};

enum class ControlTransferInfoRecipient {
  kDevice,
  kInterface,
  kEndpoint,
  kOther,
};

enum class ControlTransferInfoRequestType {
  kStandard,
  kClass,
  kVendor,
  kReserved,
};

struct ControlTransferInfo {
  bool operator==(const ControlTransferInfo& other) const;

  Direction direction;
  ControlTransferInfoRecipient recipient;
  ControlTransferInfoRequestType request_type;
  int64_t request;
  int64_t value;
  int64_t index;
  optional<int64_t> length;
  optional<pp::VarArrayBuffer> data;
  optional<int64_t> timeout;
};

struct TransferResultInfo {
  optional<int64_t> result_code;
  optional<pp::VarArrayBuffer> data;
};

constexpr int64_t kTransferResultInfoSuccessResultCode = 0;

struct DeviceFilter {
  optional<int64_t> vendor_id;
  optional<int64_t> product_id;
  optional<int64_t> interface_class;
  optional<int64_t> interface_subclass;
  optional<int64_t> interface_protocol;
};

struct GetDevicesOptions {
  optional<std::vector<DeviceFilter>> filters;
};

struct GetUserSelectedDevicesOptions {
  optional<bool> multiple;
  optional<std::vector<DeviceFilter>> filters;
};

//
// Following are the function overloads that can be used to perform the
// conversion between values of these types and Pepper values (which correspond
// to the JavaScript values used with chrome.usb API).
//

bool VarAs(
    const pp::Var& var,
    Direction* result,
    std::string* error_message);

pp::Var MakeVar(Direction value);

bool VarAs(
    const pp::Var& var, Device* result, std::string* error_message);

pp::Var MakeVar(const Device& value);

bool VarAs(
    const pp::Var& var,
    ConnectionHandle* result,
    std::string* error_message);

pp::Var MakeVar(const ConnectionHandle& value);

bool VarAs(
    const pp::Var& var,
    EndpointDescriptorType* result,
    std::string* error_message);

pp::Var MakeVar(EndpointDescriptorType value);

bool VarAs(
    const pp::Var& var,
    EndpointDescriptorSynchronization* result,
    std::string* error_message);

pp::Var MakeVar(EndpointDescriptorSynchronization value);

bool VarAs(
    const pp::Var& var,
    EndpointDescriptorUsage* result,
    std::string* error_message);

pp::Var MakeVar(EndpointDescriptorUsage value);

bool VarAs(
    const pp::Var& var,
    EndpointDescriptor* result,
    std::string* error_message);

bool VarAs(
    const pp::Var& var,
    InterfaceDescriptor* result,
    std::string* error_message);

bool VarAs(
    const pp::Var& var,
    ConfigDescriptor* result,
    std::string* error_message);

pp::Var MakeVar(const GenericTransferInfo& value);

pp::Var MakeVar(ControlTransferInfoRecipient value);

pp::Var MakeVar(ControlTransferInfoRequestType value);

pp::Var MakeVar(const ControlTransferInfo& value);

bool VarAs(
    const pp::Var& var,
    TransferResultInfo* result,
    std::string* error_message);

pp::Var MakeVar(const DeviceFilter& value);

pp::Var MakeVar(const GetDevicesOptions& value);

pp::Var MakeVar(const GetUserSelectedDevicesOptions& value);

//
// Following are the definitions of structures representing the results returned
// from the chrome.usb API methods.
//

struct GetDevicesResult {
  std::vector<Device> devices;
};

struct GetUserSelectedDevicesResult {
  std::vector<Device> devices;
};

struct GetConfigurationsResult {
  std::vector<ConfigDescriptor> configurations;
};

struct OpenDeviceResult {
  ConnectionHandle connection_handle;
};

struct CloseDeviceResult {};

struct SetConfigurationResult {};

struct GetConfigurationResult {
  ConfigDescriptor configuration;
};

struct ListInterfacesResult {
  std::vector<InterfaceDescriptor> descriptors;
};

struct ClaimInterfaceResult {};

struct ReleaseInterfaceResult {};

struct TransferResult {
  TransferResultInfo result_info;
};

struct ResetDeviceResult {
  bool reset_success;
};

// This type represents a callback that is used for receiving the asynchronous
// transfer results.
using AsyncTransferCallback = std::function<
    void(RequestResult<TransferResult>)>;

}  // namespace chrome_usb

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_TYPES_H_
