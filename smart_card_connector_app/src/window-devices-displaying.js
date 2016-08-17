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
goog.require('GoogleSmartCard.MessagingCommon');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTracker');
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
 * @type {GSC.PcscLiteServer.ReaderTracker}
 */
var readerTracker;

/**
 * @param {!Array.<!GSC.PcscLiteServer.ReaderInfo>} readers
 */
function displayReaderList(readers) {
  logger.info(readers.length + ' card reader(s) available: ' +
              GSC.DebugDump.dump(readers));
  goog.dom.removeChildren(readersListElement);

  for (let reader of readers) {
    GSC.Logging.checkWithLogger(logger, !goog.isNull(readersListElement));
    goog.asserts.assert(readersListElement);

    var errorIndicator = goog.dom.createDom(
        'div', 'error-indicator color-' + reader['status']);
    var text = reader['name'] +
        (reader['error'] ? ' (Error id = ' + reader['error'] + ')' : '');

    var element = goog.dom.createDom('li', undefined, errorIndicator, text);
    goog.dom.append(readersListElement, element);
  };
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
function getUserSelectedDevicesCallback(devices) {
  logger.fine('USB selection dialog finished, ' + devices.length + ' devices ' +
              'were chosen');
}

GSC.ConnectorApp.Window.DevicesDisplaying.initialize = function() {
  readerTracker = GSC.MessagingCommon.extractKey(
      GSC.PopupWindow.Client.getData(), 'readerTracker');
  displayReaderList(readerTracker.getReaders());
  readerTracker.addOnUpdateListener(displayReaderList);
  chrome.app.window.current().onClosed.addListener(function() {
    readerTracker.removeOnUpdateListener(displayReaderList);
  });

  goog.events.listen(
      addDeviceElement, goog.events.EventType.CLICK, addDeviceClickListener);
};

});  // goog.scope
