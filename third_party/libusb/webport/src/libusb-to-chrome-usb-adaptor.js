/**
 * @license
 * Copyright 2021 Google Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

goog.provide('GoogleSmartCard.LibusbToChromeUsbAdaptor');

goog.require('GoogleSmartCard.LibusbProxyDataModel');
goog.require('GoogleSmartCard.LibusbToJsApiAdaptor');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.asserts');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;
const LibusbJsConfigurationDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsConfigurationDescriptor;
const LibusbJsControlTransferParameters =
    GSC.LibusbProxyDataModel.LibusbJsControlTransferParameters;
const LibusbJsDevice = GSC.LibusbProxyDataModel.LibusbJsDevice;
const LibusbJsDirection = GSC.LibusbProxyDataModel.LibusbJsDirection;
const LibusbJsEndpointDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsEndpointDescriptor;
const LibusbJsEndpointType = GSC.LibusbProxyDataModel.LibusbJsEndpointType;
const LibusbJsGenericTransferParameters =
    GSC.LibusbProxyDataModel.LibusbJsGenericTransferParameters;
const LibusbJsInterfaceDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsInterfaceDescriptor;
const LibusbJsTransferRecipient =
    GSC.LibusbProxyDataModel.LibusbJsTransferRecipient;
const LibusbJsTransferRequestType =
    GSC.LibusbProxyDataModel.LibusbJsTransferRequestType;
const LibusbJsTransferResult = GSC.LibusbProxyDataModel.LibusbJsTransferResult;

const logger = GSC.Logging.getScopedLogger('LibusbToChromeUsbAdaptor');

/**
 * Implements the Libusb requests via the chrome.usb Apps API.
 */
