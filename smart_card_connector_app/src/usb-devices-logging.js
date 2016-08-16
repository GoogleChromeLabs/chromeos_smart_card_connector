/** @license
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview This script does some minor device logging on the USB level
 * (therefore it sees some stuff that can miss the higher PC/SC level).
 */

goog.provide('GoogleSmartCard.ConnectorApp.setupUsbDevicesLogging');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.array');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var USB_DEVICE_FILTERS = [{'interfaceClass': 0x0B}];

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('ConnectorApp.UsbDevicesLogging');

function loadDeviceList() {
  logger.fine('Requesting available USB devices list...');
  chrome.usb.getDevices({'filters': USB_DEVICE_FILTERS}, getDevicesCallback);
}

/**
 * @param {!chrome.usb.Device} device
 */
function usbDeviceAddedListener(device) {
  logger.fine('A USB device was added: ' + makeReaderDescriptionString(device));
  loadDeviceList();
}

/**
 * @param {!chrome.usb.Device} device
 */
function usbDeviceRemovedListener(device) {
  logger.fine(
      'A USB device was removed: ' + makeReaderDescriptionString(device));
  loadDeviceList();
}

/**
 * @param {!Array.<!chrome.usb.Device>} devices
 */
function getDevicesCallback(devices) {
  goog.array.sortByKey(devices, function(device) { return device.device; });

  logger.info(devices.length + ' USB device(s) available: ' +
              GSC.DebugDump.dump(devices));
}

/**
 * @param {!chrome.usb.Device} reader
 * @return {string}
 */
function makeReaderDescriptionString(reader) {
  var parts = [];

  if (reader.productName) {
    parts.push(chrome.i18n.getMessage(
        'readerDescriptionProductNamePart', reader.productName));
  } else {
    parts.push(chrome.i18n.getMessage(
        'readerDescriptionProductNamePartUnknown'));
  }

  if (reader.manufacturerName) {
    parts.push(chrome.i18n.getMessage(
        'readerDescriptionManufacturerNamePart', reader.manufacturerName));
  }

  return parts.join(' ');
}

GSC.ConnectorApp.setupUsbDevicesLogging = function() {
  loadDeviceList();
  chrome.usb.onDeviceAdded.addListener(usbDeviceAddedListener);
  chrome.usb.onDeviceRemoved.addListener(usbDeviceRemovedListener);
};

// goog.exportSymbol('GoogleSmartCard.ConnectorApp.setupUsbDevicesLogging',
//                   setupUsbDevicesLogging);

});  // goog.scope
