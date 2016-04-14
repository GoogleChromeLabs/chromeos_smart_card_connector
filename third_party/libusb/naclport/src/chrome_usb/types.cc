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

#include "chrome_usb/types.h"

#include <google_smart_card_common/pp_var_utils/enum_converter.h>
#include <google_smart_card_common/pp_var_utils/struct_converter.h>

namespace google_smart_card {

using chrome_usb::Direction;
using chrome_usb::Device;
using chrome_usb::ConnectionHandle;
using chrome_usb::EndpointDescriptorType;
using chrome_usb::EndpointDescriptorSynchronization;
using chrome_usb::EndpointDescriptorUsage;
using chrome_usb::EndpointDescriptor;
using chrome_usb::InterfaceDescriptor;
using chrome_usb::ConfigDescriptor;
using chrome_usb::GenericTransferInfo;
using chrome_usb::ControlTransferInfoRecipient;
using chrome_usb::ControlTransferInfoRequestType;
using chrome_usb::ControlTransferInfo;
using chrome_usb::TransferResultInfo;
using chrome_usb::DeviceFilter;
using chrome_usb::GetDevicesOptions;
using chrome_usb::GetUserSelectedDevicesOptions;

using DirectionConverter = EnumConverter<Direction, std::string>;

using DeviceConverter = StructConverter<Device>;

using ConnectionHandleConverter = StructConverter<ConnectionHandle>;

using EndpointDescriptorTypeConverter = EnumConverter<
    EndpointDescriptorType, std::string>;

using EndpointDescriptorSynchronizationConverter = EnumConverter<
    EndpointDescriptorSynchronization, std::string>;

using EndpointDescriptorUsageConverter = EnumConverter<
    EndpointDescriptorUsage, std::string>;

using EndpointDescriptorConverter = StructConverter<EndpointDescriptor>;

using InterfaceDescriptorConverter = StructConverter<InterfaceDescriptor>;

using ConfigDescriptorConverter = StructConverter<ConfigDescriptor>;

using GenericTransferInfoConverter = StructConverter<GenericTransferInfo>;

using ControlTransferInfoRecipientConverter = EnumConverter<
    ControlTransferInfoRecipient, std::string>;

using ControlTransferInfoRequestTypeConverter = EnumConverter<
    ControlTransferInfoRequestType, std::string>;

using ControlTransferInfoConverter = StructConverter<ControlTransferInfo>;

using TransferResultInfoConverter = StructConverter<TransferResultInfo>;

using DeviceFilterConverter = StructConverter<DeviceFilter>;

using GetDevicesOptionsConverter = StructConverter<GetDevicesOptions>;

using GetUserSelectedDevicesOptionsConverter =
    StructConverter<GetUserSelectedDevicesOptions>;

template <>
constexpr const char* DirectionConverter::GetEnumTypeName() {
  return "chrome_usb::Direction";
}

template <>
template <typename Callback>
void DirectionConverter::VisitCorrespondingPairs(Callback callback) {
  callback(Direction::kIn, "in");
  callback(Direction::kOut, "out");
}

template <>
constexpr const char* DeviceConverter::GetStructTypeName() {
  return "chrome_usb::Device";
}

template <>
template <typename Callback>
void DeviceConverter::VisitFields(const Device& value, Callback callback) {
  callback(&value.device, "device");
  callback(&value.vendor_id, "vendorId");
  callback(&value.product_id, "productId");
  callback(&value.product_name, "productName");
  callback(&value.manufacturer_name, "manufacturerName");
  callback(&value.serial_number, "serialNumber");
}

template <>
constexpr const char* ConnectionHandleConverter::GetStructTypeName() {
  return "chrome_usb::ConnectionHandle";
}

template <>
template <typename Callback>
void ConnectionHandleConverter::VisitFields(
    const ConnectionHandle& value, Callback callback) {
  callback(&value.handle, "handle");
  callback(&value.vendor_id, "vendorId");
  callback(&value.product_id, "productId");
}

template <>
constexpr const char* EndpointDescriptorTypeConverter::GetEnumTypeName() {
  return "chrome_usb::EndpointDescriptorType";
}

template <>
template <typename Callback>
void EndpointDescriptorTypeConverter::VisitCorrespondingPairs(
    Callback callback) {
  callback(EndpointDescriptorType::kControl, "control");
  callback(EndpointDescriptorType::kInterrupt, "interrupt");
  callback(EndpointDescriptorType::kIsochronous, "isochronous");
  callback(EndpointDescriptorType::kBulk, "bulk");
}

template <>
constexpr const char*
EndpointDescriptorSynchronizationConverter::GetEnumTypeName() {
  return "chrome_usb::EndpointDescriptorSynchronization";
}

template <>
template <typename Callback>
void EndpointDescriptorSynchronizationConverter::VisitCorrespondingPairs(
    Callback callback) {
  callback(EndpointDescriptorSynchronization::kAsynchronous, "asynchronous");
  callback(EndpointDescriptorSynchronization::kAdaptive, "adaptive");
  callback(EndpointDescriptorSynchronization::kSynchronous, "synchronous");
}

template <>
constexpr const char* EndpointDescriptorUsageConverter::GetEnumTypeName() {
  return "chrome_usb::EndpointDescriptorUsage";
}

template <>
template <typename Callback>
void EndpointDescriptorUsageConverter::VisitCorrespondingPairs(
    Callback callback) {
  callback(EndpointDescriptorUsage::kData, "data");
  callback(EndpointDescriptorUsage::kFeedback, "feedback");
  callback(EndpointDescriptorUsage::kExplicitFeedback, "explicitFeedback");
}

template <>
constexpr const char* EndpointDescriptorConverter::GetStructTypeName() {
  return "chrome_usb::EndpointDescriptor";
}

template <>
template <typename Callback>
void EndpointDescriptorConverter::VisitFields(
    const EndpointDescriptor& value, Callback callback) {
  callback(&value.address, "address");
  callback(&value.type, "type");
  callback(&value.direction, "direction");
  callback(&value.maximum_packet_size, "maximumPacketSize");
  callback(&value.synchronization, "synchronization");
  callback(&value.usage, "usage");
  callback(&value.polling_interval, "pollingInterval");
  callback(&value.extra_data, "extra_data");
}

template <>
constexpr const char* InterfaceDescriptorConverter::GetStructTypeName() {
  return "chrome_usb::InterfaceDescriptor";
}

template <>
template <typename Callback>
void InterfaceDescriptorConverter::VisitFields(
    const InterfaceDescriptor& value, Callback callback) {
  callback(&value.interface_number, "interfaceNumber");
  callback(&value.alternate_setting, "alternateSetting");
  callback(&value.interface_class, "interfaceClass");
  callback(&value.interface_subclass, "interfaceSubclass");
  callback(&value.interface_protocol, "interfaceProtocol");
  callback(&value.description, "description");
  callback(&value.endpoints, "endpoints");
  callback(&value.extra_data, "extra_data");
}

template <>
constexpr const char* ConfigDescriptorConverter::GetStructTypeName() {
  return "chrome_usb::ConfigDescriptor";
}

template <>
template <typename Callback>
void ConfigDescriptorConverter::VisitFields(
    const ConfigDescriptor& value, Callback callback) {
  callback(&value.active, "active");
  callback(&value.configuration_value, "configurationValue");
  callback(&value.description, "description");
  callback(&value.self_powered, "selfPowered");
  callback(&value.remote_wakeup, "remoteWakeup");
  callback(&value.max_power, "maxPower");
  callback(&value.interfaces, "interfaces");
  callback(&value.extra_data, "extra_data");
}

template <>
constexpr const char* GenericTransferInfoConverter::GetStructTypeName() {
  return "chrome_usb::GenericTransferInfo";
}

template <>
template <typename Callback>
void GenericTransferInfoConverter::VisitFields(
    const GenericTransferInfo& value, Callback callback) {
  callback(&value.direction, "direction");
  callback(&value.endpoint, "endpoint");
  callback(&value.length, "length");
  callback(&value.data, "data");
  callback(&value.timeout, "timeout");
}

template <>
constexpr const char* ControlTransferInfoRecipientConverter::GetEnumTypeName() {
  return "chrome_usb::ControlTransferInfoRecipient";
}

template <>
template <typename Callback>
void ControlTransferInfoRecipientConverter::VisitCorrespondingPairs(
    Callback callback) {
  callback(ControlTransferInfoRecipient::kDevice, "device");
  callback(ControlTransferInfoRecipient::kInterface, "interface");
  callback(ControlTransferInfoRecipient::kEndpoint, "endpoint");
  callback(ControlTransferInfoRecipient::kOther, "other");
}

template <>
constexpr const char*
ControlTransferInfoRequestTypeConverter::GetEnumTypeName() {
  return "chrome_usb::ControlTransferInfoRequestType";
}

template <>
template <typename Callback>
void ControlTransferInfoRequestTypeConverter::VisitCorrespondingPairs(
    Callback callback) {
  callback(ControlTransferInfoRequestType::kStandard, "standard");
  callback(ControlTransferInfoRequestType::kClass, "class");
  callback(ControlTransferInfoRequestType::kVendor, "vendor");
  callback(ControlTransferInfoRequestType::kReserved, "reserved");
}

template <>
constexpr const char* ControlTransferInfoConverter::GetStructTypeName() {
  return "chrome_usb::ControlTransferInfo";
}

template <>
template <typename Callback>
void ControlTransferInfoConverter::VisitFields(
    const ControlTransferInfo& value, Callback callback) {
  callback(&value.direction, "direction");
  callback(&value.recipient, "recipient");
  callback(&value.request_type, "requestType");
  callback(&value.request, "request");
  callback(&value.value, "value");
  callback(&value.index, "index");
  callback(&value.length, "length");
  callback(&value.data, "data");
  callback(&value.timeout, "timeout");
}

template <>
constexpr const char* TransferResultInfoConverter::GetStructTypeName() {
  return "chrome_usb::TransferResultInfo";
}

template <>
template <typename Callback>
void TransferResultInfoConverter::VisitFields(
    const TransferResultInfo& value, Callback callback) {
  callback(&value.result_code, "resultCode");
  callback(&value.data, "data");
}

template <>
constexpr const char* DeviceFilterConverter::GetStructTypeName() {
  return "chrome_usb::DeviceFilter";
}

template <>
template <typename Callback>
void DeviceFilterConverter::VisitFields(
    const DeviceFilter& value, Callback callback) {
  callback(&value.vendor_id, "vendorId");
  callback(&value.product_id, "productId");
  callback(&value.interface_class, "interfaceClass");
  callback(&value.interface_subclass, "interfaceSubclass");
  callback(&value.interface_protocol, "interfaceProtocol");
}

template <>
constexpr const char* GetDevicesOptionsConverter::GetStructTypeName() {
  return "chrome_usb::GetDevicesOptions";
}

template <>
template <typename Callback>
void GetDevicesOptionsConverter::VisitFields(
    const GetDevicesOptions& value, Callback callback) {
  callback(&value.filters, "filters");
}

template <>
constexpr const char*
GetUserSelectedDevicesOptionsConverter::GetStructTypeName() {
  return "chrome_usb::GetUserSelectedDevicesOptions";
}

template <>
template <typename Callback>
void GetUserSelectedDevicesOptionsConverter::VisitFields(
    const GetUserSelectedDevicesOptions& value, Callback callback) {
  callback(&value.filters, "filters");
}

namespace chrome_usb {

bool VarAs(const pp::Var& var, Direction* result, std::string* error_message) {
  return DirectionConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(Direction value) {
  return DirectionConverter::ConvertToVar(value);
}

bool VarAs(const pp::Var& var, Device* result, std::string* error_message) {
  return DeviceConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(const Device& value) {
  return DeviceConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var, ConnectionHandle* result, std::string* error_message) {
  return ConnectionHandleConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(const ConnectionHandle& value) {
  return ConnectionHandleConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var,
    EndpointDescriptorType* result,
    std::string* error_message) {
  return EndpointDescriptorTypeConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(EndpointDescriptorType value) {
  return EndpointDescriptorTypeConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var,
    EndpointDescriptorSynchronization* result,
    std::string* error_message) {
  return EndpointDescriptorSynchronizationConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(EndpointDescriptorSynchronization value) {
  return EndpointDescriptorSynchronizationConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var,
    EndpointDescriptorUsage* result,
    std::string* error_message) {
  return EndpointDescriptorUsageConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(EndpointDescriptorUsage value) {
  return EndpointDescriptorUsageConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var,
    EndpointDescriptor* result,
    std::string* error_message) {
  return EndpointDescriptorConverter::ConvertFromVar(
      var, result, error_message);
}

bool VarAs(
    const pp::Var& var,
    InterfaceDescriptor* result,
    std::string* error_message) {
  return InterfaceDescriptorConverter::ConvertFromVar(
      var, result, error_message);
}

bool VarAs(
    const pp::Var& var,
    ConfigDescriptor* result,
    std::string* error_message) {
  return ConfigDescriptorConverter::ConvertFromVar(var, result, error_message);
}

pp::Var MakeVar(const GenericTransferInfo& value) {
  return GenericTransferInfoConverter::ConvertToVar(value);
}

pp::Var MakeVar(ControlTransferInfoRecipient value) {
  return ControlTransferInfoRecipientConverter::ConvertToVar(value);
}

pp::Var MakeVar(ControlTransferInfoRequestType value) {
  return ControlTransferInfoRequestTypeConverter::ConvertToVar(value);
}

pp::Var MakeVar(const ControlTransferInfo& value) {
  return ControlTransferInfoConverter::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var,
    TransferResultInfo* result,
    std::string* error_message) {
  return TransferResultInfoConverter::ConvertFromVar(
      var, result, error_message);
}

pp::Var MakeVar(const DeviceFilter& value) {
  return DeviceFilterConverter::ConvertToVar(value);
}

pp::Var MakeVar(const GetDevicesOptions& value) {
  return GetDevicesOptionsConverter::ConvertToVar(value);
}

pp::Var MakeVar(const GetUserSelectedDevicesOptions& value) {
  return GetUserSelectedDevicesOptionsConverter::ConvertToVar(value);
}

}  // namespace chrome_usb

}  // namespace google_smart_card
