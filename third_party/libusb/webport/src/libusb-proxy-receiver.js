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
goog.require('GoogleSmartCard.LibusbProxyHook');
goog.require('GoogleSmartCard.LibusbToChromeUsbAdaptor');
goog.require('GoogleSmartCard.LibusbToJsApiAdaptor');
goog.require('GoogleSmartCard.LibusbToWebusbAdaptor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.StubLibusbToJsApiAdaptor');
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

// This constant must match the string in libusb_js_proxy.cc.
const EXECUTABLE_MODULE_REQUESTER_NAME = 'libusb';

const GSC = GoogleSmartCard;
const debugDumpSanitized = GSC.DebugDump.debugDumpSanitized;

const logger = GSC.Logging.getScopedLogger('LibusbProxyReceiver');

/**
 * This class implements handling of libusb requests received from the
 * executable module (the `LibusbJsProxy` class). The requests are handled by
 * transforming them into the chrome.usb API requests.
 */
GSC.LibusbProxyReceiver = class {
  /**
   * @param {!goog.messaging.AbstractChannel} executableModuleMessageChannel
   */
  constructor(executableModuleMessageChannel) {
    // Note: the request receiver instance is not stored anywhere, as it makes
    // itself being owned by the message channel.
    new GSC.RequestReceiver(
        EXECUTABLE_MODULE_REQUESTER_NAME, executableModuleMessageChannel,
        this.onRequestReceivedFromExecutableModule_.bind(this));
    /** @private @const */
    this.realLibusbToJsApiAdaptor_ = chooseLibusbToJsApiAdaptor();
    /** @type {!Array<!GSC.LibusbProxyHook>} @private @const */
    this.hooks_ = [];
  }

  /**
   * Adds a new hook. Multiple hooks are chained in the FILO order.
   * @param {!GSC.LibusbProxyHook} hook
   */
  addHook(hook) {
    // If it's the first hook, then chain it with the real implementation,
    // otherwise - with the previously added hook.
    hook.setDelegate(this.getLibusbToJsApiAdaptor_());
    this.hooks_.push(hook);
  }

  /**
   * @return {!GSC.LibusbToJsApiAdaptor}
   * @private
   */
  getLibusbToJsApiAdaptor_() {
    if (this.hooks_.length)
      return this.hooks_[this.hooks_.length - 1];
    return this.realLibusbToJsApiAdaptor_;
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
          'Failed to parse the remote call message: ' +
              debugDumpSanitized(payload));
    }
    goog.log.fine(
        logger,
        `Received a remote call request: ${
            remoteCallMessage.getDebugRepresentation()}`);
    return await this.dispatchLibusbJsFunction_(remoteCallMessage);
  }

  /**
   * @param {!GSC.RemoteCallMessage} remoteCallMessage
   * @return {!Promise<!Array>}
   * @private
   */
  async dispatchLibusbJsFunction_(remoteCallMessage) {
    // Note: The function name strings must match the ones in
    // libusb_js_proxy.cc.
    switch (remoteCallMessage.functionName) {
      case 'listDevices':
        return [await this.getLibusbToJsApiAdaptor_().listDevices(
            ...remoteCallMessage.functionArguments)];
      case 'getConfigurations':
        return [await this.getLibusbToJsApiAdaptor_().getConfigurations(
            ...remoteCallMessage.functionArguments)];
      case 'openDeviceHandle':
        return [await this.getLibusbToJsApiAdaptor_().openDeviceHandle(
            ...remoteCallMessage.functionArguments)];
      case 'closeDeviceHandle':
        await this.getLibusbToJsApiAdaptor_().closeDeviceHandle(
            ...remoteCallMessage.functionArguments);
        return [];
      case 'claimInterface':
        await this.getLibusbToJsApiAdaptor_().claimInterface(
            ...remoteCallMessage.functionArguments);
        return [];
      case 'releaseInterface':
        await this.getLibusbToJsApiAdaptor_().releaseInterface(
            ...remoteCallMessage.functionArguments);
        return [];
      case 'resetDevice':
        await this.getLibusbToJsApiAdaptor_().resetDevice(
            ...remoteCallMessage.functionArguments);
        return [];
      case 'controlTransfer':
        return [await this.getLibusbToJsApiAdaptor_().controlTransfer(
            ...remoteCallMessage.functionArguments)];
      case 'bulkTransfer':
        return [await this.getLibusbToJsApiAdaptor_().bulkTransfer(
            ...remoteCallMessage.functionArguments)];
      case 'interruptTransfer':
        return [await this.getLibusbToJsApiAdaptor_().interruptTransfer(
            ...remoteCallMessage.functionArguments)];
    }
    throw new Error(
        `Unknown libusb-JS function ${remoteCallMessage.functionName}`);
  }
};

/** @return {!GSC.LibusbToJsApiAdaptor} */
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
  return new GSC.StubLibusbToJsApiAdaptor();
}
});  // goog.scope
