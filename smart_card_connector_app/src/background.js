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

goog.provide('GoogleSmartCard.ConnectorApp.BackgroundMain');

goog.require('GoogleSmartCard.Libusb.ChromeUsbBackend');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.PopupWindow.Server');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');

goog.scope(function() {

/** @const */
var WINDOW_URL = 'window.html';

/**
 * Note: the bounds of the window were carefully adjusted in order to fit the
 * USB device selector dialog (as shown by the chrome.usb.getUserSelectedDevices
 * method: refer to
 * <https://developer.chrome.com/apps/usb#method-getUserSelectedDevices>).
 * @const
 */
var WINDOW_OPTIONS = {
  'frame': 'none',
  'hidden': true,
  'id': 'window',
  'innerBounds': {
    'width': 530,
    'height': 430
  },
  'resizable': false
};

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('ConnectorApp.BackgroundMain');

logger.fine('The background script has started...');

var naclModule = new GSC.NaclModule(
    'nacl_module.nmf', GSC.NaclModule.Type.PNACL);

var libusbChromeUsbBackend = new GSC.Libusb.ChromeUsbBackend(
    naclModule.messageChannel);
var pcscLiteReadinessTracker =
    new GSC.PcscLiteServerClientsManagement.ReadinessTracker(
        naclModule.messageChannel);

naclModule.load();

chrome.app.runtime.onLaunched.addListener(launchedListener);

chrome.runtime.onConnectExternal.addListener(externalConnectionListener);

chrome.runtime.onInstalled.addListener(installedListener);

function launchedListener() {
  logger.fine('Received onLaunched event, opening window...');
  openWindow();
}

function openWindow() {
  GSC.PopupWindow.Server.createWindow(WINDOW_URL, WINDOW_OPTIONS);
}

/**
 * @param {!Port} port
 */
function externalConnectionListener(port) {
  logger.fine('Received onConnectExternal event, creating the PC/SC-Lite ' +
              'client handler...');
  var portMessageChannel = new GSC.PortMessageChannel(port);
  var clientHandler = new GSC.PcscLiteServerClientsManagement.ClientHandler(
      naclModule.messageChannel,
      pcscLiteReadinessTracker,
      portMessageChannel,
      goog.isNull(portMessageChannel.extensionId) ?
          undefined : portMessageChannel.extensionId);
}

/**
 * @param {!Object} details
 */
function installedListener(details) {
  GSC.Logging.checkWithLogger(
      logger, goog.object.containsKey(details, 'reason'));
  var reason = details['reason'];
  GSC.Logging.checkWithLogger(logger, goog.isString(reason));
  if (reason == 'install') {
    logger.fine('Received onInstalled event due to the App installation, ' +
                'opening window...');
    openWindow();
  }
}

});  // goog.scope
