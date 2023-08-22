/**
 * @license
 * Copyright 2016 Google Inc. All Rights Reserved.
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

goog.provide('GoogleSmartCard.MockPort');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.Disposable');
goog.require('goog.events.ListenerMap');
goog.require('goog.functions');
goog.require('goog.testing');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const logger = GSC.Logging.getScopedLogger('MockPort');

/**
 * Mock of the Port class.
 */
GSC.MockPort = class extends goog.Disposable {
  /**
   * @param {string=} opt_name
   */
  constructor(opt_name) {
    super();

    /** @private @const */
    this.name_ = opt_name;

    /** @type {?} */
    this.postMessage = goog.testing.createFunctionMock('postMessage');

    /** @private */
    this.isConnected_ = true;

    /** @private */
    this.listenerMap_ = new goog.events.ListenerMap(null);

    /** @private */
    this.fakePort_ = this.createFakePort_();
  }

  /** @override */
  disposeInternal() {
    this.disconnect_();
    delete this.postMessage;
    this.listenerMap_.removeAll();
    delete this.listenerMap_;
    delete this.fakePort_;
    super.disposeInternal();
  }

  /**
   * @param {*} message
   */
  fireOnMessage(message) {
    GSC.Logging.checkWithLogger(
        logger, !this.isDisposed() && this.isConnected_,
        'Trying to fire onMessage for closed mock port');
    for (const listener of this.getListeners_('onMessage'))
      listener(message);
  }

  /**
   * @return {!Port}
   */
  getFakePort() {
    return this.fakePort_;
  }

  /**
   * @return {!Port}
   * @suppress {invalidCasts}
   * @private
   */
  createFakePort_() {
    const self = this;

    // Return the value that pretends to be a Port (the type checking was
    // suppressed to allow such, technically invalid, cast).
    return /** @type {!Port} */ ({
      'name': this.name_,
      'onDisconnect': {
        'addListener': function(callback) {
          self.addListener_('onDisconnect', callback);
        },
        'removeListener': function(callback) {
          self.removeListener_('onDisconnect', callback);
        }
      },
      'onMessage': {
        'addListener': function(callback) {
          self.addListener_('onMessage', callback);
        },
        'removeListener': function(callback) {
          self.removeListener_('onMessage', callback);
        }
      },
      'postMessage': this.postMessage,
      'disconnect': this.disconnect_.bind(this)
    });
  }

  /**
   * @private
   */
  disconnect_() {
    if (!this.isConnected_)
      return;
    this.isConnected_ = false;
    for (const listener of this.getListeners_('onDisconnect'))
      listener();
  }

  /**
   * @param {string} type
   * @param {!Function} callback
   * @private
   */
  addListener_(type, callback) {
    this.listenerMap_.add(type, callback, false);
  }

  /**
   * @param {string} type
   * @param {!Function} callback
   * @private
   */
  removeListener_(type, callback) {
    this.listenerMap_.remove(type, callback, false);
  }

  /**
   * @param {string} type
   * @return {!Array.<!Function>}
   * @private
   */
  getListeners_(type) {
    const result = [];
    for (const listenerKey of this.listenerMap_.getListeners(type, false)) {
      if (goog.functions.isFunction(listenerKey.listener))
        result.push(listenerKey.listener);
    }
    return result;
  }
};
});  // goog.scope
