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
goog.require('GoogleSmartCard.I18n');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.ObjectHelpers');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTracker');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.dom.dataset');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.log.Logger');
goog.require('goog.string');

goog.scope(function() {

/**
 * USB device filter that is used when displaying the USB selection dialog to
 * the used.
 */
const USB_DEVICE_FILTERS = [{'interfaceClass': 0x0B}];

/**
 * PC/SC usually appends some numbers to the reader names. This constant
 * specifies which suffixes should be removed before displaying, as it doesn't
 * make much sense to expose them to the user.
 *
 * Only the first matched suffix from this list would be removed from the
 * resulting reader name.
 */
const READER_NAME_SUFFIXES_TO_REMOVE = [' 00 00', ' 00'];

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

/** @type {!Element} */
const readersListElement =
    /** @type {!Element} */ (goog.dom.getElement('readers-list'));

/** @type {!Element} */
const addDeviceElement =
    /** @type {!Element} */ (goog.dom.getElement('add-device'));

/**
 * @param {!Array.<!GSC.PcscLiteServer.ReaderInfo>} readers 
 */
function onReadersChanged(readers) {
  displayReaderList(readers);
  updateAddDeviceButtonText(readers.length);
}

/**
 * @param {!Array.<!GSC.PcscLiteServer.ReaderInfo>} readers
 */
function displayReaderList(readers) {
  logger.info('Displaying ' + readers.length + ' card reader(s): ' +
              GSC.DebugDump.dump(readers));
  goog.dom.removeChildren(readersListElement);

  for (let reader of readers) {
    GSC.Logging.checkWithLogger(logger, readersListElement !== null);
    goog.asserts.assert(readersListElement);

    let indicatorClasses =
        'reader-state-indicator reader-state-indicator-' + reader['status'];
    if (reader['status'] == GSC.PcscLiteServer.ReaderStatus.SUCCESS &&
        reader['isCardPresent']) {
      indicatorClasses = 'reader-card-present-indicator';
    }
    const indicator = goog.dom.createDom('span', indicatorClasses);

    const indicatorContainer =
        goog.dom.createDom('span', 'reader-indicator-container', indicator);

    indicatorContainer.dataset.title = getTooltipText(reader);
    GSC.I18n.adjustElementTranslation(indicatorContainer);

    const text = makeReaderNameForDisplaying(reader['name']) +
        (reader['error'] ? ' (Error ' + reader['error'] + ')' : '');

    const element =
        goog.dom.createDom('li', undefined, indicatorContainer, text);
    goog.dom.append(readersListElement, element);
  }
}

function makeReaderNameForDisplaying(readerName) {
  for (let suffixToRemove of READER_NAME_SUFFIXES_TO_REMOVE) {
    if (goog.string.endsWith(readerName, suffixToRemove)) {
      const newReaderName =
          readerName.substr(0, readerName.length - suffixToRemove.length);
      logger.fine(
          'Transformed reader name "' + readerName + '" into "' +
          newReaderName + '"');
      return newReaderName;
    }
  }
  return readerName;
}

/**
 * @param {!GSC.PcscLiteServer.ReaderInfo} reader 
 * @return {string}
 */
function getTooltipText(reader) {
  switch (reader['status']) {
    case GSC.PcscLiteServer.ReaderStatus.INIT:
      return 'initReaderTooltip';
    case GSC.PcscLiteServer.ReaderStatus.SUCCESS:
      if (reader['isCardPresent']) return 'successCardTooltip';
      return 'successReaderTooltip';
    case GSC.PcscLiteServer.ReaderStatus.FAILURE:
      return 'failureReaderTooltip';
    default:
      GSC.Logging.failWithLogger(logger);
      return '';
  }
}

/**
 * @param {number} readersCount 
 */
function updateAddDeviceButtonText(readersCount) {
  if (readersCount == 0) {
    addDeviceElement.dataset.i18n = 'addDevice';
  } else {
    addDeviceElement.dataset.i18n = 'addAnotherDevice';
  }
  GSC.I18n.adjustElementTranslation(addDeviceElement);
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
  /**
   * Points to the "addOnUpdateListener" method of the ReaderTracker instance
   * that is owned by the background page.
   */
  const readerTrackerSubscriber =
      /** @type {function(function(!Array.<!GSC.PcscLiteServer.ReaderInfo>))} */
      (GSC.ObjectHelpers.extractKey(
           GSC.PopupWindow.Client.getData(), 'readerTrackerSubscriber'));
  // Start tracking the current list of readers.
  readerTrackerSubscriber(onReadersChanged);

  /**
   * Points to the "removeOnUpdateListener" method of the ReaderTracker instance
   * that is owned by the background page.
   */
  const readerTrackerUnsubscriber =
      /** @type {function(function(!Array.<!GSC.PcscLiteServer.ReaderInfo>))} */
      (GSC.ObjectHelpers.extractKey(
           GSC.PopupWindow.Client.getData(), 'readerTrackerUnsubscriber'));
  // Stop tracking the current list of readers when our window gets closed.
  chrome.app.window.current().onClosed.addListener(function() {
    readerTrackerUnsubscriber(onReadersChanged);
  });

  goog.events.listen(
      addDeviceElement, goog.events.EventType.CLICK, addDeviceClickListener);
};

});  // goog.scope
