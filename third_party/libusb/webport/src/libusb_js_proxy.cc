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

#include "libusb_js_proxy.h"

#include <stdlib.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/requesting/request_result.h>

#include "libusb_js_proxy_data_model.h"

namespace google_smart_card {

namespace {

// These constants must match the strings in libusb-proxy-receiver.js.
constexpr char kJsRequesterName[] = "libusb";
constexpr char kJsRequestListDevices[] = "list_devices";

//
// We use stubs for the device bus number (as the chrome.usb API does not
// provide means of retrieving it). We modify this for a device when opening
// the device fails. This makes PCSC recognize it as a new device which causes
// PCSC to retry opening it. The number of reconnection attempts is limited
// by kMaximumBusNumber - kDefaultBusNumber.
//

constexpr uint8_t kDefaultBusNumber = 1;
constexpr uint8_t kMaximumBusNumber = 64;

//
// Bit mask values for the bmAttributes field of the libusb_config_descriptor
// structure.
//

constexpr uint8_t kLibusbConfigDescriptorBmAttributesRemoteWakeup = 1 << 5;
constexpr uint8_t kLibusbConfigDescriptorBmAttributesSelfPowered = 1 << 6;

//
// Positions of the first non-zero bits in the libusb mask constants.
//

constexpr int kLibusbTransferTypeMaskShift = 0;
static_assert(!(LIBUSB_TRANSFER_TYPE_MASK &
                ((1 << kLibusbTransferTypeMaskShift) - 1)),
              "kLibusbTransferTypeMaskShift constant is wrong");
static_assert((LIBUSB_TRANSFER_TYPE_MASK >> kLibusbTransferTypeMaskShift) & 1,
              "kLibusbTransferTypeMaskShift constant is wrong");

constexpr int kLibusbIsoSyncTypeMaskShift = 2;
static_assert(!(LIBUSB_ISO_SYNC_TYPE_MASK &
                ((1 << kLibusbIsoSyncTypeMaskShift) - 1)),
              "kLibusbIsoSyncTypeMaskShift constant is wrong");
static_assert((LIBUSB_ISO_SYNC_TYPE_MASK >> kLibusbIsoSyncTypeMaskShift) & 1,
              "kLibusbIsoSyncTypeMaskShift constant is wrong");

constexpr int kLibusbIsoUsageTypeMaskShift = 4;
static_assert(!(LIBUSB_ISO_USAGE_TYPE_MASK &
                ((1 << kLibusbIsoUsageTypeMaskShift) - 1)),
              "kLibusbIsoUsageTypeMaskShift constant is wrong");
static_assert((LIBUSB_ISO_USAGE_TYPE_MASK >> kLibusbIsoUsageTypeMaskShift) & 1,
              "kLibusbIsoUsageTypeMaskShift constant is wrong");

// Mask for libusb_request_recipient bits in bmRequestType field of the
// libusb_control_setup structure.
constexpr int kLibusbRequestRecipientMask =
    LIBUSB_RECIPIENT_DEVICE | LIBUSB_RECIPIENT_INTERFACE |
    LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_RECIPIENT_OTHER;

// Mask for libusb_request_type bits in bmRequestType field of the
// libusb_control_setup structure.
constexpr int kLibusbRequestTypeMask =
    LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_REQUEST_TYPE_CLASS |
    LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_REQUEST_TYPE_RESERVED;

std::unique_ptr<uint8_t[]> CopyRawData(const uint8_t* data, size_t byte_count) {
  std::unique_ptr<uint8_t[]> result(new uint8_t[byte_count]);
  std::copy(data, data + byte_count, result.get());
  return result;
}

std::unique_ptr<uint8_t[]> CopyRawData(const std::vector<uint8_t>& data) {
  if (data.empty())
    return nullptr;
  return CopyRawData(&data[0], data.size());
}

libusb_context* GetLibusbTransferContext(const libusb_transfer* transfer) {
  if (!transfer)
    return nullptr;
  libusb_device_handle* const device_handle = transfer->dev_handle;
  if (!device_handle)
    return nullptr;
  return device_handle->context();
}

libusb_context* GetLibusbTransferContextChecked(
    const libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  libusb_context* const result = GetLibusbTransferContext(transfer);
  GOOGLE_SMART_CARD_CHECK(result);
  return result;
}

// TODO(#429): Delete this converter once all code is switched to libusb_js.
chrome_usb::Device ConvertLibusbJsDeviceToChromeUsb(
    const LibusbJsDevice& js_device) {
  chrome_usb::Device chrome_usb_device;
  chrome_usb_device.device = js_device.device_id;
  chrome_usb_device.vendor_id = js_device.vendor_id;
  chrome_usb_device.product_id = js_device.product_id;
  chrome_usb_device.version = js_device.version;
  chrome_usb_device.product_name =
      js_device.product_name ? *js_device.product_name : "";
  chrome_usb_device.manufacturer_name =
      js_device.manufacturer_name ? *js_device.manufacturer_name : "";
  chrome_usb_device.serial_number =
      js_device.serial_number ? *js_device.serial_number : "";
  return chrome_usb_device;
}

}  // namespace

LibusbJsProxy::LibusbJsProxy(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router,
    chrome_usb::ApiBridgeInterface* chrome_usb_api_bridge)
    : js_requester_(kJsRequesterName, global_context, typed_message_router),
      js_call_adaptor_(&js_requester_),
      chrome_usb_api_bridge_(chrome_usb_api_bridge),
      default_context_(contexts_storage_.CreateContext()) {
  GOOGLE_SMART_CARD_CHECK(chrome_usb_api_bridge_);
}

LibusbJsProxy::~LibusbJsProxy() = default;

void LibusbJsProxy::ShutDown() {
  js_requester_.ShutDown();
}

int LibusbJsProxy::LibusbInit(libusb_context** ctx) {
  // If the default context was requested, nothing is done (it's always existing
  // and initialized as long as this class object is alive).
  if (ctx)
    *ctx = contexts_storage_.CreateContext().get();
  return LIBUSB_SUCCESS;
}

void LibusbJsProxy::LibusbExit(libusb_context* ctx) {
  // If the default context deinitialization was requested, nothing is done
  // (it's always kept initialized as long as this class object is alive).
  if (ctx)
    contexts_storage_.DestroyContext(ctx);
}

ssize_t LibusbJsProxy::LibusbGetDeviceList(libusb_context* ctx,
                                           libusb_device*** list) {
  GOOGLE_SMART_CARD_CHECK(list);

  ctx = SubstituteDefaultContextIfNull(ctx);

  GenericRequestResult request_result =
      js_call_adaptor_.SyncCall(kJsRequestListDevices);
  std::string error_message;
  std::vector<LibusbJsDevice> js_devices;
  if (!RemoteCallAdaptor::ExtractResultPayload(std::move(request_result),
                                               &error_message, &js_devices)) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbGetDeviceList request failed: "
        << error_message;
    return LIBUSB_ERROR_OTHER;
  }

