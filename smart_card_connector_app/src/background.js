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

goog.require('GoogleSmartCard.ConnectorApp.Background.MainWindowManaging');
goog.require('GoogleSmartCard.Libusb.ChromeLoginStateHook');
goog.require('GoogleSmartCard.Libusb.ChromeUsbBackend');
goog.require('GoogleSmartCard.LogBufferForwarder');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPair');
goog.require('GoogleSmartCard.MessageChannelPool');
goog.require('GoogleSmartCard.MessagingCommon');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTracker');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

const JS_LOGS_HANDLER_MESSAGE_TYPE = 'js_logs_handler';

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('ConnectorApp.BackgroundMain');

// Used to forward logs collected on the JS side to the NaCl module's stdout, in
// order to simplify accessing them in some configurations.
// Note that this object needs to be created as early as possible, in order to
// not miss any important log.
const logBufferForwarderToNaclModule = new GSC.LogBufferForwarder(
    GSC.Logging.getLogBuffer(), JS_LOGS_HANDLER_MESSAGE_TYPE);

const extensionManifest = chrome.runtime.getManifest();
logger.info(
    `The Smart Card Connector app (id "${chrome.runtime.id}", version ` +
    `${extensionManifest.version}) background script started. Browser ` +
    `version: "${window.navigator.appVersion}".`);

var naclModule = new GSC.NaclModule(
    'nacl_module.nmf', GSC.NaclModule.Type.PNACL);
naclModule.addOnDisposeCallback(naclModuleDisposedListener);

naclModule.getLoadPromise().then(() => {
  // Skip forwarding logs that were received from the NaCl module or generated
  // while sending messages to it, in order to avoid duplication and/or infinite
  // recursion.
  logBufferForwarderToNaclModule.ignoreLogger(
      naclModule.logMessagesReceiver.logger.getName());
  // Start forwarding all future log messages collected on the JS side, but also
  // immediately post the messages that have been accumulated so far.
  logBufferForwarderToNaclModule.startForwarding(
      naclModule.getMessageChannel());
}, () => {});

var libusbChromeUsbBackend = new GSC.Libusb.ChromeUsbBackend(
    naclModule.getMessageChannel());
var chromeLoginStateHook = new GSC.Libusb.ChromeLoginStateHook();
libusbChromeUsbBackend.addRequestSuccessHook(
    chromeLoginStateHook.getRequestSuccessHook());
// Start the backend regardless of whether the hook initialization succeeded.
chromeLoginStateHook.getHookReadyPromise().then(() => {}, () => {}).then(
    function() { libusbChromeUsbBackend.startProcessingEvents(); });

var pcscLiteReadinessTracker =
    new GSC.PcscLiteServerClientsManagement.ReadinessTracker(
        naclModule.getMessageChannel());
var messageChannelPool = new GSC.MessageChannelPool;

var readerTrackerMessageChannelPair = new GSC.MessageChannelPair;
createClientHandler(readerTrackerMessageChannelPair.getFirst(), undefined);
var readerTracker = new GSC.PcscLiteServer.ReaderTracker(
    naclModule.getMessageChannel(),
    readerTrackerMessageChannelPair.getSecond(),
    naclModule.getLogger());

naclModule.startLoading();

chrome.app.runtime.onLaunched.addListener(launchedListener);

chrome.runtime.onConnect.addListener(connectionListener);
chrome.runtime.onConnectExternal.addListener(externalConnectionListener);
chrome.runtime.onMessageExternal.addListener(externalMessageListener);

/**
 * Called when the NaCl module is disposed of.
 */
function naclModuleDisposedListener() {
  if (!goog.DEBUG) {
    // Trigger the fatal error in the Release mode, that will emit an error
    // message and trigger the app reload.
    GSC.Logging.failWithLogger(logger, 'Server NaCl module was disposed of');
  }
}

/**
 * Called when the onLaunched event is received (that is, when the user clicks
 * on the app in the Chrome OS app launcher).
 */
function launchedListener() {
  logger.fine('Received onLaunched event, opening window...');
  GSC.ConnectorApp.Background.MainWindowManaging.openWindowDueToUserRequest(
      makeDataForMainWindow());
}

/**
 * Called when the onConnect event is received.
 * @param {!Port} port
 */
