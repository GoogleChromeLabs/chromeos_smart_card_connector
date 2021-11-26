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
constexpr char kJsRequestListDevices[] = "listDevices";
constexpr char kJsRequestGetConfigurations[] = "getConfigurations";
constexpr char kJsRequestOpenDeviceHandle[] = "openDeviceHandle";
constexpr char kJsRequestCloseDeviceHandle[] = "closeDeviceHandle";
constexpr char kJsRequestClaimInterface[] = "claimInterface";
constexpr char kJsRequestReleaseInterface[] = "releaseInterface";
constexpr char kJsRequestResetDevice[] = "resetDevice";
constexpr char kJsRequestControlTransfer[] = "controlTransfer";
constexpr char kJsRequestBulkTransfer[] = "bulkTransfer";
constexpr char kJsRequestInterruptTransfer[] = "interruptTransfer";

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
    GOOGLE_SMART_CARD_LOG_WARNING << "LibusbGetDeviceList request failed: "
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

uint8_t JsEndpointTypeToLibusbMask(LibusbJsEndpointType value) {
  switch (value) {
    case LibusbJsEndpointType::kBulk:
      return LIBUSB_TRANSFER_TYPE_BULK << kLibusbTransferTypeMaskShift;
    case LibusbJsEndpointType::kControl:
      return LIBUSB_TRANSFER_TYPE_CONTROL << kLibusbTransferTypeMaskShift;
    case LibusbJsEndpointType::kInterrupt:
      return LIBUSB_TRANSFER_TYPE_INTERRUPT << kLibusbTransferTypeMaskShift;
    case LibusbJsEndpointType::kIsochronous:
      return LIBUSB_TRANSFER_TYPE_ISOCHRONOUS << kLibusbTransferTypeMaskShift;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }
}

void FillLibusbEndpointDescriptor(const LibusbJsEndpointDescriptor& js_endpoint,
                                  libusb_endpoint_descriptor* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  std::memset(result, 0, sizeof(libusb_endpoint_descriptor));

  result->bLength = sizeof(libusb_endpoint_descriptor);

  result->bDescriptorType = LIBUSB_DT_ENDPOINT;

  result->bEndpointAddress = js_endpoint.endpoint_address;

  result->bmAttributes |= JsEndpointTypeToLibusbMask(js_endpoint.type);
  // TODO(#429): Investigate synchronization and usage fields.

  result->wMaxPacketSize = js_endpoint.max_packet_size;

  // TODO(#429): Investigate the polling_interval field.

  if (js_endpoint.extra_data)
    result->extra = CopyRawData(*js_endpoint.extra_data).release();

  result->extra_length =
      js_endpoint.extra_data ? js_endpoint.extra_data->size() : 0;
}

void FillLibusbInterfaceDescriptor(
    const LibusbJsInterfaceDescriptor& js_interface,
    libusb_interface_descriptor* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  std::memset(result, 0, sizeof(libusb_interface_descriptor));

  result->bLength = sizeof(libusb_interface_descriptor);

  result->bDescriptorType = LIBUSB_DT_INTERFACE;

  result->bInterfaceNumber = js_interface.interface_number;

  result->bNumEndpoints = js_interface.endpoints.size();

  result->bInterfaceClass = js_interface.interface_class;

  result->bInterfaceSubClass = js_interface.interface_subclass;

  result->bInterfaceProtocol = js_interface.interface_protocol;

  result->endpoint =
      new libusb_endpoint_descriptor[js_interface.endpoints.size()];
  for (size_t index = 0; index < js_interface.endpoints.size(); ++index) {
    FillLibusbEndpointDescriptor(
        js_interface.endpoints[index],
        const_cast<libusb_endpoint_descriptor*>(&result->endpoint[index]));
  }

  if (js_interface.extra_data)
    result->extra = CopyRawData(*js_interface.extra_data).release();

  result->extra_length =
      js_interface.extra_data ? js_interface.extra_data->size() : 0;
}