  *list = new libusb_device*[js_devices.size() + 1];
  for (size_t index = 0; index < js_devices.size(); ++index)
    (*list)[index] = new libusb_device(ctx, js_devices[index]);

  // The resulting list must be NULL-terminated according to the libusb
  // documentation.
  (*list)[js_devices.size()] = nullptr;

  return js_devices.size();
}

void LibusbJsProxy::LibusbFreeDeviceList(libusb_device** list,
                                         int unref_devices) {
  if (!list)
    return;
  if (unref_devices) {
    for (size_t index = 0; list[index]; ++index)
      LibusbUnrefDevice(list[index]);
  }
  delete[] list;
}

libusb_device* LibusbJsProxy::LibusbRefDevice(libusb_device* dev) {
  GOOGLE_SMART_CARD_CHECK(dev);

  dev->AddReference();

  return dev;
}

void LibusbJsProxy::LibusbUnrefDevice(libusb_device* dev) {
  GOOGLE_SMART_CARD_CHECK(dev);

  dev->RemoveReference();
}

namespace {

uint8_t ChromeUsbEndpointDescriptorTypeToLibusbMask(
    chrome_usb::EndpointDescriptorType value) {
  switch (value) {
    case chrome_usb::EndpointDescriptorType::kControl:
      return LIBUSB_TRANSFER_TYPE_CONTROL << kLibusbTransferTypeMaskShift;
    case chrome_usb::EndpointDescriptorType::kInterrupt:
      return LIBUSB_TRANSFER_TYPE_INTERRUPT << kLibusbTransferTypeMaskShift;
    case chrome_usb::EndpointDescriptorType::kIsochronous:
      return LIBUSB_TRANSFER_TYPE_ISOCHRONOUS << kLibusbTransferTypeMaskShift;
    case chrome_usb::EndpointDescriptorType::kBulk:
      return LIBUSB_TRANSFER_TYPE_BULK << kLibusbTransferTypeMaskShift;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }
}

uint8_t ChromeUsbEndpointDescriptorSynchronizationToLibusbMask(
    chrome_usb::EndpointDescriptorSynchronization value) {
  switch (value) {
    case chrome_usb::EndpointDescriptorSynchronization::kAsynchronous:
      return LIBUSB_ISO_SYNC_TYPE_ASYNC << kLibusbIsoSyncTypeMaskShift;
    case chrome_usb::EndpointDescriptorSynchronization::kAdaptive:
      return LIBUSB_ISO_SYNC_TYPE_ADAPTIVE << kLibusbIsoSyncTypeMaskShift;
    case chrome_usb::EndpointDescriptorSynchronization::kSynchronous:
      return LIBUSB_ISO_SYNC_TYPE_SYNC << kLibusbIsoSyncTypeMaskShift;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }
}

uint8_t ChromeUsbEndpointDescriptorUsageToLibusbMask(
    chrome_usb::EndpointDescriptorUsage value) {
  switch (value) {
    case chrome_usb::EndpointDescriptorUsage::kData:
      return LIBUSB_ISO_USAGE_TYPE_DATA << kLibusbIsoUsageTypeMaskShift;
    case chrome_usb::EndpointDescriptorUsage::kFeedback:
      return LIBUSB_ISO_USAGE_TYPE_FEEDBACK << kLibusbIsoUsageTypeMaskShift;
    case chrome_usb::EndpointDescriptorUsage::kExplicitFeedback:
      return LIBUSB_ISO_USAGE_TYPE_IMPLICIT << kLibusbIsoUsageTypeMaskShift;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }
}

void FillLibusbEndpointDescriptor(
    const chrome_usb::EndpointDescriptor& chrome_usb_descriptor,
    libusb_endpoint_descriptor* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  std::memset(result, 0, sizeof(libusb_endpoint_descriptor));

  result->bLength = sizeof(libusb_endpoint_descriptor);

  result->bDescriptorType = LIBUSB_DT_ENDPOINT;

  result->bEndpointAddress = chrome_usb_descriptor.address;

  result->bmAttributes |=
      ChromeUsbEndpointDescriptorTypeToLibusbMask(chrome_usb_descriptor.type);
  if (chrome_usb_descriptor.type ==
      chrome_usb::EndpointDescriptorType::kIsochronous) {
    GOOGLE_SMART_CARD_CHECK(chrome_usb_descriptor.synchronization);
    result->bmAttributes |=
        ChromeUsbEndpointDescriptorSynchronizationToLibusbMask(
            *chrome_usb_descriptor.synchronization);
    GOOGLE_SMART_CARD_CHECK(chrome_usb_descriptor.usage);
    result->bmAttributes |= ChromeUsbEndpointDescriptorUsageToLibusbMask(
        *chrome_usb_descriptor.usage);
  }

  result->wMaxPacketSize = chrome_usb_descriptor.maximum_packet_size;

  if (chrome_usb_descriptor.polling_interval)
    result->bInterval = *chrome_usb_descriptor.polling_interval;

  result->extra = CopyRawData(chrome_usb_descriptor.extra_data).release();

  result->extra_length = chrome_usb_descriptor.extra_data.size();
}

void FillLibusbInterfaceDescriptor(
    const chrome_usb::InterfaceDescriptor& chrome_usb_descriptor,
    libusb_interface_descriptor* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  std::memset(result, 0, sizeof(libusb_interface_descriptor));

  result->bLength = sizeof(libusb_interface_descriptor);

  result->bDescriptorType = LIBUSB_DT_INTERFACE;

  result->bInterfaceNumber = chrome_usb_descriptor.interface_number;

  result->bNumEndpoints = chrome_usb_descriptor.endpoints.size();

  result->bInterfaceClass = chrome_usb_descriptor.interface_class;

  result->bInterfaceSubClass = chrome_usb_descriptor.interface_subclass;

  result->bInterfaceProtocol = chrome_usb_descriptor.interface_protocol;

  result->endpoint =
      new libusb_endpoint_descriptor[chrome_usb_descriptor.endpoints.size()];
  for (size_t index = 0; index < chrome_usb_descriptor.endpoints.size();
       ++index) {
    FillLibusbEndpointDescriptor(
        chrome_usb_descriptor.endpoints[index],
        const_cast<libusb_endpoint_descriptor*>(&result->endpoint[index]));
  }

  result->extra = CopyRawData(chrome_usb_descriptor.extra_data).release();

  result->extra_length = chrome_usb_descriptor.extra_data.size();
}

void FillLibusbInterface(
    const chrome_usb::InterfaceDescriptor& chrome_usb_descriptor,
    libusb_interface* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  result->altsetting = new libusb_interface_descriptor[1];
  FillLibusbInterfaceDescriptor(
      chrome_usb_descriptor,
      const_cast<libusb_interface_descriptor*>(result->altsetting));

  result->num_altsetting = 1;
}

void FillLibusbConfigDescriptor(
    const chrome_usb::ConfigDescriptor& chrome_usb_descriptor,
    libusb_config_descriptor* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  std::memset(result, 0, sizeof(libusb_config_descriptor));

  result->bLength = sizeof(libusb_config_descriptor);

  result->bDescriptorType = LIBUSB_DT_CONFIG;

  result->wTotalLength = sizeof(libusb_config_descriptor);

  result->bNumInterfaces = chrome_usb_descriptor.interfaces.size();

  result->bConfigurationValue = chrome_usb_descriptor.configuration_value;

  if (chrome_usb_descriptor.remote_wakeup)
    result->bmAttributes |= kLibusbConfigDescriptorBmAttributesRemoteWakeup;
  if (chrome_usb_descriptor.self_powered)
    result->bmAttributes |= kLibusbConfigDescriptorBmAttributesSelfPowered;

  result->MaxPower = chrome_usb_descriptor.max_power;

  result->interface =
      new libusb_interface[chrome_usb_descriptor.interfaces.size()];
  for (size_t index = 0; index < chrome_usb_descriptor.interfaces.size();
       ++index) {
    FillLibusbInterface(
        chrome_usb_descriptor.interfaces[index],
        const_cast<libusb_interface*>(&result->interface[index]));
  }

  result->extra = CopyRawData(chrome_usb_descriptor.extra_data).release();

  result->extra_length = chrome_usb_descriptor.extra_data.size();
}

}  // namespace

