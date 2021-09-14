/**
 * @license
 * Copyright 2016 Google Inc.
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

goog.provide('GoogleSmartCard.Libusb.ChromeUsbBackend');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.DeferredProcessor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.array');
goog.require('goog.functions');
goog.require('goog.iter');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

const REQUESTER_NAME = 'libusb_chrome_usb';

const GSC = GoogleSmartCard;
const debugDump = GSC.DebugDump.debugDump;
const RemoteCallMessage = GSC.RemoteCallMessage;

function isUsbApiAvailable() {
  return chrome !== undefined && chrome.usb !== undefined;
}

/**
 * This class implements handling of requests received from libusb NaCl port,
 * which is performed by executing the corresponding chrome.usb API methods and
 * forwarding their results back to the caller.
 * @param {!goog.messaging.AbstractChannel} naclModuleMessageChannel
 * @constructor
 */
GSC.Libusb.ChromeUsbBackend = function(naclModuleMessageChannel) {
  if (!isUsbApiAvailable()) {
    goog.log.warning(
        this.logger,
        'The USB API is not available. All USB requests will silently fail.');
  }

  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      REQUESTER_NAME, naclModuleMessageChannel, this.handleRequest_.bind(this));
  this.startObservingDevices_();

  /**
   * @type {!Array<function(string, !Array): !Array>}
   * Array of hooks that are called on every successful API request.
   * @private
   */
  this.requestSuccessHooks_ = [];

  /**
   * @type {!goog.promise.Resolver}
   * Gets resolved once setup is complete and API requests can be processed.
   * @private @const
   */
  this.setupDonePromiseResolver_ = goog.Promise.withResolver();

  /**
   * @type {!GoogleSmartCard.DeferredProcessor}
   * Queue of API requests that are not yet processed because setup is still in
   * progress.
   * @private @const
   */
  this.deferredProcessor_ =
      new GSC.DeferredProcessor(this.setupDonePromiseResolver_.promise);
};

const ChromeUsbBackend = GSC.Libusb.ChromeUsbBackend;

/**
 * @type {!goog.log.Logger}
 * @const
 */
ChromeUsbBackend.prototype.logger =
    GSC.Logging.getScopedLogger('Libusb.ChromeUsbBackend');

/**
 * Adds a function hook that is called whenever a request is resolved
 * successfully. The hook is called with the request name and its results
 * and can modify the results.
 * @param {function(string, !Array): !Array} hook parameters: API call and
 * results
 */
ChromeUsbBackend.prototype.addRequestSuccessHook = function(hook) {
  this.requestSuccessHooks_.push(hook);
};

/**
 * @param {function(string, !Array): !Array} hook
 */
ChromeUsbBackend.prototype.removeRequestSuccessHook = function(hook) {
  this.requestSuccessHooks_ = this.requestSuccessHooks_.filter(function(value) {
    return value !== hook;
  });
};

/**
 * Start processing API calls. All registered hooks must have completed setup.
 */
ChromeUsbBackend.prototype.startProcessingEvents = function() {
  this.setupDonePromiseResolver_.resolve();
};

/** @private */
ChromeUsbBackend.prototype.startObservingDevices_ = function() {
  if (!isUsbApiAvailable()) {
    // No event listeners are possible as the USB API is unavailable. No need to
    // log errors here, since this was already done in the constructor.
    return;
  }
  chrome.usb.onDeviceAdded.addListener(this.deviceAddedListener_.bind(this));
  chrome.usb.onDeviceRemoved.addListener(
      this.deviceRemovedListener_.bind(this));
  this.logCurrentDevices_();
};

/**
 * @param {!chrome.usb.Device} device
 * @private
 */
ChromeUsbBackend.prototype.deviceAddedListener_ = function(device) {
  goog.log.fine(
      this.logger, 'A USB device was added: ' + GSC.DebugDump.dump(device));
  this.logCurrentDevices_();
};

/**
 * @param {!chrome.usb.Device} device
 * @private
 */
ChromeUsbBackend.prototype.deviceRemovedListener_ = function(device) {
  goog.log.fine(
      this.logger, 'A USB device was removed: ' + GSC.DebugDump.dump(device));
  this.logCurrentDevices_();
};

/** @private */
ChromeUsbBackend.prototype.logCurrentDevices_ = function() {
  chrome.usb.getDevices({}, this.logDevices_.bind(this));
};

/**
 * @param {!Array.<!chrome.usb.Device>} devices
 * @private
 */
ChromeUsbBackend.prototype.logDevices_ = function(devices) {
  goog.array.sortByKey(devices, function(device) {
    return device.device;
  });
  goog.log.info(
      this.logger,
      devices.length +
          ' USB device(s) available: ' + GSC.DebugDump.dump(devices));
};

