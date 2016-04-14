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

goog.require('GoogleSmartCard.Libusb.ChromeUsbRequestHandler');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.messaging.MessageChannel');

goog.scope(function() {

/** @const */
var REQUESTER_NAME = 'libusb_chrome_usb';

/** @const */
var GSC = GoogleSmartCard;

/**
 * @param {!goog.messaging.MessageChannel} naclModuleMessageChannel
 * @constructor
 */
GSC.Libusb.ChromeUsbBackend = function(naclModuleMessageChannel) {
  /** @private */
  this.chromeUsbRequestHandler_ = new GSC.Libusb.ChromeUsbRequestHandler;

  /** @private */
  this.chromeUsbRequestReceiver_ = new GSC.RequestReceiver(
      REQUESTER_NAME, naclModuleMessageChannel, this.chromeUsbRequestHandler_);
};

});  // goog.scope