int LibusbJsProxy::LibusbGetActiveConfigDescriptor(
    libusb_device* dev,
    libusb_config_descriptor** config) {
  GOOGLE_SMART_CARD_CHECK(dev);
  GOOGLE_SMART_CARD_CHECK(config);

  const RequestResult<chrome_usb::GetConfigurationsResult> result =
      chrome_usb_api_bridge_->GetConfigurations(
          ConvertLibusbJsDeviceToChromeUsb(dev->js_device()));
  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbGetActiveConfigDescriptor request "
        << "failed: " << result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  const std::vector<chrome_usb::ConfigDescriptor>& chrome_usb_configs =
      result.payload().configurations;

  *config = nullptr;
  for (const auto& chrome_usb_config : chrome_usb_configs) {
    if (chrome_usb_config.active) {
      if (*config) {
        // Normally there should be only one active configuration, but the
        // chrome.usb API implementation misbehaves on some devices: see
        // <https://crbug.com/1218141>. As a workaround, proceed with the first
        // received configuration that's marked as active.
        GOOGLE_SMART_CARD_LOG_WARNING
            << "Unexpected result from the chrome.usb.getConfigurations() API: "
               "received multiple active configurations";
        break;
      }

      *config = new libusb_config_descriptor;
      FillLibusbConfigDescriptor(chrome_usb_config, *config);
    }
  }
  if (!*config) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbGetActiveConfigDescriptor request "
        << "failed: No active config descriptors were returned by chrome.usb "
           "API";
    return LIBUSB_ERROR_OTHER;
  }
  return LIBUSB_SUCCESS;
}