function connectionListener(port) {
  logger.fine('Received onConnect event');
  var portMessageChannel = new GSC.PortMessageChannel(port);
  createClientHandler(portMessageChannel, undefined);
}

/**
 * Called when the onConnectExternal event is received.
 * @param {!Port} port
 */
function externalConnectionListener(port) {
  logger.fine('Received onConnectExternal event');
  var portMessageChannel = new GSC.PortMessageChannel(port);
  if (portMessageChannel.extensionId === null) {
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
 * Called when the onMessageExternal event is received.
 * @param {*} message
 * @param {!MessageSender} sender
 */
function externalMessageListener(message, sender) {
  logger.fine('Received onMessageExternal event');
  if (sender.id === undefined) {
    logger.warning('Ignoring the external message as there is no sender ' +
                   'extension id specified');
    return;
  }
  var channel = getOrCreateSingleMessageBasedChannel(sender.id);
  if (!channel)
    return;
  channel.deliverMessage(message);
  if (channel.isDisposed()) {
    // The channel could get disposed of due to the detected client reload. So
    // here an attempt is made to deliver the same message again to a freshly
    // created message channel, so that the message from the reloaded client has
    // a chance to be handled.
    channel = getOrCreateSingleMessageBasedChannel(sender.id);
    if (!channel)
      return;
    channel.deliverMessage(message);
  }
}

/**
 * Returns a single message based channel instance that talks to the specified
 * extension - either returns an existing instance if there's one, or creates a
 * new one otherwise. May return null if the channel becomes immediately
 * disposed of due to some error.
 * @param {string} clientExtensionId
 * @return {GSC.SingleMessageBasedChannel?}
 */
function getOrCreateSingleMessageBasedChannel(clientExtensionId) {
  var existingChannel = messageChannelPool.getChannels(clientExtensionId).find(
      function(channel) {
        return channel instanceof GSC.SingleMessageBasedChannel;
      });
  if (existingChannel)
    return /** @type {!GSC.SingleMessageBasedChannel} */ (existingChannel);
  var newChannel = new GSC.SingleMessageBasedChannel(
      clientExtensionId, undefined, true);
  messageChannelPool.addChannel(clientExtensionId, newChannel);
  GSC.MessagingCommon.setNonFatalDefaultServiceCallback(newChannel);
  createClientHandler(newChannel, clientExtensionId);
  if (newChannel.isDisposed())
    return null;
  return newChannel;
}

/**
 * Creates a PC/SC-Lite client handler for the specified message channel and the
 * client extension.
 * @param {!goog.messaging.AbstractChannel} clientMessageChannel
 * @param {string|undefined} clientExtensionId
 */
function createClientHandler(clientMessageChannel, clientExtensionId) {
  GSC.Logging.checkWithLogger(logger, !clientMessageChannel.isDisposed());

  var clientTitleForLog = clientExtensionId !== undefined ?
      'app "' + clientExtensionId + '"' : 'own app';

  if (naclModule.isDisposed() || naclModule.getMessageChannel().isDisposed()) {
    logger.warning(
        'Could not create PC/SC-Lite client handler for ' + clientTitleForLog +
        ' as the server is disposed. Disposing of the client message ' +
        'channel...');
    clientMessageChannel.dispose();
    return;
  }

  // Note: the reference to the created client handler is not stored anywhere,
  // because it manages its lifetime itself, based on the lifetimes of the
  // passed message channels.
  var clientHandler = new GSC.PcscLiteServerClientsManagement.ClientHandler(
      naclModule.getMessageChannel(),
      pcscLiteReadinessTracker,
      clientMessageChannel,
      clientExtensionId);

  var logMessage =
      'Created a new PC/SC-Lite client handler for ' + clientTitleForLog +
      ' (handler id ' + clientHandler.id + ')';
  if (clientExtensionId !== undefined)
    logger.info(logMessage);
  else
    logger.fine(logMessage);
}

/**
 * Creates data for passing into the opened main window.
 * @return {!Object}
 */
function makeDataForMainWindow() {
  return {
    'clientAppListUpdateSubscriber':
        messageChannelPool.addOnUpdateListener.bind(messageChannelPool),
    'readerTrackerSubscriber':
        readerTracker.addOnUpdateListener.bind(readerTracker),
    'readerTrackerUnsubscriber':
        readerTracker.removeOnUpdateListener.bind(readerTracker)};
}

});  // goog.scope
