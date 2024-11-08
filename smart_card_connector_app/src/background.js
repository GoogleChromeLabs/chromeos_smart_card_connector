/**
 * @license
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

goog.require('GoogleSmartCard.BackgroundPageUnloadPreventing');
goog.require('GoogleSmartCard.ConnectorApp.Background.MainWindowManaging');
goog.require('GoogleSmartCard.ConnectorApp.ChromeApiProvider');
goog.require('GoogleSmartCard.ConnectorApp.Window.Constants');
goog.require('GoogleSmartCard.EmscriptenModule');
goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.LibusbLoginStateHook');
goog.require('GoogleSmartCard.LibusbProxyReceiver');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPair');
goog.require('GoogleSmartCard.MessageChannelPool');
goog.require('GoogleSmartCard.MessagingCommon');
goog.require('GoogleSmartCard.MessagingOrigin');
goog.require('GoogleSmartCard.OffscreenDocEmscriptenModule');
goog.require('GoogleSmartCard.Packaging');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTracker');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.AdminPolicyService');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('GoogleSmartCard.SmartCardFilterLibusbHook');
goog.require('goog.asserts');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.BackgroundMain');

const extensionManifest = chrome.runtime.getManifest();
goog.log.info(
    logger,
    `The Smart Card Connector app (id "${chrome.runtime.id}", version ` +
        `${extensionManifest.version}) background script started. Browser ` +
        `version: "${globalThis.navigator.appVersion}".`);

/**
 * Loads the binary executable module depending on the toolchain configuration.
 * @return {!GSC.ExecutableModule}
 */
function createExecutableModule() {
  switch (GSC.ExecutableModule.TOOLCHAIN) {
    case GSC.ExecutableModule.Toolchain.EMSCRIPTEN:
      if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
        // Multi-threading in WebAssembly requires spawning Workers, which for
        // Extensions Manifest v3 is only allowed from an Offscreen Document but
        // not from a Service Worker (as of Chrome M126).
        return new GSC.OffscreenDocEmscriptenModule('executable_module');
      } else {
        return new GSC.EmscriptenModule('executable_module');
      }
  }
  GSC.Logging.fail(
      `Cannot load executable module: unknown toolchain ` +
      `${GSC.ExecutableModule.TOOLCHAIN}`);
  goog.asserts.fail();
}

/**
 * The binary executable module that contains the actual smart card client code.
 * @type {!GSC.ExecutableModule}
 */
const executableModule = createExecutableModule();
executableModule.addOnDisposeCallback(executableModuleDisposedListener);

const libusbProxyReceiver =
    new GSC.LibusbProxyReceiver(executableModule.getMessageChannel());
// Disable USB communication for the in-session instance of our extension
// whenever the ChromeOS Lock Screen is on, to avoid the access conflict with
// the lock-screen instance of ourselves that might be needed for user
// authentication.
libusbProxyReceiver.addHook(new GSC.LibusbLoginStateHook());
// Ignore non-smart-card USB devices and interfaces. This avoid spurious
// initialization failures in the CCID driver. Also it's a workaround for the
// case when attempts to connect to non-smart-card interfaces fail and cause the
// smart card interface connections to break as well.
libusbProxyReceiver.addHook(new GSC.SmartCardFilterLibusbHook());

const pcscLiteReadinessTracker =
    new GSC.PcscLiteServerClientsManagement.ReadinessTracker(
        executableModule.getMessageChannel());
const messageChannelPool = new GSC.MessageChannelPool();

const readerTrackerMessageChannelPair = new GSC.MessageChannelPair();
createClientHandler(readerTrackerMessageChannelPair.getFirst(), undefined);
const readerTracker = new GSC.PcscLiteServer.ReaderTracker(
    executableModule.getMessageChannel(),
    readerTrackerMessageChannelPair.getSecond(), executableModule.getLogger());

createAdminPolicyService();

executableModule.startLoading();

// Launching is only possible for an app; in case of the extension packaging
// mode, the user should click on the browser action icon next to the address
// bar.
if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP)
  chrome.app.runtime.onLaunched.addListener(launchedListener);

chrome.runtime.onConnect.addListener(onMessagePortConnected);
chrome.runtime.onConnectExternal.addListener(externalConnectionListener);
chrome.runtime.onMessageExternal.addListener(externalMessageListener);