namespace {

void DestroyLibusbEndpointDescriptor(
    const libusb_endpoint_descriptor& endpoint_descriptor) {
  delete[] endpoint_descriptor.extra;
}

void DestroyLibusbInterfaceDescriptor(
    const libusb_interface_descriptor& interface_descriptor) {
  for (unsigned index = 0; index < interface_descriptor.bNumEndpoints;
       ++index) {
    DestroyLibusbEndpointDescriptor(interface_descriptor.endpoint[index]);
  }
  delete[] interface_descriptor.endpoint;

  delete[] interface_descriptor.extra;
}

void DestroyLibusbInterface(const libusb_interface& interface) {
  for (int index = 0; index < interface.num_altsetting; ++index) {
    DestroyLibusbInterfaceDescriptor(interface.altsetting[index]);
  }
  delete[] interface.altsetting;
}

void DestroyLibusbConfigDescriptor(
    const libusb_config_descriptor& config_descriptor) {
  for (uint8_t index = 0; index < config_descriptor.bNumInterfaces; ++index) {
    DestroyLibusbInterface(config_descriptor.interface[index]);
  }
  delete[] config_descriptor.interface;

  delete[] config_descriptor.extra;
}

}  // namespace

void LibusbJsProxy::LibusbFreeConfigDescriptor(
    libusb_config_descriptor* config) {
  if (!config)
    return;
  DestroyLibusbConfigDescriptor(*config);
  delete config;
}

namespace {

void FillLibusbDeviceDescriptor(const LibusbJsDevice& js_device,
                                libusb_device_descriptor* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  std::memset(result, 0, sizeof(libusb_device_descriptor));

  result->bLength = sizeof(libusb_device_descriptor);

  result->bDescriptorType = LIBUSB_DT_DEVICE;

  result->idVendor = js_device.vendor_id;

  result->idProduct = js_device.product_id;

  if (js_device.version) {
    // When using the chrome.usb API, the version field is filled only in
    // Chrome >= 51 (see <http://crbug.com/598825>).
    result->bcdDevice = *js_device.version;
  }

  //
  // The JS APIs also provide information about the product name,
  // manufacturer name and serial number. However, it's difficult to pass this
  // information to consumers here, because the corresponding
  // libusb_device_descriptor structure fields (iProduct, iManufacturer,
  // iSerialNumber) should contain not the strings themselves, but their indexes
  // instead. The string indexes, however, are not provided by chrome.usb API.
  //
  // One solution would be to use a generated string indexes here and patch the
  // inline libusb_get_string_descriptor function. But avoiding the collisions
  // of the generated string indexes with some other existing ones is difficult.
  // Moreover, this solution would still keep some incompatibility with the
  // original libusb interface, as the consumers could tried reading the strings
  // by performing the corresponding control transfers themselves instead of
  // using libusb_get_string_descriptor function - which will obviously fail.
  //
  // Another, more correct, solution would be to iterate here over all possible
  // string indexes and therefore match the strings returned by chrome.usb API
  // with their original string indexes. But this solution has an obvious
  // drawback of performance penalty; also some devices bugs may be hit in this
  // solution.
  //
  // That's why it's currently decided to not populate these
  // libusb_device_descriptor structure fields at all.
  //
}

}  // namespace

int LibusbJsProxy::LibusbGetDeviceDescriptor(libusb_device* dev,
                                             libusb_device_descriptor* desc) {
  GOOGLE_SMART_CARD_CHECK(dev);
  GOOGLE_SMART_CARD_CHECK(desc);

  FillLibusbDeviceDescriptor(dev->js_device(), desc);
  return LIBUSB_SUCCESS;
}

uint8_t LibusbJsProxy::LibusbGetBusNumber(libusb_device* dev) {
  auto bus_numbers_iterator = bus_numbers_.find(dev->js_device().device_id);
  if (bus_numbers_iterator != bus_numbers_.end()) {
    return bus_numbers_iterator->second;
  }
  return kDefaultBusNumber;
}

uint8_t LibusbJsProxy::LibusbGetDeviceAddress(libusb_device* dev) {
  GOOGLE_SMART_CARD_CHECK(dev);

  const int64_t device_id = dev->js_device().device_id;
  // FIXME(emaxx): Fix the implementation to re-use the free device identifiers.
  // The current implementation will break, for instance, after a device is
  // unplugged and plugged back 256 times.
  GOOGLE_SMART_CARD_CHECK(device_id < std::numeric_limits<uint8_t>::max());
  return static_cast<uint8_t>(device_id);
}

int LibusbJsProxy::LibusbOpen(libusb_device* dev,
                              libusb_device_handle** handle) {
  GOOGLE_SMART_CARD_CHECK(dev);
  GOOGLE_SMART_CARD_CHECK(handle);

  const RequestResult<chrome_usb::OpenDeviceResult> result =
      chrome_usb_api_bridge_->OpenDevice(
          ConvertLibusbJsDeviceToChromeUsb(dev->js_device()));
  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbOpen "
        << "request failed: " << result.error_message();
    // Modify the devices (fake) bus number that we report so that PCSC will
    // retry to connect to the device once it updates the device list.
    uint32_t new_bus_number =
        static_cast<uint32_t>(LibusbGetBusNumber(dev)) + 1;
    if (new_bus_number <= kMaximumBusNumber) {
      bus_numbers_[dev->js_device().device_id] = new_bus_number;
    }
    return LIBUSB_ERROR_OTHER;
  }
  const chrome_usb::ConnectionHandle chrome_usb_connection_handle =
      result.payload().connection_handle;

  *handle = new libusb_device_handle(dev, chrome_usb_connection_handle);
  return LIBUSB_SUCCESS;
}

