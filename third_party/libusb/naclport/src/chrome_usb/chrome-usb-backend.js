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
goog.require('GoogleSmartCard.Libusb.ChromeUsbRequestHandler');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.array');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var REQUESTER_NAME = 'libusb_chrome_usb';

/** @const */
var GSC = GoogleSmartCard;

/**
 * FIXME(emaxx): Write docs.
 * @param {!goog.messaging.AbstractChannel} naclModuleMessageChannel
 * @constructor
 */
GSC.Libusb.ChromeUsbBackend = function(naclModuleMessageChannel) {
  /** @private */
  this.chromeUsbRequestHandler_ = new GSC.Libusb.ChromeUsbRequestHandler;

  /** @private */
  this.chromeUsbRequestReceiver_ = new GSC.RequestReceiver(
      REQUESTER_NAME, naclModuleMessageChannel, this.chromeUsbRequestHandler_);

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

});  // goog.scope