/**
 * @param {!Object} payload
 * @return {!goog.Promise}
 * @private
 */
ChromeUsbBackend.prototype.handleRequest_ = function(payload) {
  return this.deferredProcessor_.addJob(
      this.processRequest_.bind(this, payload));
};

/**
 * @param {!Object} payload
 * @return {!goog.Promise}
 * @private
 */
ChromeUsbBackend.prototype.processRequest_ = function(payload) {
  const remoteCallMessage = RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the remote call message: ' + debugDump(payload));
  }

  const debugRepresentation =
      'chrome.usb.' + remoteCallMessage.getDebugRepresentation();
  goog.log.fine(
      this.logger, 'Received a remote call request: ' + debugRepresentation);

  if (!isUsbApiAvailable()) {
    // No function call is possible as the USB API is unavailable. No need to
    // log errors here, since this was already done in the constructor.
    return goog.Promise.reject(new Error('USB API unavailable'));
  }

  const promiseResolver = goog.Promise.withResolver();

  const chromeUsbFunction =
      this.getChromeUsbFunction_(remoteCallMessage.functionName);
  const chromeUsbFunctionArgs = goog.array.concat(
      remoteCallMessage.functionArguments,
      this.chromeUsbApiGenericCallback_.bind(
          this, debugRepresentation, remoteCallMessage.functionName,
          promiseResolver));

  /** @preserveTry */
  try {
    chromeUsbFunction.apply(null, chromeUsbFunctionArgs);
  } catch (exc) {
    this.reportRequestException_(debugRepresentation, promiseResolver, exc);
  }

  return promiseResolver.promise;
};

/**
 * @param {string} functionName
 * @return {!Function}
 * @private
 */
ChromeUsbBackend.prototype.getChromeUsbFunction_ = function(functionName) {
  if (!goog.object.containsKey(chrome.usb, functionName) ||
      !goog.functions.isFunction(chrome.usb[functionName])) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Unknown chrome.usb API function requested: ' + functionName);
  }
  return chrome.usb[functionName];
};

/**
 * @param {string} debugRepresentation
 * @param {string} functionName API function that was called
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {...*} var_args Values passed to the callback by chrome.usb API.
 * @private
 */
ChromeUsbBackend.prototype.chromeUsbApiGenericCallback_ = function(
    debugRepresentation, functionName, promiseResolver, var_args) {
  const lastError = chrome.runtime.lastError;
  if (lastError !== undefined) {
    // FIXME(emaxx): Looks like the USB transfer timeouts also raise this
    // lastError flag, that is not suitable for us as we want to distinguish
    // the timeouts from the fatal errors.
    this.reportRequestError_(
        debugRepresentation, promiseResolver,
        goog.object.get(lastError, 'message', 'Unknown error'));
  } else {
    this.reportRequestSuccess_(
        debugRepresentation, functionName, promiseResolver,
        Array.prototype.slice.call(arguments, 3));
  }
};

/**
 * @param {string} debugRepresentation
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {*} exc
 * @private
 */
ChromeUsbBackend.prototype.reportRequestException_ = function(
    debugRepresentation, promiseResolver, exc) {
  goog.log.warning(
      this.logger,
      'JavaScript exception was thrown while calling ' + debugRepresentation +
          ': ' + exc);
  promiseResolver.reject(exc);
};

/**
 * @param {string} debugRepresentation
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {string} errorMessage
 * @private
 */
ChromeUsbBackend.prototype.reportRequestError_ = function(
    debugRepresentation, promiseResolver, errorMessage) {
  if (errorMessage == 'Transfer timed out.') {
    // FIXME(emaxx): Remove this special branch here once the USB transfer
    // timeouts support is implemented. This branch is useful before this is
    // done, as it suppresses the useless warning messages.
    goog.log.info(this.logger, 'Transfer timed out: ' + debugRepresentation);
  } else {
    goog.log.warning(
        this.logger,
        'API error occurred while calling ' + debugRepresentation + ': ' +
            errorMessage);
  }
  promiseResolver.reject(new Error(errorMessage));
};

/**
 * @param {string} debugRepresentation
 * @param {string} functionName API function that was called
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {!Array} resultArgs
 * @private
 */
ChromeUsbBackend.prototype.reportRequestSuccess_ = function(
    debugRepresentation, functionName, promiseResolver, resultArgs) {
  this.requestSuccessHooks_.forEach(function(hook) {
    resultArgs = hook(functionName, resultArgs);
  });
  goog.log.fine(
      this.logger,
      'Results returned by the ' + debugRepresentation + ' call: ' +
          goog.iter.join(goog.iter.map(resultArgs, debugDump), ', '));
  promiseResolver.resolve(resultArgs);
};
});  // goog.scope