GSC.LibusbToChromeUsbAdaptor = class extends GSC.LibusbToJsApiAdaptor {
  constructor() {
    super();
    /**
     * Mapping from IDs (generated by us) into device objects (returned by the
     * chrome.usb API).
     * @type {!Map<number, !chrome.usb.Device>}
     */
    this.idToChromeUsbDeviceMap_ = new Map();
  }

  /** @override */
  async listDevices() {
    const chromeUsbDevices = /** @type {!Array<!chrome.usb.Device>} */ (
        await promisify(chrome.usb.getDevices, /*options=*/ {}));

    // Keep references to all current chrome.usb.Device objects, since other API
    // functions take chrome.usb.Device as parameter, meanwhile the requests
    // from the C++ side only specify the deviceId. While we could try
    // recreating the chrome.usb.Device objects on the flight, based on the
    // LibusbJsDevice instances, it's error-prone and also breaks when a new
    // field gets added into chrome.usb.Device.
    this.updateDeviceMap_(chromeUsbDevices);

    return chromeUsbDevices.map(convertChromeUsbDeviceToLibusb);
  }

  /** @override */
  async getConfigurations(deviceId) {
    const chromeUsbDevice = this.getDeviceByIdOrThrow_(deviceId);
    const chromeUsbConfigDescriptors =
        /** @type {!Array<!chrome.usb.ConfigDescriptor>} */ (
            await promisify(chrome.usb.getConfigurations, chromeUsbDevice));
    return chromeUsbConfigDescriptors.map(
        convertChromeUsbDeviceDescriptorToLibusb);
  }

  /** @override */
  async openDeviceHandle(deviceId) {
    const chromeUsbDevice = this.getDeviceByIdOrThrow_(deviceId);
    const chromeUsbConnectionHandle =
        /** @type {!chrome.usb.ConnectionHandle} */ (
            await promisify(chrome.usb.openDevice, chromeUsbDevice));
    return chromeUsbConnectionHandle.handle;
  }

  /** @override */
  async closeDeviceHandle(deviceId, deviceHandle) {
    const chromeUsbConnectionHandle =
        this.getConnectionHandleWithDeviceIdOrThrow_(deviceId, deviceHandle);
    await promisify(chrome.usb.closeDevice, chromeUsbConnectionHandle);
  }

  /** @override */
  async claimInterface(deviceId, deviceHandle, interfaceNumber) {
    const chromeUsbConnectionHandle =
        this.getConnectionHandleWithDeviceIdOrThrow_(deviceId, deviceHandle);
    await promisify(
        chrome.usb.claimInterface, chromeUsbConnectionHandle, interfaceNumber);
  }

  /** @override */
  async releaseInterface(deviceId, deviceHandle, interfaceNumber) {
    const chromeUsbConnectionHandle =
        this.getConnectionHandleWithDeviceIdOrThrow_(deviceId, deviceHandle);
    await promisify(
        chrome.usb.releaseInterface, chromeUsbConnectionHandle,
        interfaceNumber);
  }

  /** @override */
  async resetDevice(deviceId, deviceHandle) {
    const chromeUsbConnectionHandle =
        this.getConnectionHandleWithDeviceIdOrThrow_(deviceId, deviceHandle);
    await promisify(chrome.usb.resetDevice, chromeUsbConnectionHandle);
  }

  /** @override */
  async controlTransfer(deviceId, deviceHandle, parameters) {
    const chromeUsbConnectionHandle =
        this.getConnectionHandleWithDeviceIdOrThrow_(deviceId, deviceHandle);
    const chromeUsbControlTransferInfo =
        getChromeUsbControlTransferInfo(parameters);
    const chromeUsbTransferResultInfo =
        /** @type {!chrome.usb.TransferResultInfo} */ (await promisify(
            chrome.usb.controlTransfer, chromeUsbConnectionHandle,
            chromeUsbControlTransferInfo));
    return getLibusbJsTransferResultOrThrow(chromeUsbTransferResultInfo);
  }

  /** @override */
  async bulkTransfer(deviceId, deviceHandle, parameters) {
    return await this.genericTransfer_(
        chrome.usb.bulkTransfer, deviceId, deviceHandle, parameters);
  }

  /** @override */
  async interruptTransfer(deviceId, deviceHandle, parameters) {
    return await this.genericTransfer_(
        chrome.usb.interruptTransfer, deviceId, deviceHandle, parameters);
  }

  /**
   * @private
   * @param {!Array<!chrome.usb.Device>} chromeUsbDevices
   */
  updateDeviceMap_(chromeUsbDevices) {
    this.idToChromeUsbDeviceMap_ = new Map();
    for (const chromeUsbDevice of chromeUsbDevices)
      this.idToChromeUsbDeviceMap_.set(chromeUsbDevice.device, chromeUsbDevice);
  }

  /**
   * @private
   * @param {number} deviceId
   * @return {!chrome.usb.Device}
   */
  getDeviceByIdOrThrow_(deviceId) {
    const chromeUsbDevice = this.idToChromeUsbDeviceMap_.get(deviceId);
    if (!chromeUsbDevice)
      throw new Error(`No device with ID ${deviceId}`);
    return chromeUsbDevice;
  }

  /**
   * @private
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @return {!chrome.usb.ConnectionHandle}
   */
  getConnectionHandleWithDeviceIdOrThrow_(deviceId, deviceHandle) {
    const chromeUsbDevice = this.getDeviceByIdOrThrow_(deviceId);
    return getChromeUsbConnectionHandle(chromeUsbDevice, deviceHandle);
  }

  /**
   * Performs a bulk or an interrupt transfer (depending on the passed API
   * method).
   * @private
   * @param {!Function} chromeUsbApiMethod
   * @param {number} deviceId
   * @param {number} deviceHandle
   * @param {!LibusbJsGenericTransferParameters} parameters
   * @return {!Promise<!LibusbJsTransferResult>}
   */
  async genericTransfer_(
      chromeUsbApiMethod, deviceId, deviceHandle, parameters) {
    const chromeUsbConnectionHandle =
        this.getConnectionHandleWithDeviceIdOrThrow_(deviceId, deviceHandle);
    const chromeUsbGenericTransferInfo =
        getChromeUsbGenericTransferInfo(parameters);
    const chromeUsbTransferResultInfo =
        /** @type {!chrome.usb.TransferResultInfo} */ (await promisify(
            chromeUsbApiMethod, chromeUsbConnectionHandle,
            chromeUsbGenericTransferInfo));
    return getLibusbJsTransferResultOrThrow(chromeUsbTransferResultInfo);
  }
};

/**
 * Returns whether the API needed for this adaptor to work is available.
 * @static
 * @return {boolean}
 */
GSC.LibusbToChromeUsbAdaptor.isApiAvailable = function() {
  return chrome !== undefined && chrome.usb !== undefined;
};

/**
 * Runs the specified chrome.usb API method and returns its result via a
 * promise.
 * @param {!Function} apiMethod The chrome.usb API method to call.
 * @param {...*} apiArguments The parameters to pass to the called method.
 * @return {!Promise<*>}
 */
function promisify(apiMethod, ...apiArguments) {
  return new Promise((resolve, reject) => {
    apiMethod.call(chrome.usb, ...apiArguments, function(apiResult) {
      if (chrome.runtime.lastError) {
        const apiError = goog.object.get(
            chrome.runtime.lastError, 'message', 'Unknown error');
        reject(apiError);
        return;
      }
      resolve(apiResult);
    });
  });
}