void FillLibusbInterface(const LibusbJsInterfaceDescriptor& js_interface,
                         libusb_interface* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  result->altsetting = new libusb_interface_descriptor[1];
  FillLibusbInterfaceDescriptor(
      js_interface,
      const_cast<libusb_interface_descriptor*>(result->altsetting));

  result->num_altsetting = 1;
}

void FillLibusbConfigDescriptor(
    const LibusbJsConfigurationDescriptor& js_config,
    libusb_config_descriptor* result) {
  GOOGLE_SMART_CARD_CHECK(result);

  std::memset(result, 0, sizeof(libusb_config_descriptor));

  result->bLength = sizeof(libusb_config_descriptor);

  result->bDescriptorType = LIBUSB_DT_CONFIG;

  result->wTotalLength = sizeof(libusb_config_descriptor);

  result->bNumInterfaces = js_config.interfaces.size();

  result->bConfigurationValue = js_config.configuration_value;

  // TODO(#429): Investigate remote_wakeup, self_powered, max_power flags.

  result->interface = new libusb_interface[js_config.interfaces.size()];
  for (size_t index = 0; index < js_config.interfaces.size(); ++index) {
    FillLibusbInterface(
        js_config.interfaces[index],
        const_cast<libusb_interface*>(&result->interface[index]));
  }

  if (js_config.extra_data)
    result->extra = CopyRawData(*js_config.extra_data).release();

  result->extra_length =
      js_config.extra_data ? js_config.extra_data->size() : 0;
}

}  // namespace

int LibusbJsProxy::LibusbGetActiveConfigDescriptor(
    libusb_device* dev,
    libusb_config_descriptor** config) {
  GOOGLE_SMART_CARD_CHECK(dev);
  GOOGLE_SMART_CARD_CHECK(config);

  GenericRequestResult request_result = js_call_adaptor_.SyncCall(
      kJsRequestGetConfigurations, dev->js_device().device_id);
  std::string error_message;
  std::vector<LibusbJsConfigurationDescriptor> js_configs;
  if (!RemoteCallAdaptor::ExtractResultPayload(std::move(request_result),
                                               &error_message, &js_configs)) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbGetActiveConfigDescriptor request failed: " << error_message;
    return LIBUSB_ERROR_OTHER;
  }

  *config = nullptr;
  for (const auto& js_config : js_configs) {
    if (js_config.active) {
      if (*config) {
        // Normally there should be only one active configuration, but the
        // chrome.usb API implementation misbehaves on some devices: see
        // <https://crbug.com/1218141>. As a workaround, proceed with the first
        // received configuration that's marked as active.
        GOOGLE_SMART_CARD_LOG_WARNING
            << "Unexpected state in LibusbGetActiveConfigDescriptor: JS API "
               "returned multiple active configurations";
        break;
      }

      *config = new libusb_config_descriptor;
      FillLibusbConfigDescriptor(js_config, *config);
    }
  }
  if (!*config) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << "LibusbGetActiveConfigDescriptor request failed: No active config "
           "descriptors were returned by JS API";
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

  GenericRequestResult request_result = js_call_adaptor_.SyncCall(
      kJsRequestOpenDeviceHandle, dev->js_device().device_id);
  std::string error_message;
  int64_t js_device_handle = 0;
  if (!RemoteCallAdaptor::ExtractResultPayload(
          std::move(request_result), &error_message, &js_device_handle)) {
    GOOGLE_SMART_CARD_LOG_WARNING << "LibusbOpen request failed: "
                                  << error_message;
    // Modify the devices (fake) bus number that we report so that PCSC will
    // retry to connect to the device once it updates the device list.
    uint32_t new_bus_number =
        static_cast<uint32_t>(LibusbGetBusNumber(dev)) + 1;
    if (new_bus_number <= kMaximumBusNumber) {
      bus_numbers_[dev->js_device().device_id] = new_bus_number;
    }
    return LIBUSB_ERROR_OTHER;
  }

  *handle = new libusb_device_handle(dev, js_device_handle);
  return LIBUSB_SUCCESS;
}

