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

#include "libusb_tracing_wrapper.h"

#include <cstring>
#include <string>

#include <google_smart_card_common/logging/function_call_tracer.h>
#include <google_smart_card_common/logging/hex_dumping.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/logging/mask_dumping.h>

namespace google_smart_card {

namespace {

constexpr char kLoggingPrefix[] = "[libusb] ";

std::string DebugDumpLibusbReturnCode(int return_code) {
  std::string result = "\"";
  if (return_code == LIBUSB_SUCCESS)
    result += "LIBUSB_SUCCESS";
  else
    result += libusb_error_name(return_code);
  result += "\" [" + HexDumpInteger(return_code) + "]";
  return result;
}

std::string DebugDumpLibusbContext(const libusb_context* context) {
  return "libusb_context<" + (context ? HexDumpPointer(context) : "DEFAULT") +
         ">";
}

std::string DebugDumpLibusbDevice(const libusb_device* device_list) {
  return HexDumpPointer(device_list);
}

std::string DebugDumpLibusbDeviceList(libusb_device* const* device_list) {
  if (!device_list)
    return "<NULL>";
  std::string result;
  for (libusb_device* const* current_device = device_list; *current_device;
       ++current_device) {
    if (!result.empty())
      result += ", ";
    result += DebugDumpLibusbDevice(*current_device);
  }
  return HexDumpPointer(device_list) + "([" + result + "])";
}

std::string DebugDumpLibusbDescriptorType(uint8_t descriptor_type) {
  switch (descriptor_type) {
    case LIBUSB_DT_DEVICE:
      return "LIBUSB_DT_DEVICE";
    case LIBUSB_DT_CONFIG:
      return "LIBUSB_DT_CONFIG";
    case LIBUSB_DT_STRING:
      return "LIBUSB_DT_STRING";
    case LIBUSB_DT_INTERFACE:
      return "LIBUSB_DT_INTERFACE";
    case LIBUSB_DT_ENDPOINT:
      return "LIBUSB_DT_ENDPOINT";
    case LIBUSB_DT_BOS:
      return "LIBUSB_DT_BOS";
    case LIBUSB_DT_DEVICE_CAPABILITY:
      return "LIBUSB_DT_DEVICE_CAPABILITY";
    case LIBUSB_DT_HID:
      return "LIBUSB_DT_HID";
    case LIBUSB_DT_REPORT:
      return "LIBUSB_DT_REPORT";
    case LIBUSB_DT_PHYSICAL:
      return "LIBUSB_DT_PHYSICAL";
    case LIBUSB_DT_HUB:
      return "LIBUSB_DT_HUB";
    case LIBUSB_DT_SUPERSPEED_HUB:
      return "LIBUSB_DT_SUPERSPEED_HUB";
    case LIBUSB_DT_SS_ENDPOINT_COMPANION:
      return "LIBUSB_DT_SS_ENDPOINT_COMPANION";
    default:
      return HexDumpInteger(descriptor_type);
  }
}

std::string DebugDumpLibusbClassCode(uint8_t class_code) {
  switch (class_code) {
    case LIBUSB_CLASS_PER_INTERFACE:
      return "LIBUSB_CLASS_PER_INTERFACE";
    case LIBUSB_CLASS_AUDIO:
      return "LIBUSB_CLASS_AUDIO";
    case LIBUSB_CLASS_COMM:
      return "LIBUSB_CLASS_COMM";
    case LIBUSB_CLASS_HID:
      return "LIBUSB_CLASS_HID";
    case LIBUSB_CLASS_PHYSICAL:
      return "LIBUSB_CLASS_PHYSICAL";
    case LIBUSB_CLASS_PRINTER:
      return "LIBUSB_CLASS_PRINTER";
    case LIBUSB_CLASS_PTP:
      return "LIBUSB_CLASS_PTP";
    case LIBUSB_CLASS_MASS_STORAGE:
      return "LIBUSB_CLASS_MASS_STORAGE";
    case LIBUSB_CLASS_HUB:
      return "LIBUSB_CLASS_HUB";
    case LIBUSB_CLASS_DATA:
      return "LIBUSB_CLASS_DATA";
    case LIBUSB_CLASS_SMART_CARD:
      return "LIBUSB_CLASS_SMART_CARD";
    case LIBUSB_CLASS_CONTENT_SECURITY:
      return "LIBUSB_CLASS_CONTENT_SECURITY";
    case LIBUSB_CLASS_VIDEO:
      return "LIBUSB_CLASS_VIDEO";
    case LIBUSB_CLASS_PERSONAL_HEALTHCARE:
      return "LIBUSB_CLASS_PERSONAL_HEALTHCARE";
    case LIBUSB_CLASS_DIAGNOSTIC_DEVICE:
      return "LIBUSB_CLASS_DIAGNOSTIC_DEVICE";
    case LIBUSB_CLASS_WIRELESS:
      return "LIBUSB_CLASS_WIRELESS";
    case LIBUSB_CLASS_APPLICATION:
      return "LIBUSB_CLASS_APPLICATION";
    case LIBUSB_CLASS_VENDOR_SPEC:
      return "LIBUSB_CLASS_VENDOR_SPEC";
    default:
      return HexDumpInteger(class_code);
  }
}

std::string DebugDumpLibusbEndpointDirection(uint8_t endpoint_direction) {
  switch (endpoint_direction) {
    case LIBUSB_ENDPOINT_IN:
      return "LIBUSB_ENDPOINT_IN";
    case LIBUSB_ENDPOINT_OUT:
      return "LIBUSB_ENDPOINT_OUT";
    default:
      return HexDumpInteger(endpoint_direction);
  }
}

std::string DebugDumpLibusbEndpointAddress(uint8_t endpoint_address) {
  return HexDumpInteger(endpoint_address) + "(number=" +
         std::to_string(endpoint_address & LIBUSB_ENDPOINT_ADDRESS_MASK) +
         ", direction=" +
         DebugDumpLibusbEndpointDirection(endpoint_address &
                                          LIBUSB_ENDPOINT_DIR_MASK) +
         ")";
}

std::string DebugDumpLibusbTransferType(uint8_t transfer_type) {
  switch (transfer_type) {
    case LIBUSB_TRANSFER_TYPE_CONTROL:
      return "LIBUSB_TRANSFER_TYPE_CONTROL";
    case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
      return "LIBUSB_TRANSFER_TYPE_ISOCHRONOUS";
    case LIBUSB_TRANSFER_TYPE_BULK:
      return "LIBUSB_TRANSFER_TYPE_BULK";
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
      return "LIBUSB_TRANSFER_TYPE_INTERRUPT";
    case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
      return "LIBUSB_TRANSFER_TYPE_BULK_STREAM";
    default:
      return HexDumpInteger(transfer_type);
  }
}

std::string DebugDumpLibusbEndpointAttributes(uint8_t endpoint_attributes) {
  const int kIsoSyncTypeShift = 2;
  const int kIsoUsageTypeShift = 4;
  // TODO(emaxx): Print the debug dump of the iso_sync_type and the
  // iso_usage_type submasks, once the isochronous transfers are supported.
  return HexDumpInteger(endpoint_attributes) + "(transfer_type=" +
         DebugDumpLibusbTransferType(endpoint_attributes &
                                     LIBUSB_TRANSFER_TYPE_MASK) +
         ", iso_sync_type=" +
         std::to_string((endpoint_attributes & LIBUSB_ISO_SYNC_TYPE_MASK) >>
                        kIsoSyncTypeShift) +
         ", iso_usage_type=" +
         std::to_string((endpoint_attributes & LIBUSB_ISO_USAGE_TYPE_MASK) >>
                        kIsoUsageTypeShift) +
         ")";
}

std::string DebugDumpLibusbEndpointDescriptor(
    const libusb_endpoint_descriptor& endpoint_descriptor) {
  return "libusb_endpoint_descriptor(bLength=" +
         std::to_string(endpoint_descriptor.bLength) + ", bDescriptorType=" +
         DebugDumpLibusbDescriptorType(endpoint_descriptor.bDescriptorType) +
         ", bEndpointAddress=" +
         DebugDumpLibusbEndpointAddress(endpoint_descriptor.bEndpointAddress) +
         ", bmAttributes=" +
         DebugDumpLibusbEndpointAttributes(endpoint_descriptor.bmAttributes) +
         ", wMaxPacketSize=" +
         std::to_string(endpoint_descriptor.wMaxPacketSize) +
         ", bInterval=" + std::to_string(endpoint_descriptor.bInterval) +
         ", bRefresh=" + std::to_string(endpoint_descriptor.bRefresh) +
         ", bSynchAddress=" +
         std::to_string(endpoint_descriptor.bSynchAddress) + ", extra=<" +
         HexDumpBytes(endpoint_descriptor.extra,
                      endpoint_descriptor.extra_length) +
         ">, extra_length=" + std::to_string(endpoint_descriptor.extra_length) +
         ")";
}

std::string DebugDumpLibusbEndpointDescriptorList(
    const libusb_endpoint_descriptor* endpoint_descriptor_list,
    size_t size) {
  if (!endpoint_descriptor_list)
    return "<NULL>";
  std::string result;
  for (size_t index = 0; index < size; ++index) {
    if (!result.empty())
      result += ", ";
    result +=
        DebugDumpLibusbEndpointDescriptor(endpoint_descriptor_list[index]);
  }
  return "[" + result + "]";
}

std::string DebugDumpLibusbInterfaceDescriptor(
    const libusb_interface_descriptor& interface_descriptor) {
  return "libusb_interface_descriptor(bLength=" +
         std::to_string(interface_descriptor.bLength) + ", bDescriptorType=" +
         DebugDumpLibusbDescriptorType(interface_descriptor.bDescriptorType) +
         ", bInterfaceNumber=" +
         std::to_string(interface_descriptor.bInterfaceNumber) +
         ", bAlternateSetting=" +
         std::to_string(interface_descriptor.bAlternateSetting) +
         ", bNumEndpoints=" +
         std::to_string(interface_descriptor.bNumEndpoints) +
         ", bInterfaceClass=" +
         DebugDumpLibusbClassCode(interface_descriptor.bInterfaceClass) +
         ", endpoint=" +
         DebugDumpLibusbEndpointDescriptorList(
             interface_descriptor.endpoint,
             interface_descriptor.bNumEndpoints) +
         ", extra=<" +
         HexDumpBytes(interface_descriptor.extra,
                      interface_descriptor.extra_length) +
         ">, extra_length=" +
         std::to_string(interface_descriptor.extra_length) + ")";
}

std::string DebugDumpLibusbInterfaceDescriptorList(
    const libusb_interface_descriptor* interface_descriptor_list,
    size_t size) {
  if (!interface_descriptor_list)
    return "<NULL>";
  std::string result;
  for (size_t index = 0; index < size; ++index) {
    if (!result.empty())
      result += ", ";
    result +=
        DebugDumpLibusbInterfaceDescriptor(interface_descriptor_list[index]);
  }
  return "[" + result + "]";
}

std::string DebugDumpLibusbInterface(const libusb_interface& interface) {
  return "libusb_interface(altsetting=" +
         DebugDumpLibusbInterfaceDescriptorList(interface.altsetting,
                                                interface.num_altsetting) +
         ", num_altsetting=" + std::to_string(interface.num_altsetting) + ")";
}

std::string DebugDumpLibusbInterfaceList(const libusb_interface* interface_list,
                                         size_t size) {
  if (!interface_list)
    return "<NULL>";
  std::string result;
  for (size_t index = 0; index < size; ++index) {
    if (!result.empty())
      result += ", ";
    result += DebugDumpLibusbInterface(interface_list[index]);
  }
  return "[" + result + "]";
}

std::string DebugDumpLibusbConfigDescriptor(
    const libusb_config_descriptor& config_descriptor) {
  return "libusb_config_descriptor(bLength=" +
         std::to_string(config_descriptor.bLength) + ", bDescriptorType=" +
         DebugDumpLibusbDescriptorType(config_descriptor.bDescriptorType) +
         ", wTotalLength=" + std::to_string(config_descriptor.wTotalLength) +
         ", bNumInterfaces=" +
         std::to_string(config_descriptor.bNumInterfaces) +
         ", bConfigurationValue=" +
         std::to_string(config_descriptor.bConfigurationValue) +
         ", iConfiguration=" +
         std::to_string(config_descriptor.iConfiguration) +
         ", bmAttributes=" + std::to_string(config_descriptor.bmAttributes) +
         ", MaxPower=" + std::to_string(config_descriptor.MaxPower) +
         ", interface=" +
         DebugDumpLibusbInterfaceList(config_descriptor.interface,
                                      config_descriptor.bNumInterfaces) +
         ", extra=<" +
         HexDumpBytes(config_descriptor.extra, config_descriptor.extra_length) +
         ">, extra_length=" + std::to_string(config_descriptor.extra_length) +
         ")";
}

std::string DebugDumpLibusbConfigDescriptorPointer(
    const libusb_config_descriptor* config_descriptor) {
  return HexDumpPointer(config_descriptor) + "(" +
         DebugDumpLibusbConfigDescriptor(*config_descriptor) + ")";
}

std::string DebugDumpLibusbDeviceDescriptor(
    const libusb_device_descriptor& device_descriptor) {
  return "libusb_device_descriptor(bLength=" +
         std::to_string(device_descriptor.bLength) + ", bDescriptorType=" +
         DebugDumpLibusbDescriptorType(device_descriptor.bDescriptorType) +
         ", bcdUSB=" + HexDumpInteger(device_descriptor.bcdUSB) +
         ", bDeviceClass=" +
         DebugDumpLibusbClassCode(device_descriptor.bDeviceClass) +
         ", bDeviceSubClass=" +
         HexDumpInteger(device_descriptor.bDeviceSubClass) +
         ", bDeviceProtocol=" +
         HexDumpInteger(device_descriptor.bDeviceProtocol) +
         ", bMaxPacketSize0=" +
         std::to_string(device_descriptor.bMaxPacketSize0) +
         ", idVendor=" + HexDumpInteger(device_descriptor.idVendor) +
         ", idProduct=" + HexDumpInteger(device_descriptor.idProduct) +
         ", bcdDevice=" + HexDumpInteger(device_descriptor.bcdDevice) +
         ", iManufacturer=" + std::to_string(device_descriptor.iManufacturer) +
         ", iProduct=" + std::to_string(device_descriptor.iProduct) +
         ", iSerialNumber=" + std::to_string(device_descriptor.iSerialNumber) +
         ", bNumConfigurations=" +
         std::to_string(device_descriptor.bNumConfigurations) + ")";
}

std::string DebugDumpLibusbDeviceHandle(
    const libusb_device_handle* device_handle) {
  return "libusb_device_handle<" + HexDumpPointer(device_handle) + ">";
}

std::string DebugDumpLibusbRequestRecipient(uint8_t request_recipient) {
  switch (request_recipient) {
    case LIBUSB_RECIPIENT_DEVICE:
      return "LIBUSB_RECIPIENT_DEVICE";
    case LIBUSB_RECIPIENT_INTERFACE:
      return "LIBUSB_RECIPIENT_INTERFACE";
    case LIBUSB_RECIPIENT_ENDPOINT:
      return "LIBUSB_RECIPIENT_ENDPOINT";
    case LIBUSB_RECIPIENT_OTHER:
      return "LIBUSB_RECIPIENT_OTHER";
    default:
      return HexDumpInteger(request_recipient);
  }
}

std::string DebugDumpLibusbRequestType(uint8_t request_type) {
  switch (request_type) {
    case LIBUSB_REQUEST_TYPE_STANDARD:
      return "LIBUSB_REQUEST_TYPE_STANDARD";
    case LIBUSB_REQUEST_TYPE_CLASS:
      return "LIBUSB_REQUEST_TYPE_CLASS";
    case LIBUSB_REQUEST_TYPE_VENDOR:
      return "LIBUSB_REQUEST_TYPE_VENDOR";
    case LIBUSB_REQUEST_TYPE_RESERVED:
      return "LIBUSB_REQUEST_TYPE_RESERVED";
    default:
      return HexDumpInteger(request_type);
  }
}

std::string DebugDumpLibusbControlSetupRequestType(uint8_t request_type) {
  const int kRequestRecipientMask = (1 << 4) - 1;
  const int kRequestTypeMask = ((1 << 2) - 1) << 5;
  const int kDirectionMask = 1 << 7;
  return HexDumpInteger(request_type) + "(recipient=" +
         DebugDumpLibusbRequestRecipient(request_type & kRequestRecipientMask) +
         ", type=" +
         DebugDumpLibusbRequestType(request_type & kRequestTypeMask) +
         ", direction=" +
         DebugDumpLibusbEndpointDirection(request_type & kDirectionMask) + ")";
}

std::string DebugDumpInboundDataBuffer(const void* data,
                                       size_t size,
                                       bool is_input_data) {
  if (is_input_data)
    return HexDumpPointer(data);
  if (!data)
    return "<NULL>";
  return HexDumpPointer(data) + "<" + HexDumpBytes(data, size) + ">";
}

std::string DebugDumpOutboundDataBuffer(const void* data, size_t size) {
  if (!data)
    return "<NULL>";
  return HexDumpPointer(data) + "<" + HexDumpBytes(data, size) + ">";
}

std::string DebugDumpLibusbTransferFlagsMask(uint8_t transfer_flags_mask) {
  return DumpMask(
      transfer_flags_mask,
      {{LIBUSB_TRANSFER_SHORT_NOT_OK, "LIBUSB_TRANSFER_SHORT_NOT_OK"},
       {LIBUSB_TRANSFER_FREE_BUFFER, "LIBUSB_TRANSFER_FREE_BUFFER"},
       {LIBUSB_TRANSFER_FREE_TRANSFER, "LIBUSB_TRANSFER_FREE_TRANSFER"},
       {LIBUSB_TRANSFER_ADD_ZERO_PACKET, "LIBUSB_TRANSFER_ADD_ZERO_PACKET"}});
}

std::string DebugDumpLibusbTransferStatus(int transfer_status) {
  switch (transfer_status) {
    case LIBUSB_TRANSFER_COMPLETED:
      return "LIBUSB_TRANSFER_COMPLETED";
    case LIBUSB_TRANSFER_ERROR:
      return "LIBUSB_TRANSFER_ERROR";
    case LIBUSB_TRANSFER_TIMED_OUT:
      return "LIBUSB_TRANSFER_TIMED_OUT";
    case LIBUSB_TRANSFER_CANCELLED:
      return "LIBUSB_TRANSFER_CANCELLED";
    case LIBUSB_TRANSFER_STALL:
      return "LIBUSB_TRANSFER_STALL";
    case LIBUSB_TRANSFER_NO_DEVICE:
      return "LIBUSB_TRANSFER_NO_DEVICE";
    case LIBUSB_TRANSFER_OVERFLOW:
      return "LIBUSB_TRANSFER_OVERFLOW";
    default:
      return HexDumpInteger(transfer_status);
  }
}

std::string DebugDumpLibusbControlSetup(
    const libusb_control_setup* control_setup) {
  // Note that the structure fields, according to the documentation, are
  // always stored in the little-endian byte order, so accesses to the
  // multi-byte fields (wValue, wIndex and wLength) must be carefully wrapped
  // through libusb_le16_to_cpu() macro.
  return "libusb_control_setup(bmRequestType=" +
         DebugDumpLibusbControlSetupRequestType(control_setup->bmRequestType) +
         ", bRequest=" + HexDumpInteger(control_setup->bRequest) + ", wValue=" +
         HexDumpInteger(libusb_le16_to_cpu(control_setup->wValue)) +
         ", wIndex=" +
         HexDumpInteger(libusb_le16_to_cpu(control_setup->wIndex)) +
         ", wLength=" + std::to_string(control_setup->wLength) + ")";
}

std::string DebugDumpLibusbTransfer(libusb_transfer* transfer,
                                    bool is_inbound_argument) {
  if (!transfer)
    return "<NULL>";

  const bool is_input_transfer =
      (transfer->endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;
  const bool is_control_transfer =
      transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL;
  const void* const data = is_control_transfer
                               ? libusb_control_transfer_get_data(transfer)
                               : transfer->buffer;

  std::string result = HexDumpPointer(transfer) + "(libusb_transfer(";
  result += "dev_handle=" + DebugDumpLibusbDeviceHandle(transfer->dev_handle) +
            ", flags=" + DebugDumpLibusbTransferFlagsMask(transfer->flags) +
            ", endpoint=" + DebugDumpLibusbEndpointAddress(transfer->endpoint) +
            ", type=" + DebugDumpLibusbTransferType(transfer->type) +
            ", timeout=" + std::to_string(transfer->timeout);
  if (!is_inbound_argument)
    result += ", status=" + DebugDumpLibusbTransferStatus(transfer->status);
  result += ", length=" + std::to_string(transfer->length);
  if (!is_inbound_argument)
    result += ", actual_length=" + std::to_string(transfer->actual_length);
  result += ", callback=" +
            HexDumpPointer(reinterpret_cast<const void*>(transfer->callback)) +
            ", user_data=" + HexDumpPointer(transfer->user_data);
  if (is_inbound_argument) {
    result += ", buffer=";
    if (is_control_transfer) {
      result += HexDumpPointer(transfer->buffer) + " with " +
                DebugDumpLibusbControlSetup(
                    libusb_control_transfer_get_setup(transfer)) +
                " and data ";
    }
    result +=
        DebugDumpInboundDataBuffer(data, transfer->length, is_input_transfer);
  } else {
    if (is_input_transfer) {
      result += ", buffer=";
      if (is_control_transfer)
        result += HexDumpPointer(transfer->buffer) + " with data ";
      result += DebugDumpOutboundDataBuffer(data, transfer->actual_length);
    }
  }
  // TODO(emaxx): Print the debug dump of the iso_packet_desc field value, once
  // the isochronous transfers are supported.
  result +=
      ", num_iso_packets=" + std::to_string(transfer->num_iso_packets) + "))";
  return result;
}

// This is a helper class that provides wrapping around libusb transfers that
// adds debug logging for the transfer callback execution.
//
// The actual implementation is built on the idea of creating a temporary
// wrapper transfer that somehow holds the pointer to the original transfer.
class LibusbTransferTracingWrapper final {
 public:
  LibusbTransferTracingWrapper(const LibusbTransferTracingWrapper&) = delete;

  static libusb_transfer* CreateWrappedTransfer(
      libusb_transfer* transfer,
      LibusbInterface* wrapped_libusb) {
    // Note: Here a manual memory management is used, as the only entity that
    // can own the created class instance is libusb_transfer, which can store
    // only the pointer to it.
    //
    // The created LibusbTransferTracingWrapper instance is destroyed in the
    // LibusbTransferCallback method.
    LibusbTransferTracingWrapper* const wrapper =                    // NOLINT
        new LibusbTransferTracingWrapper(transfer, wrapped_libusb);  // NOLINT
    return wrapper->wrapper_transfer_;                               // NOLINT
  }

 private:
  LibusbTransferTracingWrapper(libusb_transfer* transfer,
                               LibusbInterface* wrapped_libusb)
      : transfer_(transfer), wrapped_libusb_(wrapped_libusb) {
    GOOGLE_SMART_CARD_CHECK(transfer_);
    GOOGLE_SMART_CARD_CHECK(wrapped_libusb_);

    // Isochronous transfers are not supported.
    GOOGLE_SMART_CARD_CHECK(!transfer_->num_iso_packets);

    wrapper_transfer_ = wrapped_libusb_->LibusbAllocTransfer(0);
    std::memcpy(wrapper_transfer_, transfer_, sizeof(libusb_transfer));
    wrapper_transfer_->callback = &LibusbTransferCallback;
    wrapper_transfer_->user_data = this;
    wrapper_transfer_->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
    wrapper_transfer_->flags &= ~LIBUSB_TRANSFER_FREE_BUFFER;
  }

  static void LibusbTransferCallback(libusb_transfer* wrapper_transfer) {
    GOOGLE_SMART_CARD_CHECK(wrapper_transfer);

    LibusbTransferTracingWrapper* const wrapper =
        static_cast<LibusbTransferTracingWrapper*>(wrapper_transfer->user_data);
    wrapper->FillOriginalTransferOutputFields();
    libusb_transfer* const original_transfer = wrapper->transfer_;
    delete wrapper;

    FunctionCallTracer tracer("libusb_transfer->callback", kLoggingPrefix);
    tracer.AddPassedArg("libusb_transfer",
                        DebugDumpLibusbTransfer(original_transfer, false));
    tracer.LogEntrance();

    original_transfer->callback(original_transfer);

    tracer.LogExit();
  }

  void FillOriginalTransferOutputFields() {
    transfer_->status = wrapper_transfer_->status;
    transfer_->actual_length = wrapper_transfer_->actual_length;
  }

  libusb_transfer* const transfer_;
  LibusbInterface* const wrapped_libusb_;
  libusb_transfer* wrapper_transfer_;
};

}  // namespace

LibusbTracingWrapper::LibusbTracingWrapper(LibusbInterface* wrapped_libusb)
    : wrapped_libusb_(wrapped_libusb) {
  GOOGLE_SMART_CARD_CHECK(wrapped_libusb_);
}

LibusbTracingWrapper::~LibusbTracingWrapper() = default;

int LibusbTracingWrapper::LibusbInit(libusb_context** ctx) {
  FunctionCallTracer tracer("libusb_init", kLoggingPrefix);
  tracer.AddPassedArg("ctx", HexDumpPointer(ctx));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbInit(ctx);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  if (return_code == LIBUSB_SUCCESS) {
    if (ctx)
      tracer.AddReturnedArg("*ctx", DebugDumpLibusbContext(*ctx));
  }
  tracer.LogExit();
  return return_code;
}

void LibusbTracingWrapper::LibusbExit(libusb_context* ctx) {
  FunctionCallTracer tracer("libusb_exit", kLoggingPrefix);
  tracer.AddPassedArg("ctx", DebugDumpLibusbContext(ctx));
  tracer.LogEntrance();

  wrapped_libusb_->LibusbExit(ctx);

  tracer.LogExit();
}

ssize_t LibusbTracingWrapper::LibusbGetDeviceList(libusb_context* ctx,
                                                  libusb_device*** list) {
  FunctionCallTracer tracer("libusb_get_device_list", kLoggingPrefix);
  tracer.AddPassedArg("ctx", DebugDumpLibusbContext(ctx));
  tracer.AddPassedArg("list", HexDumpPointer(list));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbGetDeviceList(ctx, list);

  tracer.AddReturnValue(return_code >= 0
                            ? std::to_string(return_code)
                            : DebugDumpLibusbReturnCode(return_code));
  if (return_code >= 0) {
    if (list)
      tracer.AddReturnedArg("*list", DebugDumpLibusbDeviceList(*list));
  }
  tracer.LogExit();
  return return_code;
}

void LibusbTracingWrapper::LibusbFreeDeviceList(libusb_device** list,
                                                int unref_devices) {
  FunctionCallTracer tracer("libusb_free_device_list", kLoggingPrefix);
  tracer.AddPassedArg("list", DebugDumpLibusbDeviceList(list));
  tracer.AddPassedArg("unref_devices", std::to_string(unref_devices));
  tracer.LogEntrance();

  wrapped_libusb_->LibusbFreeDeviceList(list, unref_devices);

  tracer.LogExit();
}

libusb_device* LibusbTracingWrapper::LibusbRefDevice(libusb_device* dev) {
  FunctionCallTracer tracer("libusb_ref_device", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDevice(dev));
  tracer.LogEntrance();

  libusb_device* const result = wrapped_libusb_->LibusbRefDevice(dev);

  tracer.AddReturnValue(HexDumpPointer(result));
  tracer.LogExit();
  return result;
}

void LibusbTracingWrapper::LibusbUnrefDevice(libusb_device* dev) {
  FunctionCallTracer tracer("libusb_unref_device", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDevice(dev));
  tracer.LogEntrance();

  wrapped_libusb_->LibusbUnrefDevice(dev);

  tracer.LogExit();
}

int LibusbTracingWrapper::LibusbGetActiveConfigDescriptor(
    libusb_device* dev,
    libusb_config_descriptor** config) {
  FunctionCallTracer tracer("libusb_get_active_config_descriptor",
                            kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDevice(dev));
  tracer.AddPassedArg("config", HexDumpPointer(config));
  tracer.LogEntrance();

  const int return_code =
      wrapped_libusb_->LibusbGetActiveConfigDescriptor(dev, config);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  if (return_code == LIBUSB_SUCCESS) {
    if (config) {
      tracer.AddReturnedArg("*config",
                            DebugDumpLibusbConfigDescriptorPointer(*config));
    }
  }
  tracer.LogExit();
  return return_code;
}

void LibusbTracingWrapper::LibusbFreeConfigDescriptor(
    libusb_config_descriptor* config) {
  FunctionCallTracer tracer("libusb_free_config_descriptor", kLoggingPrefix);
  tracer.AddPassedArg("config", HexDumpPointer(config));
  tracer.LogEntrance();

  wrapped_libusb_->LibusbFreeConfigDescriptor(config);

  tracer.LogExit();
}

int LibusbTracingWrapper::LibusbGetDeviceDescriptor(
    libusb_device* dev,
    libusb_device_descriptor* desc) {
  FunctionCallTracer tracer("libusb_get_device_descriptor", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDevice(dev));
  tracer.AddPassedArg("desc", HexDumpPointer(desc));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbGetDeviceDescriptor(dev, desc);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  if (return_code == LIBUSB_SUCCESS) {
    if (desc)
      tracer.AddReturnedArg("*desc", DebugDumpLibusbDeviceDescriptor(*desc));
  }
  tracer.LogExit();
  return return_code;
}

uint8_t LibusbTracingWrapper::LibusbGetBusNumber(libusb_device* dev) {
  FunctionCallTracer tracer("libusb_get_bus_number", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDevice(dev));
  tracer.LogEntrance();

  const uint8_t result = wrapped_libusb_->LibusbGetBusNumber(dev);

  tracer.AddReturnValue(std::to_string(result));
  tracer.LogExit();
  return result;
}

uint8_t LibusbTracingWrapper::LibusbGetDeviceAddress(libusb_device* dev) {
  FunctionCallTracer tracer("libusb_get_device_address", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDevice(dev));
  tracer.LogEntrance();

  const uint8_t result = wrapped_libusb_->LibusbGetDeviceAddress(dev);

  tracer.AddReturnValue(std::to_string(result));
  tracer.LogExit();
  return result;
}

int LibusbTracingWrapper::LibusbOpen(libusb_device* dev,
                                     libusb_device_handle** handle) {
  FunctionCallTracer tracer("libusb_open", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDevice(dev));
  tracer.AddPassedArg("handle", HexDumpPointer(handle));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbOpen(dev, handle);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  if (return_code == LIBUSB_SUCCESS) {
    if (handle)
      tracer.AddReturnedArg("*handle", DebugDumpLibusbDeviceHandle(*handle));
  }
  tracer.LogExit();
  return return_code;
}

void LibusbTracingWrapper::LibusbClose(libusb_device_handle* handle) {
  FunctionCallTracer tracer("libusb_close", kLoggingPrefix);
  tracer.AddPassedArg("handle", DebugDumpLibusbDeviceHandle(handle));
  tracer.LogEntrance();

  wrapped_libusb_->LibusbClose(handle);

  tracer.LogExit();
}

int LibusbTracingWrapper::LibusbClaimInterface(libusb_device_handle* dev,
                                               int interface_number) {
  FunctionCallTracer tracer("libusb_claim_interface", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDeviceHandle(dev));
  tracer.AddPassedArg("interface_number", std::to_string(interface_number));
  tracer.LogEntrance();

  const int return_code =
      wrapped_libusb_->LibusbClaimInterface(dev, interface_number);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  tracer.LogExit();
  return return_code;
}

int LibusbTracingWrapper::LibusbReleaseInterface(libusb_device_handle* dev,
                                                 int interface_number) {
  FunctionCallTracer tracer("libusb_release_interface", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDeviceHandle(dev));
  tracer.AddPassedArg("interface_number", std::to_string(interface_number));
  tracer.LogEntrance();

  const int return_code =
      wrapped_libusb_->LibusbReleaseInterface(dev, interface_number);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  tracer.LogExit();
  return return_code;
}

int LibusbTracingWrapper::LibusbResetDevice(libusb_device_handle* dev) {
  FunctionCallTracer tracer("libusb_reset_device", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDeviceHandle(dev));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbResetDevice(dev);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  tracer.LogExit();
  return return_code;
}

libusb_transfer* LibusbTracingWrapper::LibusbAllocTransfer(int iso_packets) {
  FunctionCallTracer tracer("libusb_alloc_transfer", kLoggingPrefix);
  tracer.AddPassedArg("iso_packets", std::to_string(iso_packets));
  tracer.LogEntrance();

  libusb_transfer* const result =
      wrapped_libusb_->LibusbAllocTransfer(iso_packets);

  tracer.AddReturnValue(HexDumpPointer(result));
  tracer.LogExit();
  return result;
}

int LibusbTracingWrapper::LibusbSubmitTransfer(libusb_transfer* transfer) {
  FunctionCallTracer tracer("libusb_submit_transfer", kLoggingPrefix);
  tracer.AddPassedArg("transfer", DebugDumpLibusbTransfer(transfer, true));
  tracer.LogEntrance();

  // In order to add the debug logging into the moment when the transfer
  // callback is executed, a copy of transfer is created with a wrapper
  // callback.
  libusb_transfer* const wrapped_transfer =
      LibusbTransferTracingWrapper::CreateWrappedTransfer(transfer,
                                                          wrapped_libusb_);
  AddOriginalToWrappedTransferMapItem(transfer, wrapped_transfer);

  const int return_code =
      wrapped_libusb_->LibusbSubmitTransfer(wrapped_transfer);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  tracer.LogExit();
  return return_code;
}

int LibusbTracingWrapper::LibusbCancelTransfer(libusb_transfer* transfer) {
  FunctionCallTracer tracer("libusb_cancel_transfer", kLoggingPrefix);
  tracer.AddPassedArg("transfer", HexDumpPointer(transfer));
  tracer.LogEntrance();

  // When the transfer was submitted, the original transfer was replaced with a
  // wrapped transfer (see the LibusbSubmitTransfer method implementation). So
  // here the actual cancellation should be called with the wrapper transfer.
  libusb_transfer* const wrapped_transfer = GetWrappedTransfer(transfer);
  if (!wrapped_transfer) {
    // The original transfer not submitted yet or already completed.
    return LIBUSB_ERROR_NOT_FOUND;
  }

  const int return_code =
      wrapped_libusb_->LibusbCancelTransfer(wrapped_transfer);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  tracer.LogExit();
  return return_code;
}

void LibusbTracingWrapper::LibusbFreeTransfer(libusb_transfer* transfer) {
  FunctionCallTracer tracer("libusb_free_transfer", kLoggingPrefix);
  tracer.AddPassedArg("transfer", HexDumpPointer(transfer));
  tracer.LogEntrance();

  wrapped_libusb_->LibusbFreeTransfer(transfer);

  // When the transfer was submitted, the original transfer was replaced with a
  // wrapped transfer (see the LibusbSubmitTransfer method implementation). So
  // here the mapping between these two transfers has to be deleted.
  RemoveOriginalToWrappedTransferMapItem(transfer);

  tracer.LogExit();
}

int LibusbTracingWrapper::LibusbControlTransfer(libusb_device_handle* dev,
                                                uint8_t bmRequestType,
                                                uint8_t bRequest,
                                                uint16_t wValue,
                                                uint16_t wIndex,
                                                unsigned char* data,
                                                uint16_t wLength,
                                                unsigned timeout) {
  FunctionCallTracer tracer("libusb_control_transfer", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDeviceHandle(dev));
  tracer.AddPassedArg("bmRequestType",
                      DebugDumpLibusbControlSetupRequestType(bmRequestType));
  tracer.AddPassedArg("bRequest", HexDumpInteger(bRequest));
  tracer.AddPassedArg("wValue", HexDumpInteger(wValue));
  tracer.AddPassedArg("wIndex", HexDumpInteger(wIndex));
  const bool is_input_transfer =
      (bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;
  tracer.AddPassedArg(
      "data", DebugDumpInboundDataBuffer(data, wLength, is_input_transfer));
  tracer.AddPassedArg("wLength", std::to_string(wLength));
  tracer.AddPassedArg("timeout", std::to_string(timeout));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbControlTransfer(
      dev, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout);

  tracer.AddReturnValue(return_code >= 0
                            ? std::to_string(return_code)
                            : DebugDumpLibusbReturnCode(return_code));
  if (return_code >= 0) {
    if (is_input_transfer) {
      tracer.AddReturnedArg("data",
                            DebugDumpOutboundDataBuffer(data, return_code));
    }
  }
  tracer.LogExit();
  return return_code;
}

int LibusbTracingWrapper::LibusbBulkTransfer(libusb_device_handle* dev,
                                             unsigned char endpoint,
                                             unsigned char* data,
                                             int length,
                                             int* actual_length,
                                             unsigned timeout) {
  FunctionCallTracer tracer("libusb_bulk_transfer", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDeviceHandle(dev));
  tracer.AddPassedArg("endpoint", DebugDumpLibusbEndpointAddress(endpoint));
  const bool is_input_transfer =
      (endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;
  tracer.AddPassedArg(
      "data", DebugDumpInboundDataBuffer(data, length, is_input_transfer));
  tracer.AddPassedArg("length", std::to_string(length));
  tracer.AddPassedArg("actual_length", HexDumpPointer(actual_length));
  tracer.AddPassedArg("timeout", std::to_string(timeout));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbBulkTransfer(
      dev, endpoint, data, length, actual_length, timeout);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  if (return_code == LIBUSB_SUCCESS) {
    if (is_input_transfer && actual_length) {
      tracer.AddReturnedArg("data",
                            DebugDumpOutboundDataBuffer(data, *actual_length));
    }
    if (actual_length)
      tracer.AddReturnedArg("*actual_length", std::to_string(*actual_length));
  }
  tracer.LogExit();
  return return_code;
}

int LibusbTracingWrapper::LibusbInterruptTransfer(libusb_device_handle* dev,
                                                  unsigned char endpoint,
                                                  unsigned char* data,
                                                  int length,
                                                  int* actual_length,
                                                  unsigned timeout) {
  FunctionCallTracer tracer("libusb_interrupt_transfer", kLoggingPrefix);
  tracer.AddPassedArg("dev", DebugDumpLibusbDeviceHandle(dev));
  tracer.AddPassedArg("endpoint", DebugDumpLibusbEndpointAddress(endpoint));
  const bool is_input_transfer =
      (endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN;
  tracer.AddPassedArg(
      "data", DebugDumpInboundDataBuffer(data, length, is_input_transfer));
  tracer.AddPassedArg("length", std::to_string(length));
  tracer.AddPassedArg("actual_length", HexDumpPointer(actual_length));
  tracer.AddPassedArg("timeout", std::to_string(timeout));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbInterruptTransfer(
      dev, endpoint, data, length, actual_length, timeout);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  if (return_code == LIBUSB_SUCCESS) {
    if (is_input_transfer && actual_length) {
      tracer.AddReturnedArg("data",
                            DebugDumpOutboundDataBuffer(data, *actual_length));
    }
    if (actual_length)
      tracer.AddReturnedArg("*actual_length", std::to_string(*actual_length));
  }
  tracer.LogExit();
  return return_code;
}

int LibusbTracingWrapper::LibusbHandleEvents(libusb_context* ctx) {
  FunctionCallTracer tracer("libusb_handle_events", kLoggingPrefix);
  tracer.AddPassedArg("ctx", DebugDumpLibusbContext(ctx));
  tracer.LogEntrance();

  const int return_code = wrapped_libusb_->LibusbHandleEvents(ctx);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  tracer.LogExit();
  return return_code;
}

int LibusbTracingWrapper::LibusbHandleEventsCompleted(libusb_context* ctx,
                                                      int* completed) {
  FunctionCallTracer tracer("libusb_handle_events_completed", kLoggingPrefix);
  tracer.AddPassedArg("ctx", DebugDumpLibusbContext(ctx));
  tracer.AddPassedArg("completed", HexDumpPointer(completed));
  tracer.LogEntrance();

  const int return_code =
      wrapped_libusb_->LibusbHandleEventsCompleted(ctx, completed);

  tracer.AddReturnValue(DebugDumpLibusbReturnCode(return_code));
  tracer.LogExit();
  return return_code;
}

void LibusbTracingWrapper::AddOriginalToWrappedTransferMapItem(
    libusb_transfer* original_transfer,
    libusb_transfer* wrapped_transfer) {
  const std::unique_lock<std::mutex> lock(mutex_);
  // The mapping value under the original_transfer key, if previously existed,
  // is just overwritten here, because libusb API allows to re-use the same
  // libusb_transfer structure multiple times.
  original_to_wrapped_transfer_map_[original_transfer] = wrapped_transfer;
}

libusb_transfer* LibusbTracingWrapper::GetWrappedTransfer(
    libusb_transfer* original_transfer) const {
  const std::unique_lock<std::mutex> lock(mutex_);
  const auto iter = original_to_wrapped_transfer_map_.find(original_transfer);
  if (iter == original_to_wrapped_transfer_map_.end())
    return nullptr;
  return iter->second;
}

void LibusbTracingWrapper::RemoveOriginalToWrappedTransferMapItem(
    libusb_transfer* original_transfer) {
  const std::unique_lock<std::mutex> lock(mutex_);
  original_to_wrapped_transfer_map_.erase(original_transfer);
}

}  // namespace google_smart_card
