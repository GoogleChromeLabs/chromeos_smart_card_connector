/** @license
 * Copyright 2019 Google Inc.
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

goog.provide('GoogleSmartCard.Libusb.ChromeLoginStateHook');

goog.require('GoogleSmartCard.Libusb.ChromeUsbBackend');
goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.array');
goog.require('goog.iter');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * This class implements a hook that hides USB devices when the app is running
 * for a user who has locked their screen. This causes PCSC Lite to release all
 * owned USB devices so that they can be accessed from the sign-in profile app.
 * @constructor
 */
GSC.Libusb.ChromeLoginStateHook = function() {
  this.simulateDevicesAbsent_ = false;
  if (chrome.loginState) {
    chrome.loginState.getProfileType(this.onGotProfileType_.bind(this));
  }
};

/** @const */
var ChromeLoginStateHook = GSC.Libusb.ChromeLoginStateHook;

/**
 * @type {!goog.log.Logger}
 * @const
 */
ChromeLoginStateHook.prototype.logger = GSC.Logging.getScopedLogger(
    'Libusb.LoginStateHook');

/**
 * @return {!Function}
 * @public
 */ 
ChromeLoginStateHook.prototype.getRequestSuccessHook = function() {
    return this.requestSuccessHook_.bind(this);
};

 /**
 * @param {!chrome.loginState.ProfileType} profileType
 * @private
 */
ChromeLoginStateHook.prototype.onGotProfileType_ = function(profileType) {
  goog.asserts.assert(chrome.loginState);
  if (profileType === chrome.loginState.ProfileType.USER_PROFILE) {
    chrome.loginState.getSessionState(
        this.onGetSessionState_.bind(this));
    chrome.loginState.onSessionStateChanged.addListener(
        this.onGetSessionState_.bind(this));
  }
};

/**
 * @param {!chrome.loginState.SessionState} sessionState
 * @private
 */
ChromeLoginStateHook.prototype.onGetSessionState_ = function(
    sessionState) {
  goog.asserts.assert(chrome.loginState);
  if (sessionState === chrome.loginState.SessionState.IN_LOCK_SCREEN) {
    this.logger.info("No longer showing USB devices.");
    this.simulateDevicesAbsent_ = true;
  } else {
    this.logger.info("Showing USB devices.");
    this.simulateDevicesAbsent_ = false;
  }
};

/**
 * @param {string} functionName chrome.usb function that was called
 * @param {!Array} callResults
 * @private
 */
ChromeLoginStateHook.prototype.requestSuccessHook_ = function(functionName, 
                                                              callResults) {
  if (this.simulateDevicesAbsent_ &&
      (functionName === 'getDevices' ||
       functionName === 'getConfigurations')) {
    callResults.length = 0;
  }
};

});  // goog.scope