void LibusbJsProxy::LibusbClose(libusb_device_handle* handle) {
  GOOGLE_SMART_CARD_CHECK(handle);

  const RequestResult<chrome_usb::CloseDeviceResult> result =
      chrome_usb_api_bridge_->CloseDevice(
          handle->chrome_usb_connection_handle());
  if (!result.is_successful()) {
    // It's essential to not crash in this case, because this may happen during
    // shutdown process.
    GOOGLE_SMART_CARD_LOG_ERROR << "Failed to close USB device";
    return;
  }

  delete handle;
}

int LibusbJsProxy::LibusbClaimInterface(libusb_device_handle* dev,
                                        int interface_number) {
  GOOGLE_SMART_CARD_CHECK(dev);

  const RequestResult<chrome_usb::ClaimInterfaceResult> result =
      chrome_usb_api_bridge_->ClaimInterface(
          dev->chrome_usb_connection_handle(), interface_number);
  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbClaimInterface request failed: "
        << result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  return LIBUSB_SUCCESS;
}

int LibusbJsProxy::LibusbReleaseInterface(libusb_device_handle* dev,
                                          int interface_number) {
  GOOGLE_SMART_CARD_CHECK(dev);

  const RequestResult<chrome_usb::ReleaseInterfaceResult> result =
      chrome_usb_api_bridge_->ReleaseInterface(
          dev->chrome_usb_connection_handle(), interface_number);
  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbReleaseInterface request failed: "
        << result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  return LIBUSB_SUCCESS;
}

int LibusbJsProxy::LibusbResetDevice(libusb_device_handle* dev) {
  GOOGLE_SMART_CARD_CHECK(dev);

  const RequestResult<chrome_usb::ResetDeviceResult> result =
      chrome_usb_api_bridge_->ResetDevice(dev->chrome_usb_connection_handle());
  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbResetDevice request failed: "
        << result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  return LIBUSB_SUCCESS;
}

libusb_transfer* LibusbJsProxy::LibusbAllocTransfer(int iso_packets) {
  // Isochronous transfers are not supported.
  GOOGLE_SMART_CARD_CHECK(!iso_packets);

  libusb_transfer* const result = new libusb_transfer;
  std::memset(result, 0, sizeof(libusb_transfer));

  return result;
}

namespace {

bool CreateChromeUsbControlTransferInfo(
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    unsigned char* data,
    uint16_t length,
    unsigned timeout,
    chrome_usb::ControlTransferInfo* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  result->direction =
      ((request_type & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT)
          ? chrome_usb::Direction::kOut
          : chrome_usb::Direction::kIn;

  switch (request_type & kLibusbRequestRecipientMask) {
    case LIBUSB_RECIPIENT_DEVICE:
      result->recipient = chrome_usb::ControlTransferInfoRecipient::kDevice;
      break;
    case LIBUSB_RECIPIENT_INTERFACE:
      result->recipient = chrome_usb::ControlTransferInfoRecipient::kInterface;
      break;
    case LIBUSB_RECIPIENT_ENDPOINT:
      result->recipient = chrome_usb::ControlTransferInfoRecipient::kEndpoint;
      break;
    case LIBUSB_RECIPIENT_OTHER:
      result->recipient = chrome_usb::ControlTransferInfoRecipient::kOther;
      break;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }

  switch (request_type & kLibusbRequestTypeMask) {
    case LIBUSB_REQUEST_TYPE_STANDARD:
      result->request_type =
          chrome_usb::ControlTransferInfoRequestType::kStandard;
      break;
    case LIBUSB_REQUEST_TYPE_CLASS:
      result->request_type = chrome_usb::ControlTransferInfoRequestType::kClass;
      break;
    case LIBUSB_REQUEST_TYPE_VENDOR:
      result->request_type =
          chrome_usb::ControlTransferInfoRequestType::kVendor;
      break;
    case LIBUSB_REQUEST_TYPE_RESERVED:
      result->request_type =
          chrome_usb::ControlTransferInfoRequestType::kReserved;
      break;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }

  result->request = request;

  result->value = libusb_le16_to_cpu(value);

  result->index = libusb_le16_to_cpu(index);

  if (result->direction == chrome_usb::Direction::kIn)
    result->length = length;

  if (result->direction == chrome_usb::Direction::kOut) {
    GOOGLE_SMART_CARD_CHECK(data);
    result->data = std::vector<uint8_t>(data, data + length);
  }

  result->timeout = timeout;

  return true;
}

bool CreateChromeUsbControlTransferInfo(
    libusb_transfer* transfer,
    chrome_usb::ControlTransferInfo* result) {
  GOOGLE_SMART_CARD_CHECK(transfer);
  GOOGLE_SMART_CARD_CHECK(transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL);
  GOOGLE_SMART_CARD_CHECK(result);

  //
  // Control-specific setup fields are kept in a special libusb_control_setup
  // structure placed in the beginning of the buffer; the real payload, that
  // will be sent to the Chrome USB API, is located further in the buffer (see
  // the convenience functions libusb_control_transfer_get_setup() and
  // libusb_control_transfer_get_data()).
  //
  // Note that the structure fields, according to the documentation, are
  // always stored in the little-endian byte order, so accesses to the
  // multi-byte fields (wValue, wIndex and wLength) must be carefully wrapped
  // through libusb_le16_to_cpu() macro.
  //

  if (transfer->length < 0 ||
      static_cast<size_t>(transfer->length) < LIBUSB_CONTROL_SETUP_SIZE) {
    return false;
  }

  const libusb_control_setup* const control_setup =
      libusb_control_transfer_get_setup(transfer);

  const uint16_t data_length = libusb_le16_to_cpu(control_setup->wLength);
  if (data_length != transfer->length - LIBUSB_CONTROL_SETUP_SIZE)
    return false;

  return CreateChromeUsbControlTransferInfo(
      control_setup->bmRequestType, control_setup->bRequest,
      libusb_le16_to_cpu(control_setup->wValue),
      libusb_le16_to_cpu(control_setup->wIndex),
      libusb_control_transfer_get_data(transfer), data_length,
      transfer->timeout, result);
}

void CreateChromeUsbGenericTransferInfo(
    unsigned char endpoint_address,
    unsigned char* data,
    int length,
    unsigned timeout,
    chrome_usb::GenericTransferInfo* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  result->direction =
      ((endpoint_address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT)
          ? chrome_usb::Direction::kOut
          : chrome_usb::Direction::kIn;

  result->endpoint = endpoint_address;

  if (result->direction == chrome_usb::Direction::kIn)
    result->length = length;

  if (result->direction == chrome_usb::Direction::kOut) {
    GOOGLE_SMART_CARD_CHECK(data);
    result->data = std::vector<uint8_t>(data, data + length);
  }

  result->timeout = timeout;
}

void CreateChromeUsbGenericTransferInfo(
    libusb_transfer* transfer,
    chrome_usb::GenericTransferInfo* result) {
  GOOGLE_SMART_CARD_CHECK(transfer);
  GOOGLE_SMART_CARD_CHECK(transfer->type == LIBUSB_TRANSFER_TYPE_BULK ||
                          transfer->type == LIBUSB_TRANSFER_TYPE_INTERRUPT);
  GOOGLE_SMART_CARD_CHECK(result);

  CreateChromeUsbGenericTransferInfo(transfer->endpoint, transfer->buffer,
                                     transfer->length, transfer->timeout,
                                     result);
}

chrome_usb::AsyncTransferCallback MakeChromeUsbTransferCallback(
    std::weak_ptr<libusb_context> context,
    const UsbTransferDestination& transfer_destination,
    LibusbJsProxy::TransferAsyncRequestStatePtr async_request_state) {
  return [context, transfer_destination, async_request_state](
             RequestResult<chrome_usb::TransferResult> request_result) {
    const std::shared_ptr<libusb_context> locked_context = context.lock();
    if (!locked_context) {
      // The context that was used for the original transfer submission has been
      // destroyed already.
      return;
    }

    if (transfer_destination.IsInputDirection()) {
      locked_context->OnInputTransferResultReceived(transfer_destination,
                                                    std::move(request_result));
    } else {
      locked_context->OnOutputTransferResultReceived(async_request_state,
                                                     std::move(request_result));
    }
  };
}

}  // namespace

int LibusbJsProxy::LibusbSubmitTransfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);
  GOOGLE_SMART_CARD_CHECK(transfer->dev_handle);

