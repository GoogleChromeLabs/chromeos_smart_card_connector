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
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

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
  }

  /**
   * Handles a Libusb request sent from the C++ code in the executable module.
   * The result is returned asynchronously via the returned promise.
   * @param {!Object} payload
   * @return {!goog.Promise}
   * @private
   */
  onRequestReceivedFromExecutableModule_(payload) {
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

    // TODO(#429): Handle the requests in our class instead of forwarding them
    // to ChromeUsbBackend.
    return this.libusbChromeUsbBackend_.handleRequest(payload);
  }
};
});  // goog.scope
