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
};

MockPort.prototype.disconnect = function() {
  if (!this.isConnected_)
    return;
  this.isConnected_ = false;
  for (let listenerKey of
       this.listenerMap_.getListeners('onDisconnect', false)) {
    if (goog.isFunction(listenerKey.listener))
      listenerKey.listener(undefined);
  }
};

/**
 * @param {*} message
 */
MockPort.prototype.fireOnMessage = function(message) {
  if (this.isDisposed() || !this.isConnected_) {
    GSC.Logging.failWithLogger(
        this.logger, 'Trying to fire onMessage for closed mock port');
  }
  for (let listenerKey of this.listenerMap_.getListeners('onMessage', false)) {
    if (goog.isFunction(listenerKey.listener))
      listenerKey.listener(message);
  }
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
  this.mockPort_.listenerMap_.add('onDisconnect', callback, false);
};

/**
 * @param {function()} callback
 */
MockPort.OnDisconnectEvent.prototype.removeListener = function(callback) {
  this.mockPort_.listenerMap_.remove('onDisconnect', callback, false);
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
  this.mockPort_.listenerMap_.add('onMessage', callback, false);
};

/**
 * @param {function(*)} callback
 */
MockPort.OnMessageEvent.prototype.removeListener = function(callback) {
  this.mockPort_.listenerMap_.remove('onMessage', callback, false);
};

});  // goog.scope
