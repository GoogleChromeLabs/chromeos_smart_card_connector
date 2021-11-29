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

goog.provide('GoogleSmartCard.LibusbLoginStateHook');

goog.require('GoogleSmartCard.LibusbProxyHook');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.asserts');
goog.require('goog.log');

goog.scope(function() {

const GSC = GoogleSmartCard;

const logger = GSC.Logging.getScopedLogger('LibusbLoginStateHook');

GSC.LibusbLoginStateHook = class extends GSC.LibusbProxyHook {
  constructor() {
    super();
    /** @type {boolean} @private */
    this.simulateDevicesAbsent_ = false;
    /** @type {!Function} @private @const */
    this.boundOnGotSessionState_ = this.onGotSessionState_.bind(this);

    // Note that both fields are initialized to non-nulls synchronously, since
    // the Promise constructor executes the callback before returning.
    /** @type {!Function|null} @private */
    this.resolveInitializationPromise_ = null;
    /** @type {!Promise<void>} @private @const */
    this.initializationPromise_ = new Promise((resolve, reject) => {
      this.resolveInitializationPromise_ = resolve;
    });

    if (chrome && chrome.loginState) {
      chrome.loginState.getProfileType(this.onGotProfileType_.bind(this));
    } else {
      goog.log.warning(
          logger,
          'chrome.loginState API is not available. This app might require a ' +
              'newer version of Chrome.');
      this.resolveInitializationPromise_();
    }
  }

  /** @override */
  async listDevices() {
    await this.initializationPromise_;
    if (this.simulateDevicesAbsent_)
      return [];
    return this.getDelegate().listDevices();
  }

  /** @override */
  async getConfigurations(deviceId) {
    await this.initializationPromise_;
    if (this.simulateDevicesAbsent_)
      return [];
    return this.getDelegate().getConfigurations(deviceId);
  }

  /** @override */
  disposeInternal() {
    if (chrome.loginState) {
      chrome.loginState.onSessionStateChanged.removeListener(
          this.boundOnGotSessionState_);
    }
    goog.log.fine(logger, 'Disposed');
    super.disposeInternal();
  }

  /**
   * @param {!chrome.loginState.ProfileType} profileType
   * @private
   */
  onGotProfileType_(profileType) {
    goog.asserts.assert(chrome.loginState);
    if (profileType === chrome.loginState.ProfileType.USER_PROFILE) {
      chrome.loginState.getSessionState(this.boundOnGotSessionState_);
      chrome.loginState.onSessionStateChanged.addListener(
          this.onSessionStateChanged_.bind(this));
    } else {
      this.resolveInitializationPromise_();
    }
  }

  /**
   * @param {!chrome.loginState.SessionState} sessionState
   * @private
   */
  onGotSessionState_(sessionState) {
    goog.asserts.assert(chrome.loginState);
    if (sessionState === chrome.loginState.SessionState.IN_LOCK_SCREEN &&
        !this.simulateDevicesAbsent_) {
      goog.log.info(logger, 'No longer showing USB devices.');
      this.simulateDevicesAbsent_ = true;
    } else if (
        sessionState !== chrome.loginState.SessionState.IN_LOCK_SCREEN &&
        this.simulateDevicesAbsent_) {
      goog.log.info(logger, 'Showing USB devices.');
      this.simulateDevicesAbsent_ = false;
    }
    // All calls after the first one to resolve() will be ignored.
    this.resolveInitializationPromise_();
  }

  /**
   * @private
   */
  onSessionStateChanged_() {
    chrome.loginState.getSessionState(this.boundOnGotSessionState_);
  }
};
});  // goog.scope
