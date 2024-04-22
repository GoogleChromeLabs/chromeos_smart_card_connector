/**
 * @license
 * Copyright 2024 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview A helper for waiting on an incoming message port connection.
 */

goog.provide('GoogleSmartCard.PortMessageChannelWaiter');

goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('goog.Disposable');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Provides a promise to the `PortMessageChannel` object, resolved when the
 * `chrome.runtime.onConnect` event with the specified name happens.
 */
GSC.PortMessageChannelWaiter = class extends goog.Disposable {
  /**
   * @param {string} awaitedPortName
   */
  constructor(awaitedPortName) {
    super();

    /** @const @private */
    this.awaitedPortName_ = awaitedPortName;

    /** @type {?function(!GSC.PortMessageChannel)} @private */
    this.resolve_ = null;
    /** @type {?function(!Error)} @private */
    this.reject_ = null;
    /** @type {!Promise<!GSC.PortMessageChannel>} @private */
    this.promise_ = new Promise((resolve, reject) => {
      this.resolve_ = resolve;
      this.reject_ = reject;
    });

    // Subscribe to the event. Store the bound method in a variable so that we
    // can remove the listener when needed.
    /** @type {?function(!Port)} @private */
    this.listener_ = (port) => this.onConnect_(port);
    chrome.runtime.onConnect.addListener(this.listener_);
  }

  /**
   * @return {!Promise<!GSC.PortMessageChannel>}
   */
  getPromise() {
    return this.promise_;
  }

  /** @override */
  disposeInternal() {
    this.stopListening_();

    // This rejects the promise unless it's already been resolved.
    this.reject_(new Error('Disposed'));

    super.disposeInternal();
  }

  /**
   * @param {!Port} port
   * @private
   */
  onConnect_(port) {
    if (this.isDisposed())
      return;
    if (port.name != this.awaitedPortName_) {
      // Not the port we're waiting for.
      return;
    }
    this.stopListening_();
    const portChannel = new GSC.PortMessageChannel(port);
    this.resolve_(portChannel);
  }

  /** @private */
  stopListening_() {
    if (!this.listener_) {
      // Already stopped.
      return;
    }
    chrome.runtime.onConnect.removeListener(this.listener_);
    this.listener_ = null;
  }
};
});
