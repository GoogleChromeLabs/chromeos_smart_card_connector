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
goog.require('GoogleSmartCard.MessagingCommon');
goog.require('GoogleSmartCard.MessageChannelPool');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.PopupWindow.Server');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');

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
naclModule.addOnDisposeCallback(naclModuleDisposedListener);

var libusbChromeUsbBackend = new GSC.Libusb.ChromeUsbBackend(
    naclModule.messageChannel);
var pcscLiteReadinessTracker =
    new GSC.PcscLiteServerClientsManagement.ReadinessTracker(
        naclModule.messageChannel);
var messageChannelPool = new GSC.MessageChannelPool();

naclModule.load();

chrome.app.runtime.onLaunched.addListener(launchedListener);

chrome.runtime.onConnect.addListener(connectionListener);
chrome.runtime.onConnectExternal.addListener(externalConnectionListener);
chrome.runtime.onMessageExternal.addListener(externalMessageListener);

chrome.runtime.onInstalled.addListener(installedListener);

function naclModuleDisposedListener() {
  GSC.Logging.failWithLogger(logger, 'Server NaCl module was disposed of');
}

function launchedListener() {
  logger.fine('Received onLaunched event, opening window...');
  openWindow();
}

function openWindow() {
  var data = {'clientAppListUpdateSubscriber':
      messageChannelPool.addOnUpdateListener.bind(messageChannelPool)};
  GSC.PopupWindow.Server.createWindow(WINDOW_URL, WINDOW_OPTIONS, data);
}

/**
 * @param {!Port} port
 */
function connectionListener(port) {
  logger.fine('Received onConnect event');
  var portMessageChannel = new GSC.PortMessageChannel(port);
  createClientHandler(portMessageChannel, undefined);
}

/**
 * @param {!Port} port
 */
function externalConnectionListener(port) {
  logger.fine('Received onConnectExternal event');
  var portMessageChannel = new GSC.PortMessageChannel(port);
  if (goog.isNull(portMessageChannel.extensionId)) {
    logger.warning('Ignoring the external connection as there is no sender ' +
                   'extension id specified');
    return;
  }
  messageChannelPool.addChannel(
      portMessageChannel.extensionId, portMessageChannel);
  GSC.MessagingCommon.setNonFatalDefaultServiceCallback(portMessageChannel);
  createClientHandler(portMessageChannel, portMessageChannel.extensionId);
}

/**
 * @param {*} message
 * @param {!MessageSender} sender
 */
function externalMessageListener(message, sender) {
  logger.fine('Received onMessageExternal event');
  if (!goog.isDef(sender.id)) {
    logger.warning('Ignoring the external message as there is no sender ' +
                   'extension id specified');
    return;
  }
  var channel = messageChannelPool.getChannels(sender.id).find(
      function(channel) {
        return channel instanceof GSC.SingleMessageBasedChannel;
      });
  if (!channel) {
    channel = new GSC.SingleMessageBasedChannel(sender.id);
    messageChannelPool.addChannel(sender.id, channel);
    GSC.MessagingCommon.setNonFatalDefaultServiceCallback(channel);
    createClientHandler(channel, sender.id);
  }
  channel.deliverMessage(message);
}

/**
 * @param {!goog.messaging.AbstractChannel} clientMessageChannel
 * @param {string|undefined} clientExtensionId
 */
function createClientHandler(clientMessageChannel, clientExtensionId) {
  logger.info(
      'Creating a new PC/SC-Lite client handler ' +
      (goog.isDef(clientExtensionId) ?
           '(client extension id is "' + clientExtensionId + '")' :
           '(client is the own App)') +
      '...');
  GSC.Logging.checkWithLogger(logger, !clientMessageChannel.isDisposed());

  if (naclModule.isDisposed() || naclModule.messageChannel.isDisposed()) {
    logger.warning(
        'Could not create PC/SC-Lite client handler as the server is ' +
        'disposed. Disposing of the client message channel...');
    clientMessageChannel.dispose();
    return;
  }

  // Note: the reference to the created client handler is not stored anywhere,
  // because it manages its lifetime itself, based on the lifetimes of the
  // passed message channels.
  new GSC.PcscLiteServerClientsManagement.ClientHandler(
      naclModule.messageChannel,
      pcscLiteReadinessTracker,
      clientMessageChannel,
      clientExtensionId);
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