/**
 * @param {!chrome.usb.Device} chromeUsbDevice
 * @return {!LibusbJsDevice}
 */
function convertChromeUsbDeviceToLibusb(chromeUsbDevice) {
  /** @type {!LibusbJsDevice} */
  const libusbJsDevice = {
    'deviceId': chromeUsbDevice.device,
    'vendorId': chromeUsbDevice.vendorId,
    'productId': chromeUsbDevice.productId,
    'productName': chromeUsbDevice.productName,
    'manufacturerName': chromeUsbDevice.manufacturerName,
    'serialNumber': chromeUsbDevice.serialNumber,
  };
  if (chromeUsbDevice.version !== undefined &&
      chromeUsbDevice.version !== null) {
    // The "version" field was only added in Chrome 51.
    libusbJsDevice['version'] = chromeUsbDevice.version;
  }
  return libusbJsDevice;
}

/**
 * @param {!chrome.usb.ConfigDescriptor} chromeUsbConfigDescriptor
 * @return {!LibusbJsConfigurationDescriptor}
 */
function convertChromeUsbDeviceDescriptorToLibusb(chromeUsbConfigDescriptor) {
  return {
    'active': chromeUsbConfigDescriptor.active,
    'configurationValue': chromeUsbConfigDescriptor.configurationValue,
    'extraData': chromeUsbConfigDescriptor.extra_data,
    'interfaces': chromeUsbConfigDescriptor.interfaces.map(
        convertChromeUsbInterfaceDescriptorToLibusb),
  };
}

/**
 * @param {!chrome.usb.InterfaceDescriptor} chromeUsbInterfaceDescriptor
 * @return {!LibusbJsInterfaceDescriptor}
 */
function convertChromeUsbInterfaceDescriptorToLibusb(
    chromeUsbInterfaceDescriptor) {
  return {
    'interfaceNumber': chromeUsbInterfaceDescriptor.interfaceNumber,
    'interfaceClass': chromeUsbInterfaceDescriptor.interfaceClass,
    'interfaceSubclass': chromeUsbInterfaceDescriptor.interfaceSubclass,
    'interfaceProtocol': chromeUsbInterfaceDescriptor.interfaceProtocol,
    'extraData': chromeUsbInterfaceDescriptor.extra_data,
    'endpoints': chromeUsbInterfaceDescriptor.endpoints.map(
        convertChromeUsbEndpointToLibusb),
  };
}

/**
 * @param {!chrome.usb.EndpointDescriptor} chromeUsbEndpointDescriptor
 * @return {!LibusbJsEndpointDescriptor}
 */
function convertChromeUsbEndpointToLibusb(chromeUsbEndpointDescriptor) {
  return {
    'endpointAddress': chromeUsbEndpointDescriptor.address,
    'direction': convertChromeUsbDirectionToLibusb(
        chromeUsbEndpointDescriptor.direction),
    'type':
        convertChromeUsbEndpointTypeToLibusb(chromeUsbEndpointDescriptor.type),
    'extraData': chromeUsbEndpointDescriptor.extra_data,
    'maxPacketSize': chromeUsbEndpointDescriptor.maximumPacketSize,
  };
}

/**
 * @param {string} chromeUsbDirection The chrome.usb.Direction enum.
 * @return {!LibusbJsDirection}
 */
function convertChromeUsbDirectionToLibusb(chromeUsbDirection) {
  switch (chromeUsbDirection) {
    case 'in':
      return LibusbJsDirection.IN;
    case 'out':
      return LibusbJsDirection.OUT;
  }
  GSC.Logging.failWithLogger(
      logger, `Unexpected chrome.usb direction: ${chromeUsbDirection}`);
  goog.asserts.fail();
}

/**
 * @param {string} chromeUsbEndpointType The chrome.usb.TransferType enum.
 * @return {!LibusbJsEndpointType}
 */
function convertChromeUsbEndpointTypeToLibusb(chromeUsbEndpointType) {
  switch (chromeUsbEndpointType) {
    case 'bulk':
      return LibusbJsEndpointType.BULK;
    case 'control':
      return LibusbJsEndpointType.CONTROL;
    case 'interrupt':
      return LibusbJsEndpointType.INTERRUPT;
    case 'isochronous':
      return LibusbJsEndpointType.ISOCHRONOUS;
  }
  GSC.Logging.failWithLogger(
      logger, `Unexpected chrome.usb endpoint type: ${chromeUsbEndpointType}`);
  goog.asserts.fail();
}

/**
 * @param {!chrome.usb.Device} chromeUsbDevice
 * @param {number} deviceHandle
 * @return {!chrome.usb.ConnectionHandle}
 */