  // Isochronous transfers are not supported.
  GOOGLE_SMART_CARD_CHECK(transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL ||
                          transfer->type == LIBUSB_TRANSFER_TYPE_BULK ||
                          transfer->type == LIBUSB_TRANSFER_TYPE_INTERRUPT);

  if (transfer->flags & LIBUSB_TRANSFER_ADD_ZERO_PACKET) {
    // Don't bother with this libusb feature (it's not even supported by it on
    // many platforms).
    return LIBUSB_ERROR_NOT_SUPPORTED;
  }

  libusb_context* const context = GetLibusbTransferContextChecked(transfer);

  const auto async_request_state = std::make_shared<TransferAsyncRequestState>(
      WrapLibusbTransferCallback(transfer));

  chrome_usb::GenericTransferInfo generic_transfer_info;
  chrome_usb::ControlTransferInfo control_transfer_info;
  switch (transfer->type) {
    case LIBUSB_TRANSFER_TYPE_CONTROL:
      if (!CreateChromeUsbControlTransferInfo(transfer, &control_transfer_info))
        return LIBUSB_ERROR_INVALID_PARAM;
      break;
    case LIBUSB_TRANSFER_TYPE_BULK:
      CreateChromeUsbGenericTransferInfo(transfer, &generic_transfer_info);
      break;
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
      CreateChromeUsbGenericTransferInfo(transfer, &generic_transfer_info);
      break;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }

  UsbTransferDestination transfer_destination;
  if (transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL) {
    transfer_destination =
        UsbTransferDestination::CreateFromChromeUsbControlTransfer(
            transfer->dev_handle->chrome_usb_connection_handle(),
            control_transfer_info);
  } else {
    transfer_destination =
        UsbTransferDestination::CreateFromChromeUsbGenericTransfer(
            transfer->dev_handle->chrome_usb_connection_handle(),
            generic_transfer_info);
  }

  context->AddAsyncTransferInFlight(async_request_state, transfer_destination,
                                    transfer);

  const auto chrome_usb_callback = MakeChromeUsbTransferCallback(
      contexts_storage_.FindContextByAddress(context), transfer_destination,
      async_request_state);

  switch (transfer->type) {
    case LIBUSB_TRANSFER_TYPE_CONTROL:
      chrome_usb_api_bridge_->AsyncControlTransfer(
          transfer->dev_handle->chrome_usb_connection_handle(),
          control_transfer_info, chrome_usb_callback);
      break;
    case LIBUSB_TRANSFER_TYPE_BULK:
      chrome_usb_api_bridge_->AsyncBulkTransfer(
          transfer->dev_handle->chrome_usb_connection_handle(),
          generic_transfer_info, chrome_usb_callback);
      break;
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
      chrome_usb_api_bridge_->AsyncInterruptTransfer(
          transfer->dev_handle->chrome_usb_connection_handle(),
          generic_transfer_info, chrome_usb_callback);
      break;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }

