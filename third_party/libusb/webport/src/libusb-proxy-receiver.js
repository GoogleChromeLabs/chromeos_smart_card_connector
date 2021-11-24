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

goog.provide('GoogleSmartCard.LibusbProxyReceiver');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Libusb.ChromeUsbBackend');
goog.require('GoogleSmartCard.LibusbToChromeUsbAdaptor');
goog.require('GoogleSmartCard.LibusbToJsApiAdaptor');
goog.require('GoogleSmartCard.LibusbToWebusbAdaptor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

// This constant must match the string in libusb_js_proxy.cc.
const EXECUTABLE_MODULE_REQUESTER_NAME = 'libusb';

const GSC = GoogleSmartCard;
const debugDump = GSC.DebugDump.debugDump;

const logger = GSC.Logging.getScopedLogger('LibusbProxyReceiver');

/**
 * This class implements handling of libusb requests received from the
 * executable module (the `LibusbJsProxy` class). The requests are handled by
 * transforming them into the chrome.usb API requests.
 *
 * TODO(#429): Stop receiving the ChromeUsbBackend, and generalize to support
 * WebUSB as well.
 */
GSC.LibusbProxyReceiver = class {
  /**
   * @param {!goog.messaging.AbstractChannel} executableModuleMessageChannel
   * @param {!GSC.Libusb.ChromeUsbBackend} libusbChromeUsbBackend
   */
  constructor(executableModuleMessageChannel, libusbChromeUsbBackend) {
    // Note: the request receiver instance is not stored anywhere, as it makes
    // itself being owned by the message channel.
    new GSC.RequestReceiver(
        EXECUTABLE_MODULE_REQUESTER_NAME, executableModuleMessageChannel,
        this.onRequestReceivedFromExecutableModule_.bind(this));
    /** @private @const */
    this.libusbChromeUsbBackend_ = libusbChromeUsbBackend;
    /** @private @const */
    this.libusbToJsApiAdaptor_ = chooseLibusbToJsApiAdaptor();
  }

  /**
   * Handles a Libusb request sent from the C++ code in the executable module.
   * The result is returned asynchronously via the returned promise.
   * @param {!Object} payload
   * @return {!Promise<!Array>}
   * @private
   */
  async onRequestReceivedFromExecutableModule_(payload) {
    const remoteCallMessage =
        GSC.RemoteCallMessage.parseRequestPayload(payload);
    if (!remoteCallMessage) {
      GSC.Logging.failWithLogger(
          logger,
          'Failed to parse the remote call message: ' + debugDump(payload));
    }
    goog.log.fine(
        logger,
        `Received a remote call request: ${
            remoteCallMessage.getDebugRepresentation()}`);

    if (!this.libusbToJsApiAdaptor_) {
      // No need to log errors here, since this was already done in the
      // constructor.
      throw new Error('USB API unavailable');
    }
    const result = await this.dispatchLibusbJsFunction_(remoteCallMessage);
    return [result];
  }

  /**
   * @param {!GSC.RemoteCallMessage} remoteCallMessage
   * @return {!Promise<*>}
   * @private
   */
  async dispatchLibusbJsFunction_(remoteCallMessage) {
    // Note: The function name strings must match the ones in
    // libusb_js_proxy.cc.
    switch (remoteCallMessage.functionName) {
      case 'listDevices':
        return await this.libusbToJsApiAdaptor_.listDevices(
            ...remoteCallMessage.functionArguments);
      case 'getConfigurations':
        return await this.libusbToJsApiAdaptor_.getConfigurations(
            ...remoteCallMessage.functionArguments);
    }
    // TODO(#429): Delete this fallback to ChromeUsbBackend once all functions
    // are implemented in LibusbToJsApiAdaptor.
    return await this.libusbChromeUsbBackend_.handleRequest(
        remoteCallMessage.makeRequestPayload());
  }
};

/** @return {!GSC.LibusbToJsApiAdaptor|null} */
function chooseLibusbToJsApiAdaptor() {
  // chrome.usb takes precedence: WebUSB might be available but have
  // insufficient permissions.
  if (GSC.LibusbToChromeUsbAdaptor.isApiAvailable()) {
    goog.log.fine(logger, 'Using chrome.usb API');
    return new GSC.LibusbToChromeUsbAdaptor();
  }
  if (GSC.LibusbToWebusbAdaptor.isApiAvailable()) {
    goog.log.fine(logger, 'Using WebUSB API');
    return new GSC.LibusbToWebusbAdaptor();
  }
  goog.log.warning(
      logger,
      'The USB API is not available. All USB requests will silently fail.');
  return null;
}
});  // goog.scope
