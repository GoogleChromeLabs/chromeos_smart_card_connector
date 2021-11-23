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

goog.require('GoogleSmartCard.LibusbToJsApiAdaptor');
goog.require('GoogleSmartCard.LibusbProxyDataModel');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;
const LibusbJsDevice = GSC.LibusbProxyDataModel.LibusbJsDevice;

/**
 * Implements the Libusb requests via the chrome.usb Apps API.
 */
GSC.LibusbToChromeUsbAdaptor = class extends GSC.LibusbToJsApiAdaptor {
  /** @override */
  async listDevices() {
    const devices = /** @type {!Array<!chrome.usb.Device>} */ (
        await promisify(chrome.usb.getDevices, /*options=*/ {}));
    return devices.map(convertChromeUsbDeviceToLibusb);
  }
};

/**
 * Runs the specified chrome.usb API method and returns its result via a
 * promise.
 * @param {!Function} apiMethod The chrome.usb API method to call.
 * @param {...*} apiArguments The parameters to pass to the called method.
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
  if (typeof chromeUsbDevice.version !== undefined &&
      typeof chromeUsbDevice.version !== null) {
    // The "version" field was only added in Chrome 51.
    libusbJsDevice['version'] = chromeUsbDevice.version;
  }
  return libusbJsDevice;
}
});  // goog.scope