  return LIBUSB_SUCCESS;
}

int LibusbJsProxy::LibusbCancelTransfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  libusb_context* const context = GetLibusbTransferContextChecked(transfer);
  return context->CancelTransfer(transfer) ? LIBUSB_SUCCESS
                                           : LIBUSB_ERROR_NOT_FOUND;
}

void LibusbJsProxy::LibusbFreeTransfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  if (transfer->flags & LIBUSB_TRANSFER_FREE_BUFFER)
    ::free(transfer->buffer);
  delete transfer;
}

namespace {

libusb_transfer_status FillLibusbTransferResult(
    const chrome_usb::TransferResultInfo& transfer_result_info,
    bool is_short_not_ok,
    int data_length,
    unsigned char* data_buffer,
    int* actual_length) {
  if (!transfer_result_info.result_code)
    return LIBUSB_TRANSFER_ERROR;
  if (*transfer_result_info.result_code !=
      chrome_usb::kTransferResultInfoSuccessResultCode) {
    return LIBUSB_TRANSFER_ERROR;
  }

  // FIXME(emaxx): Looks like chrome.usb API returns timeout results as if they
  // were errors. So, in case of timeout, LIBUSB_TRANSFER_ERROR will be
  // returned to the consumers instead of returning LIBUSB_TRANSFER_TIMED_OUT.
  // This doesn't look like a huge problem, but still, from the sanity
  // prospective, this probably requires fixing.

  int actual_length_value;
  if (transfer_result_info.data) {
    actual_length_value = std::min(
        static_cast<int>(transfer_result_info.data->size()), data_length);
    if (actual_length_value) {
      std::copy_n(transfer_result_info.data->begin(), actual_length_value,
                  data_buffer);
    }
  } else {
    actual_length_value = data_length;
  }

  if (actual_length)
    *actual_length = actual_length_value;

  if (is_short_not_ok && actual_length_value < data_length)
    return LIBUSB_TRANSFER_ERROR;
  return LIBUSB_TRANSFER_COMPLETED;
}

int LibusbTransferStatusToLibusbErrorCode(
    libusb_transfer_status transfer_status) {
  switch (transfer_status) {
    case LIBUSB_TRANSFER_COMPLETED:
      return LIBUSB_SUCCESS;
    case LIBUSB_TRANSFER_TIMED_OUT:
      return LIBUSB_ERROR_TIMEOUT;
    default:
      return LIBUSB_ERROR_OTHER;
  }
}

}  // namespace

int LibusbJsProxy::LibusbControlTransfer(libusb_device_handle* dev,
                                         uint8_t bmRequestType,
                                         uint8_t bRequest,
                                         uint16_t wValue,
                                         uint16_t index,
                                         unsigned char* data,
                                         uint16_t wLength,
                                         unsigned timeout) {
  GOOGLE_SMART_CARD_CHECK(dev);

  chrome_usb::ControlTransferInfo transfer_info;
  if (!CreateChromeUsbControlTransferInfo(bmRequestType, bRequest, wValue,
                                          index, data, wLength, timeout,
                                          &transfer_info)) {
    return LIBUSB_ERROR_INVALID_PARAM;
  }
  const UsbTransferDestination transfer_destination =
      UsbTransferDestination::CreateFromChromeUsbControlTransfer(
          dev->chrome_usb_connection_handle(), transfer_info);
  SyncTransferHelper sync_transfer_helper(
      contexts_storage_.FindContextByAddress(dev->context()),
      transfer_destination);

  chrome_usb_api_bridge_->AsyncControlTransfer(
      dev->chrome_usb_connection_handle(), transfer_info,
      sync_transfer_helper.chrome_usb_transfer_callback());
  const TransferRequestResult result = sync_transfer_helper.WaitForCompletion();

  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbControlTransfer request failed: "
        << result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  int actual_length;
  const int error_code =
      LibusbTransferStatusToLibusbErrorCode(FillLibusbTransferResult(
          result.payload().result_info, false, wLength, data, &actual_length));
  if (error_code == LIBUSB_SUCCESS)
    return actual_length;
  return error_code;
}

int LibusbJsProxy::LibusbBulkTransfer(libusb_device_handle* dev,
                                      unsigned char endpoint_address,
                                      unsigned char* data,
                                      int length,
                                      int* actual_length,
                                      unsigned timeout) {
  GOOGLE_SMART_CARD_CHECK(dev);

  chrome_usb::GenericTransferInfo transfer_info;
  CreateChromeUsbGenericTransferInfo(endpoint_address, data, length, timeout,
                                     &transfer_info);
  const UsbTransferDestination transfer_destination =
      UsbTransferDestination::CreateFromChromeUsbGenericTransfer(
          dev->chrome_usb_connection_handle(), transfer_info);
  SyncTransferHelper sync_transfer_helper(
      contexts_storage_.FindContextByAddress(dev->context()),
      transfer_destination);

  chrome_usb_api_bridge_->AsyncBulkTransfer(
      dev->chrome_usb_connection_handle(), transfer_info,
      sync_transfer_helper.chrome_usb_transfer_callback());
  const TransferRequestResult result = sync_transfer_helper.WaitForCompletion();

  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbBulkTransfer request failed: "
        << result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  return LibusbTransferStatusToLibusbErrorCode(FillLibusbTransferResult(
      result.payload().result_info, false, length, data, actual_length));
}

