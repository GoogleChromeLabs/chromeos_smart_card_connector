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

goog.provide('GoogleSmartCard.Libusb.ChromeUsbRequestHandler');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestHandler');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.iter');
goog.require('goog.log.Logger');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var debugDump = GSC.DebugDump.debugDump;
/** @const */
var RemoteCallMessage = GSC.RemoteCallMessage;

/**
 * @constructor
 * @implements {GSC.RequestHandler}
 */
GSC.Libusb.ChromeUsbRequestHandler = function() {};

/** @const */
var ChromeUsbRequestHandler = GSC.Libusb.ChromeUsbRequestHandler;

/**
 * @type {!goog.log.Logger}
 * @const
 */
ChromeUsbRequestHandler.prototype.logger = GSC.Logging.getScopedLogger(
    'Libusb.ChromeUsbRequestHandler');

/**
 * @override
 * @private
 */
ChromeUsbRequestHandler.prototype.handleRequest = function(payload) {
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

/** @private */
ChromeUsbRequestHandler.prototype.getChromeUsbFunction_ = function(
    functionName) {
  if (!goog.object.containsKey(chrome.usb, functionName) ||
      !goog.isFunction(chrome.usb[functionName])) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Unknown chrome.usb API function requested: ' + functionName);
  }
  return chrome.usb[functionName];
};

/** @private */
ChromeUsbRequestHandler.prototype.chromeUsbApiGenericCallback_ = function(
    debugRepresentation, promiseResolver, var_args) {
  if (goog.isDef(chrome.runtime.lastError)) {
    // FIXME(emaxx): Looks like the USB transfer timeouts also raise this
    // lastError flag, that is not suitable for us as we want to distinguish
    // the timeouts from the fatal errors.
    this.reportRequestError_(
        debugRepresentation,
        promiseResolver,
        chrome.runtime.lastError.message);
  } else {
    this.reportRequestSuccess_(
        debugRepresentation,
        promiseResolver,
        Array.prototype.slice.call(arguments, 2));
  }
};

/** @private */
ChromeUsbRequestHandler.prototype.reportRequestException_ = function(
    debugRepresentation, promiseResolver, exc) {
  this.logger.warning(
      'JavaScript exception was thrown while calling ' + debugRepresentation +
      ': ' + exc.toString());
  promiseResolver.reject(exc);
};

/** @private */
ChromeUsbRequestHandler.prototype.reportRequestError_ = function(
    debugRepresentation, promiseResolver, errorMessage) {
  this.logger.warning('API error occured while calling ' + debugRepresentation +
                      ': ' + errorMessage);
  promiseResolver.reject(new Error(errorMessage));
};

/** @private */
ChromeUsbRequestHandler.prototype.reportRequestSuccess_ = function(
    debugRepresentation, promiseResolver, resultArgs) {
  this.logger.fine(
      'Results returned by the ' + debugRepresentation + ' call: ' +
      goog.iter.join(goog.iter.map(resultArgs, debugDump), ', '));
  promiseResolver.resolve(resultArgs);
};

});  // goog.scope
