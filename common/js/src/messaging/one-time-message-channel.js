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

/** TODO @fileoverview */

goog.provide('GoogleSmartCard.OneTimeMessageChannel');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPinging.PingResponder');
goog.require('GoogleSmartCard.MessageChannelPinging.Pinger');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.asserts');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var Pinger = GSC.MessageChannelPinging.Pinger;
/** @const */
var PingResponder = GSC.MessageChannelPinging.PingResponder;

/** TODO class description
 *
 * @param {string} extensionId
 * @param {function()=} opt_onEstablished
 * @constructor
 * @extends goog.messaging.AbstractChannel
 */
GSC.OneTimeMessageChannel = function(extensionId, opt_onEstablished) {
  OneTimeMessageChannel.base(this, 'constructor');

  /** @type {string} */
  this.extensionId_ = extensionId;

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger(
      'OneTimeMessageChannel<' + extensionId + '>');

  this.registerDefaultService(this.defaultServiceCallback_.bind(this));

  /** @private */
  this.pingResponder_ = new PingResponder(this, this.logger);

  /** @private */
  this.pinger_ = new Pinger(this, this.logger, opt_onEstablished);

  this.logger.fine('Initialized successfully');
};

/** @const */
var OneTimeMessageChannel = GSC.OneTimeMessageChannel;

goog.inherits(OneTimeMessageChannel, goog.messaging.AbstractChannel);

/** @override */
OneTimeMessageChannel.prototype.send = function(serviceName, payload) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(payload));
  goog.asserts.assertObject(payload);

  var typedMessage = new GSC.TypedMessage(serviceName, payload);
  var message = typedMessage.makeMessage();
  this.logger.finest('Posting a message: ' + GSC.DebugDump.debugDump(message));

  if (this.isDisposed()) {
    GSC.Logging.failWithLogger(
        this.logger, 'Failed to post message: the channel is already disposed');
  }

  chrome.runtime.sendMessage(this.extensionId_, message);
};

/** @override */
OneTimeMessageChannel.prototype.disposeInternal = function() {
  this.pinger_.dispose();
  this.pinger_ = null;

  this.pingResponder_ = null;

  this.logger.fine('Disposed');

  OneTimeMessageChannel.base(this, 'disposeInternal');
};

OneTimeMessageChannel.prototype.deliverMessage = function(message) {
  this.logger.finest('Received a message: ' +
                     GSC.DebugDump.debugDump(message));

  var typedMessage = GSC.TypedMessage.parseTypedMessage(message);
  if (!typedMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the received message: ' +
        GSC.DebugDump.debugDump(message));
  }

  this.deliver(typedMessage.type, typedMessage.data);
};

/** @private */
OneTimeMessageChannel.prototype.defaultServiceCallback_ = function(
    serviceName, payload) {
  GSC.Logging.failWithLogger(
      this.logger,
      'Unhandled message received: serviceName="' + serviceName +
      '", payload=' + GSC.DebugDump.debugDump(payload));
};

});  // goog.scope