function getChromeUsbConnectionHandle(chromeUsbDevice, deviceHandle) {
  return /** @type {!chrome.usb.ConnectionHandle} */ ({
    'handle': deviceHandle,
    'productId': chromeUsbDevice.productId,
    'vendorId': chromeUsbDevice.vendorId,
  });
}

/**
 * @param {!LibusbJsControlTransferParameters} libusbJsParameters
 * @return {!chrome.usb.ControlTransferInfo}
 */
function getChromeUsbControlTransferInfo(libusbJsParameters) {
  const controlTransferInfo = /** @type {!chrome.usb.ControlTransferInfo} */ ({
    'direction': libusbJsParameters['dataToSend'] ? 'out' : 'in',
    'index': libusbJsParameters['index'],
    'recipient': getChromeUsbRecipient(libusbJsParameters['recipient']),
    'request': libusbJsParameters['request'],
    'requestType': getChromeUsbRequestType(libusbJsParameters['requestType']),
    'value': libusbJsParameters['value'],
  });
  if (libusbJsParameters['dataToSend'])
    controlTransferInfo['data'] = libusbJsParameters['dataToSend'];
  if (libusbJsParameters['lengthToReceive'] !== undefined)
    controlTransferInfo['length'] = libusbJsParameters['lengthToReceive'];
  return controlTransferInfo;
}

/**
 * @param {!LibusbJsTransferRecipient} libusbJsTransferRecipient
 * @return {string} The chrome.usb.Recipient value.
 */
function getChromeUsbRecipient(libusbJsTransferRecipient) {
  switch (libusbJsTransferRecipient) {
    case LibusbJsTransferRecipient.DEVICE:
      return 'device';
    case LibusbJsTransferRecipient.INTERFACE:
      return 'interface';
    case LibusbJsTransferRecipient.ENDPOINT:
      return 'endpoint';
    case LibusbJsTransferRecipient.OTHER:
      return 'other';
  }
  GSC.Logging.failWithLogger(
      logger, `Unexpected libusb recipient: ${libusbJsTransferRecipient}`);
  goog.asserts.fail();
}

/**
 * @param {!LibusbJsTransferRequestType} libusbJsTransferRequestType
 * @return {string} The chrome.usb.RequestType value.
 */
function getChromeUsbRequestType(libusbJsTransferRequestType) {
  switch (libusbJsTransferRequestType) {
    case LibusbJsTransferRequestType.STANDARD:
      return 'standard';
    case LibusbJsTransferRequestType.CLASS:
      return 'class';
    case LibusbJsTransferRequestType.VENDOR:
      return 'vendor';
  }
  GSC.Logging.failWithLogger(
      logger, `Unexpected libusb request type: ${libusbJsTransferRequestType}`);
  goog.asserts.fail();
}

/**
 * @param {!LibusbJsGenericTransferParameters} libusbJsParameters
 * @return {!chrome.usb.GenericTransferInfo}
 */
function getChromeUsbGenericTransferInfo(libusbJsParameters) {
  const genericTransferInfo = /** @type {!chrome.usb.GenericTransferInfo} */ ({
    'direction': libusbJsParameters['dataToSend'] ? 'out' : 'in',
    'endpoint': libusbJsParameters['endpointAddress'],
  });
  if (libusbJsParameters['dataToSend'])
    genericTransferInfo['data'] = libusbJsParameters['dataToSend'];
  if (libusbJsParameters['lengthToReceive'] !== undefined)
    genericTransferInfo['length'] = libusbJsParameters['lengthToReceive'];
  return genericTransferInfo;
}

/**
 * @param {!chrome.usb.TransferResultInfo} chromeUsbTransferResultInfo
 * @return {!LibusbJsTransferResult}
 */
function getLibusbJsTransferResultOrThrow(chromeUsbTransferResultInfo) {
  if (chromeUsbTransferResultInfo.resultCode) {
    throw new Error(`USB API failed with resultCode=${
        chromeUsbTransferResultInfo.resultCode}`);
  }
  /** @type {!LibusbJsTransferResult} */
  const libusbJsTransferResult = {};
  // Note that both checks - that `data` is present and that it's non-empty -
  // are necessary, since contrary to the docs even output transfers have the
  // field provided (as an empty array buffer).
  if (chromeUsbTransferResultInfo.data &&
      chromeUsbTransferResultInfo.data.byteLength) {
    libusbJsTransferResult['receivedData'] = chromeUsbTransferResultInfo.data;
  }
  return libusbJsTransferResult;
}
});  // goog.scope