int LibusbJsProxy::LibusbInterruptTransfer(libusb_device_handle* dev,
                                           unsigned char endpoint_address,
                                           unsigned char* data,
                                           int length,
                                           int* actual_length,
                                           unsigned timeout) {
  GOOGLE_SMART_CARD_CHECK(dev);

  chrome_usb::GenericTransferInfo transfer_info;
  CreateChromeUsbGenericTransferInfo(endpoint_address, data, length, timeout,
                                     &transfer_info);
  const UsbTransferDestination transfer_destination =
      UsbTransferDestination::CreateFromChromeUsbGenericTransfer(
          dev->chrome_usb_connection_handle(), transfer_info);
  SyncTransferHelper sync_transfer_helper(
      contexts_storage_.FindContextByAddress(dev->context()),
      transfer_destination);

  chrome_usb_api_bridge_->AsyncInterruptTransfer(
      dev->chrome_usb_connection_handle(), transfer_info,
      sync_transfer_helper.chrome_usb_transfer_callback());
  const TransferRequestResult result = sync_transfer_helper.WaitForCompletion();

  if (!result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbJsProxy::LibusbInterruptTransfer request failed: "
        << result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  return LibusbTransferStatusToLibusbErrorCode(FillLibusbTransferResult(
      result.payload().result_info, false, length, data, actual_length));
}

int LibusbJsProxy::LibusbHandleEvents(libusb_context* ctx) {
  return LibusbHandleEventsWithTimeout(ctx, kHandleEventsTimeoutSeconds);
}

int LibusbJsProxy::LibusbHandleEventsCompleted(libusb_context* ctx,
                                               int* completed) {
  ctx = SubstituteDefaultContextIfNull(ctx);

  ctx->WaitAndProcessAsyncTransferReceivedResults(
      std::chrono::time_point<std::chrono::high_resolution_clock>::max(),
      completed);
  return LIBUSB_SUCCESS;
}

LibusbJsProxy::SyncTransferHelper::SyncTransferHelper(
    std::shared_ptr<libusb_context> context,
    const UsbTransferDestination& transfer_destination)
    : context_(context), transfer_destination_(transfer_destination) {
  const auto async_request_state_callback =
      [this](LibusbJsProxy::TransferRequestResult request_result) {
        result_ = std::move(request_result);
      };

  async_request_state_ =
      std::make_shared<TransferAsyncRequestState>(async_request_state_callback);

  context->AddSyncTransferInFlight(async_request_state_, transfer_destination_);
}

LibusbJsProxy::SyncTransferHelper::~SyncTransferHelper() = default;

chrome_usb::AsyncTransferCallback
LibusbJsProxy::SyncTransferHelper::chrome_usb_transfer_callback() const {
  return MakeChromeUsbTransferCallback(context_, transfer_destination_,
                                       async_request_state_);
}

RequestResult<chrome_usb::TransferResult>
LibusbJsProxy::SyncTransferHelper::WaitForCompletion() {
  if (transfer_destination_.IsInputDirection()) {
    context_->WaitAndProcessInputSyncTransferReceivedResult(
        async_request_state_, transfer_destination_);
  } else {
    context_->WaitAndProcessOutputSyncTransferReceivedResult(
        async_request_state_);
  }
  return std::move(result_);
}

libusb_context* LibusbJsProxy::SubstituteDefaultContextIfNull(
    libusb_context* context_or_nullptr) const {
  if (context_or_nullptr)
    return context_or_nullptr;
  return default_context_.get();
}

LibusbJsProxy::TransferAsyncRequestCallback
LibusbJsProxy::WrapLibusbTransferCallback(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  return [this,
          transfer](RequestResult<chrome_usb::TransferResult> request_result) {
    if (request_result.is_successful()) {
      //
      // Note that the control transfers have a special libusb_control_setup
      // structure placed in the beginning of the buffer (it contains some
      // control-specific setup; see also |CreateChromeUsbControlTransferInfo|
      // function implementation for more details). So, as chrome.usb API
      // doesn't operate with these setup structures, the received response data
      // must be placed under some offset (using the helper function
      // libusb_control_transfer_get_data).
      //
      unsigned char* const data_buffer =
          transfer->type != LIBUSB_TRANSFER_TYPE_CONTROL
              ? transfer->buffer
              : libusb_control_transfer_get_data(transfer);

      const int data_length =
          transfer->type != LIBUSB_TRANSFER_TYPE_CONTROL
              ? transfer->length
              : libusb_control_transfer_get_setup(transfer)->wLength;

      transfer->status = FillLibusbTransferResult(
          request_result.payload().result_info,
          (transfer->flags & LIBUSB_TRANSFER_SHORT_NOT_OK) != 0, data_length,
          data_buffer, &transfer->actual_length);
    } else if (request_result.status() == RequestResultStatus::kCanceled) {
      transfer->status = LIBUSB_TRANSFER_CANCELLED;
    } else {
      transfer->status = LIBUSB_TRANSFER_ERROR;
    }

    GOOGLE_SMART_CARD_CHECK(transfer->callback);
    transfer->callback(transfer);

    if (transfer->flags & LIBUSB_TRANSFER_FREE_TRANSFER) {
      // Note that the transfer instance cannot be used after this point.
      LibusbFreeTransfer(transfer);
    }
  };
}

int LibusbJsProxy::LibusbHandleEventsWithTimeout(libusb_context* context,
                                                 int timeout_seconds) {
  context = SubstituteDefaultContextIfNull(context);

  context->WaitAndProcessAsyncTransferReceivedResults(
      std::chrono::high_resolution_clock::now() +
          std::chrono::seconds(timeout_seconds),
      nullptr);
  return LIBUSB_SUCCESS;
}

}  // namespace google_smart_card
