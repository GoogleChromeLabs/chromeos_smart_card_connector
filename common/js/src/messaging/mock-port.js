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
  this.name = opt_name;

  this.onDisconnect = new MockPort.OnDisconnectEvent(this);
  this.onMessage = new MockPort.OnMessageEvent(this);

  /** @type {?} */
  this.postMessage = goog.testing.createFunctionMock('postMessage');

  /** @private */
  this.isConnected_ = true;

  /** @private */
  this.listenerMap_ = new goog.events.ListenerMap(null);
};

/** @const */
var MockPort = GSC.MockPort;

goog.inherits(MockPort, goog.Disposable);

MockPort.prototype.logger = GSC.Logging.getScopedLogger('MockPort');

/** @override */
MockPort.prototype.disposeInternal = function() {
  this.disconnect();
  this.listenerMap_.removeAll();
  this.listenerMap_ = null;
  this.onDisconnect = null;
  this.onMessage = null;
  MockPort.base(this, 'disposeInternal');
};

MockPort.prototype.disconnect = function() {
  if (!this.isConnected_)
    return;
  this.isConnected_ = false;
  for (let listener of this.getListeners_('onDisconnect'))
    listener();
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
 * @suppress {invalidCasts}
 */
MockPort.prototype.getPort = function() {
  // Suppress the type checking in order to make MockPort interchangeable with
  // Port.
  return /** @type {!Port} */ (this);
};

/**
 * @param {!MockPort} mockPort
 * @constructor
 */
MockPort.OnDisconnectEvent = function(mockPort) {
  /** @private */
  this.mockPort_ = mockPort;
};

/**
 * @param {function()} callback
 */
MockPort.OnDisconnectEvent.prototype.addListener = function(callback) {
  this.mockPort_.addListener_('onDisconnect', callback);
};

/**
 * @param {function()} callback
 */
MockPort.OnDisconnectEvent.prototype.removeListener = function(callback) {
  this.mockPort_.removeListener_('onDisconnect', callback);
};

/**
 * @param {!MockPort} mockPort
 * @constructor
 */
MockPort.OnMessageEvent = function(mockPort) {
  /** @private */
  this.mockPort_ = mockPort;
};

/**
 * @param {function(*)} callback
 */
MockPort.OnMessageEvent.prototype.addListener = function(callback) {
  this.mockPort_.addListener_('onMessage', callback);
};

/**
 * @param {function(*)} callback
 */
MockPort.OnMessageEvent.prototype.removeListener = function(callback) {
  this.mockPort_.removeListener_('onMessage', callback);
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
