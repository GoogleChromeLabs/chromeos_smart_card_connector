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
 * @fileoverview This script implements displaying of the devices list and
 * requesting USB permissions in the Smart Card Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.DevicesDisplaying');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.ReaderTracker');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.events.EventType');
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
var logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

/**
 * @type {Element}
 * @const
 */
var readersListElement = goog.dom.getElement('readers-list');

/**
 * @type {Element}
 * @const
 */
var addDeviceElement = goog.dom.getElement('add-device');

/**
 * @type {GSC.ReaderTracker}
 */
var readerTracker;

function loadDeviceList() {
  logger.fine('Requesting available USB devices list...');
  chrome.usb.getDevices({'filters': USB_DEVICE_FILTERS}, getDevicesCallback);
}

function displayReaderList(readers) {
  logger.info(readers.length + ' card reader(s) available: ' +
              GSC.DebugDump.dump(readers));
  goog.dom.removeChildren(readersListElement);

  for (let reader of readers) {
    GSC.Logging.checkWithLogger(logger, !goog.isNull(readersListElement));
    goog.asserts.assert(readersListElement);
    var errorIndicator = goog.dom.createDom(
        'div', 'error-indicator color-' + reader.status);
    var title = reader.error ? {title: 'Reader error, code = ' + reader.error} :
                               undefined;
    var element = goog.dom.createDom('li', title, errorIndicator, reader.name);
    goog.dom.append(readersListElement, element);
  };
}

function readerUpdatedListener(readers) {
  displayReaderList(readers);
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
 * @param {!Event} e
 */
function addDeviceClickListener(e) {
  e.preventDefault();

  logger.fine('Running USB devices selection dialog...');
  chrome.usb.getUserSelectedDevices(
      {'multiple': true, 'filters': USB_DEVICE_FILTERS},
      getUserSelectedDevicesCallback);
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

/**
 * @param {!Array.<!chrome.usb.Device>} devices
 */
function getUserSelectedDevicesCallback(devices) {
  logger.fine('USB selection dialog finished, ' + devices.length + ' devices ' +
              'were chosen');
  loadDeviceList();
}

GSC.ConnectorApp.Window.DevicesDisplaying.initialize = function() {
  // TODO(isandrk): Move logging into background script because this way it's
  //     only working while the app window is opened.
  loadDeviceList();
  chrome.usb.onDeviceAdded.addListener(usbDeviceAddedListener);
  chrome.usb.onDeviceRemoved.addListener(usbDeviceRemovedListener);

  readerTracker = GSC.PopupWindow.Client.getData()['readerTracker'];
  displayReaderList(readerTracker.getReaders());
  readerTracker.addOnUpdateListener(readerUpdatedListener);
  chrome.app.window.current().onClosed.addListener(function() {
    readerTracker.removeOnUpdateListener(readerUpdatedListener);
  });

  goog.events.listen(
      addDeviceElement, goog.events.EventType.CLICK, addDeviceClickListener);
};

});  // goog.scope
