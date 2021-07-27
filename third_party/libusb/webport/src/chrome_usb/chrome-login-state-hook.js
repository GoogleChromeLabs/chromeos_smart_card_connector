/**
 * @license
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
goog.require('goog.Disposable');
goog.require('goog.iter');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * This class implements a hook that hides USB devices when the app is running
 * for a user who has locked their screen. This causes PCSC Lite to release all
 * owned USB devices so that they can be accessed from the sign-in profile app.
 * @extends goog.Disposable
 * @constructor
 */
GSC.Libusb.ChromeLoginStateHook = function() {
  /**
   * @type {boolean}
   * @private
   */
  this.simulateDevicesAbsent_ = false;
  /**
   * @type {!Function}
   * @private
   */
  this.boundOnGotSessionState_ = this.onGotSessionState_.bind(this);
  /**
   * @type {!goog.promise.Resolver}
   * @private
   */
  this.hookIsReadyResolver_ = goog.Promise.withResolver();
  if (chrome.loginState) {
    chrome.loginState.getProfileType(this.onGotProfileType_.bind(this));
  } else {
    goog.log.warning(
        this.logger,
        'chrome.loginState API is not available. This app might require a ' +
            'newer version of Chrome.');
    this.hookIsReadyResolver_.reject();
  }
};

const ChromeLoginStateHook = GSC.Libusb.ChromeLoginStateHook;

goog.inherits(ChromeLoginStateHook, goog.Disposable);

/**
 * @type {!goog.log.Logger}
 * @const
 */
ChromeLoginStateHook.prototype.logger =
    GSC.Logging.getScopedLogger('Libusb.LoginStateHook');

/** @override */
ChromeLoginStateHook.prototype.disposeInternal = function() {
  if (chrome.loginState) {
    chrome.loginState.onSessionStateChanged.removeListener(
        this.boundOnGotSessionState_);
  }

  goog.log.fine(this.logger, 'Disposed');

  ChromeLoginStateHook.base(this, 'disposeInternal');
};

/**
 * @return {function(string, !Array): !Array}
 * @public
 */
ChromeLoginStateHook.prototype.getRequestSuccessHook = function() {
  return this.requestSuccessHook_.bind(this);
};

/**
 * @return {!goog.Promise}
 * @public
 */
ChromeLoginStateHook.prototype.getHookReadyPromise = function() {
  return this.hookIsReadyResolver_.promise;
};

/**
 * @param {!chrome.loginState.ProfileType} profileType
 * @private
 */
ChromeLoginStateHook.prototype.onGotProfileType_ = function(profileType) {
  goog.asserts.assert(chrome.loginState);
  if (profileType === chrome.loginState.ProfileType.USER_PROFILE) {
    chrome.loginState.getSessionState(this.boundOnGotSessionState_);
    chrome.loginState.onSessionStateChanged.addListener(
        this.onSessionStateChanged_.bind(this));
  } else {
    this.hookIsReadyResolver_.resolve();
  }
};

/**
 * @param {!chrome.loginState.SessionState} sessionState
 * @private
 */
ChromeLoginStateHook.prototype.onGotSessionState_ = function(sessionState) {
  goog.asserts.assert(chrome.loginState);
  if (sessionState === chrome.loginState.SessionState.IN_LOCK_SCREEN &&
      !this.simulateDevicesAbsent_) {
    goog.log.info(this.logger, 'No longer showing USB devices.');
    this.simulateDevicesAbsent_ = true;
  } else if (
      sessionState !== chrome.loginState.SessionState.IN_LOCK_SCREEN &&
      this.simulateDevicesAbsent_) {
    goog.log.info(this.logger, 'Showing USB devices.');
    this.simulateDevicesAbsent_ = false;
  }
  // All calls after the first one to resolve() will be ignored.
  this.hookIsReadyResolver_.resolve();
};

/**
 * @private
 */
ChromeLoginStateHook.prototype.onSessionStateChanged_ = function() {
  chrome.loginState.getSessionState(this.boundOnGotSessionState_);
};

/**
 * @param {string} functionName chrome.usb function that was called
 * @param {!Array} callResults
 * @return {!Array} new callResults
 * @private
 */
ChromeLoginStateHook.prototype.requestSuccessHook_ = function(
    functionName, callResults) {
  if (this.simulateDevicesAbsent_ &&
      (functionName === 'getDevices' || functionName === 'getConfigurations')) {
    return [[]];
  }
  return callResults;
};
});  // goog.scope
