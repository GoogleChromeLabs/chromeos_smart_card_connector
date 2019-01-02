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
goog.require('GoogleSmartCard.ObjectHelpers');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTracker');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.events.EventType');
goog.require('goog.log.Logger');
goog.scope(function() {

/**
 * USB device filter that is used when displaying the USB selection dialog to
 * the used.
 * @const
 */
var USB_DEVICE_FILTERS = [{'interfaceClass': 0x0B}];

/**
 * PC/SC usually appends some numbers to the reader names. This constant
 * specifies which suffixes should be removed before displaying, as it doesn't
 * make much sense to expose them to the user.
 *
 * Only the first matched suffix from this list would be removed from the
 * resulting reader name.
 * @const
 */
var READER_NAME_SUFFIXES_TO_REMOVE = [" 00 00", " 00"];

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

/**
 * @type {!Element}
 * @const
 */
var readersListElement = /** @type {!Element} */ (goog.dom.getElement(
    'readers-list'));

/**
 * @type {!Element}
 * @const
 */
var addDeviceElement = /** @type {!Element} */ (goog.dom.getElement(
    'add-device'));

/**
 * @param {!Array.<!GSC.PcscLiteServer.ReaderInfo>} readers
 */
function displayReaderList(readers) {
  logger.info('Displaying ' + readers.length + ' card reader(s): ' +
              GSC.DebugDump.dump(readers));
  goog.dom.removeChildren(readersListElement);

  for (let reader of readers) {
    GSC.Logging.checkWithLogger(logger, !goog.isNull(readersListElement));
    goog.asserts.assert(readersListElement);

    var indicatorClasses = 'reader-state-indicator reader-state-indicator-' +
                           reader['status'];
    if (reader['status'] == GSC.PcscLiteServer.ReaderStatus.SUCCESS &&
        reader['isCardPresent']) {
      indicatorClasses = 'reader-card-present-indicator';
    }
    var indicator = goog.dom.createDom('span', indicatorClasses);

    var indicatorContainer = goog.dom.createDom(
        'span', 'reader-indicator-container', indicator);

    var text = makeReaderNameForDisplaying(reader['name']) +
        (reader['error'] ? ' (Error ' + reader['error'] + ')' : '');

    var element = goog.dom.createDom('li', undefined, indicatorContainer, text);
    goog.dom.append(readersListElement, element);
  }
}

function makeReaderNameForDisplaying(readerName) {
  for (let suffixToRemove of READER_NAME_SUFFIXES_TO_REMOVE) {
    if (goog.string.endsWith(readerName, suffixToRemove)) {
      var newReaderName = readerName.substr(
          0, readerName.length - suffixToRemove.length);
      logger.fine('Transformed reader name "' + readerName + '" into "' +
                  newReaderName + '"');
      return newReaderName;
    }
  }
  return readerName;
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
  var readerTrackerSubscriber =
      /** @type {function(function(!Array.<!GSC.PcscLiteServer.ReaderInfo>))} */
      (GSC.ObjectHelpers.extractKey(
           GSC.PopupWindow.Client.getData(), 'readerTrackerSubscriber'));
  readerTrackerSubscriber(displayReaderList);

  var readerTrackerUnsubscriber =
      /** @type {function(function(!Array.<!GSC.PcscLiteServer.ReaderInfo>))} */
      (GSC.ObjectHelpers.extractKey(
           GSC.PopupWindow.Client.getData(), 'readerTrackerUnsubscriber'));
  chrome.app.window.current().onClosed.addListener(function() {
    readerTrackerUnsubscriber(displayReaderList);
  });

  goog.events.listen(
      addDeviceElement, goog.events.EventType.CLICK, addDeviceClickListener);
};

});  // goog.scope