if (GSC.ExecutableModule.TOOLCHAIN ===
        GSC.ExecutableModule.Toolchain.EMSCRIPTEN &&
    GSC.Packaging.MODE !== GSC.Packaging.Mode.EXTENSION) {
  // Open a message channel to an invisible iframe, in order to keep our
  // background page always alive, and hence be responsive to incoming smart
  // card requests. The user who doesn't like extra resource usage can uninstall
  // our application if they don't actually use smart cards.
  // This code doesn't work in manifest v3, hence it's disabled in the
  // `EXTENSION` packaging mode.
  GSC.BackgroundPageUnloadPreventing.enable();
}

if (chrome.smartCardProviderPrivate !== undefined) {
  // This object's lifetime is bound to the message channel's one, hence we
  // don't need to store it in a variable.
  new GSC.ConnectorApp.ChromeApiProvider(
      executableModule.getMessageChannel(), pcscLiteReadinessTracker.promise);
} else {
  goog.log.info(
      logger,
      `chrome.smartCardProviderPrivate is not available. ` +
          `Requests from Web Smart Card API will not be handled.`);
}

/**
 * Called when the executable module is disposed of.
 */
function executableModuleDisposedListener() {
  if (!goog.DEBUG) {
    // Trigger the fatal error in the Release mode, that will emit an error
    // message and trigger the app reload.
    GSC.Logging.failWithLogger(logger, 'Executable module was disposed of');
  }
}

/**
 * Schedules the listener to be called when the executable module gets disposed
 * of (e.g., because of a crash).
 *
 * The listener is called immediately if this already happened.
 * @param {!function()} listener
 */
function addOnExecutableModuleDisposedListener(listener) {
  executableModule.addOnDisposeCallback(() => {
    listener();
  });
}

/**
 * Called when the onLaunched event is received (that is, when the user clicks
 * on the app in the ChromeOS app launcher).
 */
function launchedListener() {
  goog.log.fine(logger, 'Received onLaunched event, opening window...');
  GSC.ConnectorApp.Background.MainWindowManaging.openWindowDueToUserRequest();
}

/**
 * Called when the onConnectExternal event is received.
 * @param {!Port} port
 */
function externalConnectionListener(port) {
  goog.log.fine(logger, 'Received onConnectExternal event');
  const portMessageChannel = new GSC.PortMessageChannel(port);
  if (portMessageChannel.messagingOrigin === null) {
    goog.log.warning(
        logger,
        'Ignoring the external connection as there is no sender specified');
    return;
  }
  // The opener will send PC/SC commands (or garbage if it's broken) - create a
  // handler for processing these commands. Also add it to the pool of opened
  // ports, so that subscribers (like UI) will learn about this new caller.
  messageChannelPool.addChannel(
      portMessageChannel.messagingOrigin, portMessageChannel);
  GSC.MessagingCommon.setNonFatalDefaultServiceCallback(portMessageChannel);
  createClientHandler(portMessageChannel, portMessageChannel.messagingOrigin);
}

/**
 * Called when the onMessageExternal event is received.
 * @param {*} message
 * @param {!MessageSender} sender
 */