void LibusbJsProxy::LibusbClose(libusb_device_handle* handle) {
  GOOGLE_SMART_CARD_CHECK(handle);

  GenericRequestResult request_result = js_call_adaptor_.SyncCall(
      kJsRequestCloseDeviceHandle, handle->device()->js_device().device_id,
      handle->js_device_handle());
  std::string error_message;
  if (!request_result.is_successful()) {
    // It's essential to not crash in this case, because this may happen during
    // shutdown process.
    GOOGLE_SMART_CARD_LOG_ERROR << "Failed to close USB device";
  }

  delete handle;
}

int LibusbJsProxy::LibusbClaimInterface(libusb_device_handle* dev,
                                        int interface_number) {
  GOOGLE_SMART_CARD_CHECK(dev);

  GenericRequestResult request_result = js_call_adaptor_.SyncCall(
      kJsRequestClaimInterface, dev->device()->js_device().device_id,
      dev->js_device_handle(), interface_number);
  if (!request_result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING << "LibusbClaimInterface request failed: "
                                  << request_result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  return LIBUSB_SUCCESS;
}

int LibusbJsProxy::LibusbReleaseInterface(libusb_device_handle* dev,
                                          int interface_number) {
  GOOGLE_SMART_CARD_CHECK(dev);

  GenericRequestResult request_result = js_call_adaptor_.SyncCall(
      kJsRequestReleaseInterface, dev->device()->js_device().device_id,
      dev->js_device_handle(), interface_number);
  if (!request_result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING << "LibusbReleaseInterface request failed: "
                                  << request_result.error_message();
    return LIBUSB_ERROR_OTHER;
  }
  return LIBUSB_SUCCESS;
}

int LibusbJsProxy::LibusbResetDevice(libusb_device_handle* dev) {
  GOOGLE_SMART_CARD_CHECK(dev);

  GenericRequestResult request_result = js_call_adaptor_.SyncCall(
      kJsRequestResetDevice, dev->device()->js_device().device_id,
      dev->js_device_handle());
  if (!request_result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING << "LibusbResetDevice request failed: "
                                  << request_result.error_message();
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

bool CreateLibusbJsControlTransferParameters(
    libusb_transfer* transfer,
    LibusbJsControlTransferParameters* result) {
  GOOGLE_SMART_CARD_CHECK(transfer);
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

  switch (control_setup->bmRequestType & kLibusbRequestTypeMask) {
    case LIBUSB_REQUEST_TYPE_STANDARD:
      result->request_type = LibusbJsTransferRequestType::kStandard;
      break;
    case LIBUSB_REQUEST_TYPE_CLASS:
      result->request_type = LibusbJsTransferRequestType::kClass;
      break;
    case LIBUSB_REQUEST_TYPE_VENDOR:
      result->request_type = LibusbJsTransferRequestType::kVendor;
      break;
    case LIBUSB_REQUEST_TYPE_RESERVED:
      GOOGLE_SMART_CARD_LOG_WARNING
          << "Libusb reserved request type is unsupported";
      return false;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }

  switch (control_setup->bmRequestType & kLibusbRequestRecipientMask) {
    case LIBUSB_RECIPIENT_DEVICE:
      result->recipient = LibusbJsTransferRecipient::kDevice;
      break;
    case LIBUSB_RECIPIENT_INTERFACE:
      result->recipient = LibusbJsTransferRecipient::kInterface;
      break;
    case LIBUSB_RECIPIENT_ENDPOINT:
      result->recipient = LibusbJsTransferRecipient::kEndpoint;
      break;
    case LIBUSB_RECIPIENT_OTHER:
      result->recipient = LibusbJsTransferRecipient::kOther;
      break;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }

  result->request = control_setup->bRequest;
  result->value = libusb_le16_to_cpu(control_setup->wValue);
  result->index = libusb_le16_to_cpu(control_setup->wIndex);

  if ((control_setup->bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) ==
      LIBUSB_ENDPOINT_OUT) {
    result->data_to_send = std::vector<uint8_t>(
        libusb_control_transfer_get_data(transfer),
        libusb_control_transfer_get_data(transfer) + data_length);
  } else {
    result->length_to_receive = data_length;
  }

  return true;
}

void CreateLibusbJsGenericTransferParameters(
    libusb_transfer* transfer,
    LibusbJsGenericTransferParameters* result) {
  GOOGLE_SMART_CARD_CHECK(transfer);
  GOOGLE_SMART_CARD_CHECK(result);
  GOOGLE_SMART_CARD_CHECK(transfer->type == LIBUSB_TRANSFER_TYPE_BULK ||
                          transfer->type == LIBUSB_TRANSFER_TYPE_INTERRUPT);

  result->endpoint_address = transfer->endpoint;
  if ((transfer->endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
    GOOGLE_SMART_CARD_CHECK(transfer->buffer);
    result->data_to_send = std::vector<uint8_t>(
        transfer->buffer, transfer->buffer + transfer->length);
  } else {
    result->length_to_receive = transfer->length;
  }
}

std::function<void(GenericRequestResult)> MakeLibusbJsTransferCallback(
    std::weak_ptr<libusb_context> context,
    const UsbTransferDestination& transfer_destination,
    LibusbJsProxy::TransferAsyncRequestStatePtr async_request_state) {
  return [context, transfer_destination,
          async_request_state](GenericRequestResult js_result) {
    const std::shared_ptr<libusb_context> locked_context = context.lock();
    if (!locked_context) {
      // The context that was used for the original transfer submission has
      // been destroyed already.
      return;
    }
    LibusbJsTransferResult js_transfer_result;
    RequestResult<LibusbJsTransferResult> converted_result =
        RemoteCallAdaptor::ConvertResultPayload(
            std::move(js_result), &js_transfer_result, &js_transfer_result);
    if (transfer_destination.IsInputDirection()) {
      locked_context->OnInputTransferResultReceived(
          transfer_destination, std::move(converted_result));
    } else {
      locked_context->OnOutputTransferResultReceived(
          async_request_state, std::move(converted_result));
    }
  };
}

UsbTransferDestination CreateUsbTransferDestinationForTransfer(
    libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);
  switch (transfer->type) {
    case LIBUSB_TRANSFER_TYPE_CONTROL: {
      const libusb_control_setup* const control_setup =
          libusb_control_transfer_get_setup(transfer);
      return UsbTransferDestination::CreateForControlTransfer(
          transfer->dev_handle->js_device_handle(),
          control_setup->bmRequestType, control_setup->bRequest,
          control_setup->wValue, control_setup->wIndex);
    }
    case LIBUSB_TRANSFER_TYPE_BULK:
    case LIBUSB_TRANSFER_TYPE_INTERRUPT: {
      return UsbTransferDestination::CreateForGenericTransfer(
          transfer->dev_handle->js_device_handle(), transfer->endpoint);
    }
  }
  GOOGLE_SMART_CARD_NOTREACHED;
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

  LibusbJsGenericTransferParameters generic_transfer_params;
  LibusbJsControlTransferParameters control_transfer_params;
  switch (transfer->type) {
    case LIBUSB_TRANSFER_TYPE_CONTROL:
      if (!CreateLibusbJsControlTransferParameters(transfer,
                                                   &control_transfer_params))
        return LIBUSB_ERROR_INVALID_PARAM;
      break;
    case LIBUSB_TRANSFER_TYPE_BULK:
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
      CreateLibusbJsGenericTransferParameters(transfer,
                                              &generic_transfer_params);
      break;
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }

  const UsbTransferDestination transfer_destination =
      CreateUsbTransferDestinationForTransfer(transfer);

  context->AddAsyncTransferInFlight(async_request_state, transfer_destination,
                                    transfer);

  const auto js_api_callback = MakeLibusbJsTransferCallback(
      contexts_storage_.FindContextByAddress(context), transfer_destination,
      async_request_state);

  switch (transfer->type) {
    case LIBUSB_TRANSFER_TYPE_CONTROL:
      js_call_adaptor_.AsyncCall(
          js_api_callback, kJsRequestControlTransfer,
          transfer->dev_handle->device()->js_device().device_id,
          transfer->dev_handle->js_device_handle(), control_transfer_params);
      break;
    case LIBUSB_TRANSFER_TYPE_BULK:
      js_call_adaptor_.AsyncCall(
          js_api_callback, kJsRequestBulkTransfer,
          transfer->dev_handle->device()->js_device().device_id,
          transfer->dev_handle->js_device_handle(), generic_transfer_params);
      break;
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
      js_call_adaptor_.AsyncCall(
          js_api_callback, kJsRequestInterruptTransfer,
          transfer->dev_handle->device()->js_device().device_id,
          transfer->dev_handle->js_device_handle(), generic_transfer_params);
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
    const LibusbJsTransferResult& js_result,
    bool is_short_not_ok,
    int data_length,
    unsigned char* data_buffer,
    int* actual_length) {
  // FIXME(emaxx): Looks like chrome.usb API returns timeout results as if they
  // were errors. So, in case of timeout, LIBUSB_TRANSFER_ERROR will be
  // returned to the consumers instead of returning LIBUSB_TRANSFER_TIMED_OUT.
  // This doesn't look like a huge problem, but still, from the sanity
  // prospective, this probably requires fixing.

  int actual_length_value;
  if (js_result.received_data) {
    actual_length_value = std::min(
        static_cast<int>(js_result.received_data->size()), data_length);
    if (actual_length_value) {
      std::copy_n(js_result.received_data->begin(), actual_length_value,
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

// The callback to be passed in the libusb_transfer structures used for
// performing synchronous transfers. The callback assumes that the `user_data`
// field points to the int that's used by the event loop as a signal to stop.
void OnSyncTransferCompleted(libusb_transfer* transfer) {
  int* completed = static_cast<int*>(transfer->user_data);
  *completed = 1;
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

  // Implement the synchronous transfer in terms of asynchronous one.
  libusb_transfer transfer;
  std::memset(&transfer, 0, sizeof(transfer));

  // Libusb requires the control transfer's setup packet (of size
  // `LIBUSB_CONTROL_SETUP_SIZE`) to precede the data buffer.
  std::vector<uint8_t> buffer(LIBUSB_CONTROL_SETUP_SIZE + wLength);
  libusb_fill_control_setup(buffer.data(), bmRequestType, bRequest, wValue,
                            index, wLength);
  if ((bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
    // It's output transfer, so copy the passed data into the new buffer.
    std::copy_n(data, wLength, buffer.data() + LIBUSB_CONTROL_SETUP_SIZE);
  }

  int transfer_completed = 0;
  libusb_fill_control_transfer(&transfer, dev, buffer.data(),
                               /*callback=*/
                               &OnSyncTransferCompleted,
                               /*user_data=*/&transfer_completed, timeout);

  int transfer_result = LibusbSubmitTransfer(&transfer);
  if (transfer_result != LIBUSB_SUCCESS)
    return transfer_result;
  while (!transfer_completed) {
    // No need to check the return code (and cancel the transfer when it fails),
    // as our implementation of libusb_handle_events_* always succeeds.
    LibusbHandleEventsCompleted(dev->context(), &transfer_completed);
  }
  GOOGLE_SMART_CARD_CHECK(transfer_completed);

  if ((bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
    // It's input transfer, so copy the received data into the passed buffer.
    std::copy_n(buffer.data() + LIBUSB_CONTROL_SETUP_SIZE,
                transfer.actual_length, data);
  }
  transfer_result = LibusbTransferStatusToLibusbErrorCode(transfer.status);
  if (transfer_result != LIBUSB_SUCCESS)
    return transfer_result;
  return transfer.actual_length;
}

int LibusbJsProxy::LibusbBulkTransfer(libusb_device_handle* dev,
                                      unsigned char endpoint_address,
                                      unsigned char* data,
                                      int length,
                                      int* actual_length,
                                      unsigned timeout) {
  GOOGLE_SMART_CARD_CHECK(dev);
  return DoGenericSyncTranfer(LIBUSB_TRANSFER_TYPE_BULK, dev, endpoint_address,
                              data, length, actual_length, timeout);
}

int LibusbJsProxy::LibusbInterruptTransfer(libusb_device_handle* dev,
                                           unsigned char endpoint_address,
                                           unsigned char* data,
                                           int length,
                                           int* actual_length,
                                           unsigned timeout) {
  GOOGLE_SMART_CARD_CHECK(dev);
  return DoGenericSyncTranfer(LIBUSB_TRANSFER_TYPE_INTERRUPT, dev,
                              endpoint_address, data, length, actual_length,
                              timeout);
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

libusb_context* LibusbJsProxy::SubstituteDefaultContextIfNull(
    libusb_context* context_or_nullptr) const {
  if (context_or_nullptr)
    return context_or_nullptr;
  return default_context_.get();
}

LibusbJsProxy::TransferAsyncRequestCallback
LibusbJsProxy::WrapLibusbTransferCallback(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  return [this, transfer](TransferRequestResult request_result) {
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
          request_result.payload(),
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

int LibusbJsProxy::DoGenericSyncTranfer(libusb_transfer_type transfer_type,
                                        libusb_device_handle* device_handle,
                                        unsigned char endpoint_address,
                                        unsigned char* data,
                                        int length,
                                        int* actual_length,
                                        unsigned timeout) {
  GOOGLE_SMART_CARD_CHECK(transfer_type == LIBUSB_TRANSFER_TYPE_BULK ||
                          transfer_type == LIBUSB_TRANSFER_TYPE_INTERRUPT);

  // Implement the synchronous transfer in terms of asynchronous one.
  libusb_transfer transfer;
  std::memset(&transfer, 0, sizeof(transfer));

  int transfer_completed = 0;
  if (transfer_type == LIBUSB_TRANSFER_TYPE_BULK) {
    libusb_fill_bulk_transfer(&transfer, device_handle, endpoint_address, data,
                              length,
                              /*callback=*/&OnSyncTransferCompleted,
                              /*user_data=*/&transfer_completed, timeout);
  } else if (transfer_type == LIBUSB_TRANSFER_TYPE_INTERRUPT) {
    libusb_fill_interrupt_transfer(&transfer, device_handle, endpoint_address,
                                   data, length,
                                   /*callback=*/&OnSyncTransferCompleted,
                                   /*user_data=*/&transfer_completed, timeout);
  } else {
    GOOGLE_SMART_CARD_NOTREACHED;
  }

  int transfer_result = LibusbSubmitTransfer(&transfer);
  if (transfer_result != LIBUSB_SUCCESS)
    return transfer_result;
  while (!transfer_completed) {
    // No need to check the return code (and cancel the transfer when it fails),
    // as our implementation of libusb_handle_events_* always succeeds.
    LibusbHandleEventsCompleted(device_handle->context(), &transfer_completed);
  }
  GOOGLE_SMART_CARD_CHECK(transfer_completed);

  if (actual_length)
    *actual_length = transfer.actual_length;
  return LibusbTransferStatusToLibusbErrorCode(transfer.status);
}

}  // namespace google_smart_card
