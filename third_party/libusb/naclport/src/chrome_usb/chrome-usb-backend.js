/** @license
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
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.iter');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

/** @const */
var REQUESTER_NAME = 'libusb_chrome_usb';

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var debugDump = GSC.DebugDump.debugDump;
/** @const */
var RemoteCallMessage = GSC.RemoteCallMessage;

/**
 * This class implements handling of requests received from libusb NaCl port,
 * which is performed by executing the corresponding chrome.usb API methods and
 * forwarding their results back to the caller.
 * @param {!goog.messaging.AbstractChannel} naclModuleMessageChannel
 * @constructor
 */
GSC.Libusb.ChromeUsbBackend = function(naclModuleMessageChannel) {
  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      REQUESTER_NAME, naclModuleMessageChannel, this.handleRequest_.bind(this));

  this.startObservingDevices_();
};

/** @const */
var ChromeUsbBackend = GSC.Libusb.ChromeUsbBackend;

/**
 * @type {!goog.log.Logger}
 * @const
 */
ChromeUsbBackend.prototype.logger = GSC.Logging.getScopedLogger(
    'Libusb.ChromeUsbBackend');

/** @private */
ChromeUsbBackend.prototype.startObservingDevices_ = function() {
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
  this.logger.fine('A USB device was added: ' + GSC.DebugDump.dump(device));
  this.logCurrentDevices_();
};

/**
 * @param {!chrome.usb.Device} device
 * @private
 */
ChromeUsbBackend.prototype.deviceRemovedListener_ = function(device) {
  this.logger.fine('A USB device was removed: ' + GSC.DebugDump.dump(device));
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
  goog.array.sortByKey(devices, function(device) { return device.device; });
  this.logger.info(devices.length + ' USB device(s) available: ' +
                   GSC.DebugDump.dump(devices));
};

/**
 * @param {!Object} payload
 * @return {!goog.Promise}
 * @private
 */
ChromeUsbBackend.prototype.handleRequest_ = function(payload) {
  var remoteCallMessage = RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the remote call message: ' + debugDump(payload));
  }

  var debugRepresentation =
      'chrome.usb.' + remoteCallMessage.getDebugRepresentation();
  this.logger.fine('Received a remote call request: ' + debugRepresentation);

  var promiseResolver = goog.Promise.withResolver();

  var chromeUsbFunction = this.getChromeUsbFunction_(
      remoteCallMessage.functionName);
  var chromeUsbFunctionArgs = goog.array.concat(
      remoteCallMessage.functionArguments,
      this.chromeUsbApiGenericCallback_.bind(
          this, debugRepresentation, promiseResolver));

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
      !goog.isFunction(chrome.usb[functionName])) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Unknown chrome.usb API function requested: ' + functionName);
  }
  return chrome.usb[functionName];
};

/**
 * @param {string} debugRepresentation
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {...*} var_args Values passed to the callback by chrome.usb API.
 * @private
 */
ChromeUsbBackend.prototype.chromeUsbApiGenericCallback_ = function(
    debugRepresentation, promiseResolver, var_args) {
  if (goog.isDef(chrome.runtime.lastError)) {
    // FIXME(emaxx): Looks like the USB transfer timeouts also raise this
    // lastError flag, that is not suitable for us as we want to distinguish
    // the timeouts from the fatal errors.
    this.reportRequestError_(
        debugRepresentation,
        promiseResolver,
        goog.isDef(chrome.runtime.lastError['message']) ?
            chrome.runtime.lastError['message'] : 'Unknown error');
  } else {
    this.reportRequestSuccess_(
        debugRepresentation,
        promiseResolver,
        Array.prototype.slice.call(arguments, 2));
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
  this.logger.warning(
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
    this.logger.info('Transfer timed out: ' + debugRepresentation);
  } else {
    this.logger.warning('API error occurred while calling ' +
                        debugRepresentation + ': ' + errorMessage);
  }
  promiseResolver.reject(new Error(errorMessage));
};

/**
 * @param {string} debugRepresentation
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {!Array} resultArgs
 * @private
 */
ChromeUsbBackend.prototype.reportRequestSuccess_ = function(
    debugRepresentation, promiseResolver, resultArgs) {
  this.logger.fine(
      'Results returned by the ' + debugRepresentation + ' call: ' +
      goog.iter.join(goog.iter.map(resultArgs, debugDump), ', '));
  promiseResolver.resolve(resultArgs);
};

});  // goog.scope