function externalMessageListener(message, sender) {
  goog.log.fine(logger, 'Received onMessageExternal event');
  if (sender.id === undefined) {
    // Note: The single-message-based communication protocol doesn't support
    // talking to web pages, since there's no way to send messages to web pages
    // other than in response to messages from them.
    goog.log.warning(
        logger,
        'Ignoring the external message as there is no sender ' +
            'extension id specified');
    return;
  }
  let channel = getOrCreateSingleMessageBasedChannel(sender.id);
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
 * Called when another page within our extension opened a port to us (the
 * Service Worker or, if we're a Chrome App, the Background Page).
 * @param {!Port} port
 */
function onMessagePortConnected(port) {
  if (port.name ===
      GSC.ConnectorApp.Window.Constants.DEVICE_LIST_MESSAGING_PORT_NAME) {
    setUpSendingDeviceList(port);
  } else if (port.name ===
      GSC.ConnectorApp.Window.Constants.CLIENT_APP_LIST_MESSAGING_PORT_NAME) {
    setUpSendingAppList(port);
  }
  // Ignore the port otherwise.
}

/**
 * Start sending the current list of devices over the given port (it's created
 * by the window).
 * @param {!Port} port
 */
function setUpSendingDeviceList(port) {
  const channel = new GSC.PortMessageChannel(port);

  function onReadersUpdated(readers) {
    channel.send('', readers);
  }

  readerTracker.addOnUpdateListener(onReadersUpdated);
  channel.addOnDisposeCallback(() => {
    readerTracker.removeOnUpdateListener(onReadersUpdated);
  });
}

/**
 * Start sending the current list of connected client apps over the given port
 * (it's created by the window).
 * @param {!Port} port
 */
function setUpSendingAppList(port) {
  const channel = new GSC.PortMessageChannel(port);

  function onClientsUpdated(clientOriginList) {
    channel.send('', clientOriginList);
  }

  messageChannelPool.addOnUpdateListener(onClientsUpdated);
  channel.addOnDisposeCallback(() => {
    readerTracker.removeOnUpdateListener(onClientsUpdated);
  });
}

/**
 * Returns a single message based channel instance that talks to the specified
 * extension - either returns an existing instance if there's one, or creates a
 * new one otherwise. May return null if the channel becomes immediately
 * disposed of due to some error.
 * @param {string} senderExtensionId
 * @return {GSC.SingleMessageBasedChannel?}
 */
function getOrCreateSingleMessageBasedChannel(senderExtensionId) {
  const messagingOrigin =
      GSC.MessagingOrigin.getFromExtensionId(senderExtensionId);
  const existingChannel =
      messageChannelPool.getChannels(messagingOrigin).find(function(channel) {
        return channel instanceof GSC.SingleMessageBasedChannel;
      });
  if (existingChannel)
    return /** @type {!GSC.SingleMessageBasedChannel} */ (existingChannel);
  const newChannel =
      new GSC.SingleMessageBasedChannel(senderExtensionId, undefined, true);
  messageChannelPool.addChannel(messagingOrigin, newChannel);
  GSC.MessagingCommon.setNonFatalDefaultServiceCallback(newChannel);
  createClientHandler(newChannel, messagingOrigin);
  if (newChannel.isDisposed())
    return null;
  return newChannel;
}

/**
 * Creates the class that provides the C++ layer with the AdminPolicy obtained
 * from the managed storage.
 */
function createAdminPolicyService() {
  if (executableModule.isDisposed() ||
      executableModule.getMessageChannel().isDisposed()) {
    goog.log.warning(
        logger,
        'Could not create AdminPolicyService as the server is disposed.');
    return;
  }
  // This object's lifetime is bound to the message channel's one, hence we
  // don't need to store it in a variable.
  new GSC.PcscLiteServerClientsManagement.AdminPolicyService(
      executableModule.getMessageChannel(), pcscLiteReadinessTracker);
}

/**
 * Creates a PC/SC-Lite client handler for the specified message channel and the
 * client extension.
 * The clientOrigin argument equal to undefined denotes the case when the sender
 * is our own extension/app.
 * @param {!goog.messaging.AbstractChannel} clientMessageChannel
 * @param {string|undefined} clientOrigin
 */
function createClientHandler(clientMessageChannel, clientOrigin) {
  GSC.Logging.checkWithLogger(logger, !clientMessageChannel.isDisposed());
  const clientOriginForLog =
      clientOrigin !== undefined ? clientOrigin : 'ourselves';

  if (executableModule.isDisposed() ||
      executableModule.getMessageChannel().isDisposed()) {
    goog.log.warning(
        logger,
        'Could not create PC/SC-Lite client handler for ' + clientOriginForLog +
            ' as the server is disposed. Disposing of the client message ' +
            'channel...');
    clientMessageChannel.dispose();
    return;
  }

  // Note: the reference to the created client handler is not stored anywhere,
  // because it manages its lifetime itself, based on the lifetimes of the
  // passed message channels.
  const clientHandler = new GSC.PcscLiteServerClientsManagement.ClientHandler(
      executableModule.getMessageChannel(), pcscLiteReadinessTracker.promise,
      clientMessageChannel, clientOrigin);

  const logMessage = 'Created a new PC/SC-Lite client handler for ' +
      clientOriginForLog + ' (handler id ' + clientHandler.id + ')';
  if (clientOrigin !== undefined)
    goog.log.info(logger, logMessage);
  else
    goog.log.fine(logger, logMessage);
}

/**
 * Sets global variables to be used by the main window (once it's opened).
 */
function exposeGlobalsForMainWindow() {
  goog.global['googleSmartCard_executableModuleDisposalSubscriber'] =
      addOnExecutableModuleDisposedListener;
}

exposeGlobalsForMainWindow();
});  // goog.scope
