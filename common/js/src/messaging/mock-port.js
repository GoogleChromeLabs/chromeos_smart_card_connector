/** @license
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
goog.require('goog.testing');

goog.setTestOnly();

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * Mock of the Port class.
 * @param {string=} opt_name
 * @constructor
 * @extends goog.Disposable
 */
GSC.MockPort = function(opt_name) {
  /** @const */
  this.name_ = opt_name;

  /** @type {?} */
  this.postMessage = goog.testing.createFunctionMock('postMessage');

  /** @private */
  this.isConnected_ = true;

  /** @private */
  this.listenerMap_ = new goog.events.ListenerMap(null);

  /** @private */
  this.fakePort_ = this.createFakePort_();
};

/** @const */
var MockPort = GSC.MockPort;

goog.inherits(MockPort, goog.Disposable);

MockPort.prototype.logger = GSC.Logging.getScopedLogger('MockPort');

/** @override */
MockPort.prototype.disposeInternal = function() {
  this.disconnect_();
  delete this.postMessage;
  this.listenerMap_.removeAll();
  delete this.listenerMap_;
  delete this.fakePort_;
  MockPort.base(this, 'disposeInternal');
};

/**
 * @param {*} message
 */
MockPort.prototype.fireOnMessage = function(message) {
  GSC.Logging.checkWithLogger(
      this.logger,
      !this.isDisposed() && this.isConnected_,
      'Trying to fire onMessage for closed mock port');
  for (let listener of this.getListeners_('onMessage'))
    listener(message);
};

/**
 * @return {!Port}
 */
MockPort.prototype.getFakePort = function() {
  return this.fakePort_;
};

/**
 * @return {!Port}
 * @suppress {invalidCasts}
 * @private
 */
MockPort.prototype.createFakePort_ = function() {
  /** @const */
  var self = this;

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
};

/**
 * @private
 */
MockPort.prototype.disconnect_ = function() {
  if (!this.isConnected_)
    return;
  this.isConnected_ = false;
  for (let listener of this.getListeners_('onDisconnect'))
    listener();
};

/**
 * @param {string} type
 * @param {!Function} callback
 * @private
 */
MockPort.prototype.addListener_ = function(type, callback) {
  this.listenerMap_.add(type, callback, false);
};

/**
 * @param {string} type
 * @param {!Function} callback
 * @private
 */
MockPort.prototype.removeListener_ = function(type, callback) {
  this.listenerMap_.remove(type, callback, false);
};

/**
 * @param {string} type
 * @return {!Array.<!Function>}
 * @private
 */
MockPort.prototype.getListeners_ = function(type) {
  var result = [];
  for (let listenerKey of this.listenerMap_.getListeners(type, false)) {
    if (goog.isFunction(listenerKey.listener))
      result.push(listenerKey.listener);
  }
  return result;
};

});  // goog.scope
