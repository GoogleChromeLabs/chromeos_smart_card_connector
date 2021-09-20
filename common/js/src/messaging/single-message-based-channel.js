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

/**
 * @fileoverview This file contains the Closure-style message channel (see
 * <http://google.github.io/closure-library/api/interface_goog_messaging_MessageChannel.html>)
 * wrapper around the Chrome simple one-time requests (see
 * <https://developer.chrome.com/extensions/messaging#simple>).
 */

goog.provide('GoogleSmartCard.SingleMessageBasedChannel');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPinging.PingResponder');
goog.require('GoogleSmartCard.MessageChannelPinging.Pinger');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.asserts');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

const Pinger = GSC.MessageChannelPinging.Pinger;
const PingResponder = GSC.MessageChannelPinging.PingResponder;

/**
 * This class is a wrapper around the Chrome simple one-time requests (see
 * <https://developer.chrome.com/extensions/messaging#simple>) that transforms
 * them into Closure-style message channels (see
 * <http://google.github.io/closure-library/api/interface_goog_messaging_MessageChannel.html>).
 *
 * Apart from adapting the one-time communication mechanisms into the methods of
 * the goog.messaging.AbstractChannel class, this class enables pinging over
 * this message channel (see the message-channel-pinging.js file for details).
 * @param {string} extensionId
 * @param {function()=} opt_onEstablished
 * @param {boolean=} opt_shouldPingOnPing Whether an additional ping message
 * should be sent each time a ping message is received.
 * @constructor
 * @extends goog.messaging.AbstractChannel
 */
GSC.SingleMessageBasedChannel = function(
    extensionId, opt_onEstablished, opt_shouldPingOnPing) {
  SingleMessageBasedChannel.base(this, 'constructor');

  /** @const @type {string} */
  this.extensionId = extensionId;

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger(
      'SingleMessageBasedChannel<' + extensionId + '>');

  /** @private @const */
  this.shouldPingOnPing_ = !!opt_shouldPingOnPing;

  this.registerDefaultService(this.defaultServiceCallback_.bind(this));

  /** @private */
  this.pinger_ = new Pinger(this, this.logger, opt_onEstablished);

  /** @private */
  this.pingResponder_ = new PingResponder(
      this, this.logger, this.pingMessageReceivedListener_.bind(this));

  goog.log.fine(this.logger, 'Initialized successfully');
};

const SingleMessageBasedChannel = GSC.SingleMessageBasedChannel;

goog.inherits(SingleMessageBasedChannel, goog.messaging.AbstractChannel);

/**
 * @override
 * @param {string} serviceName
 * @param {!Object|string} payload
 */
SingleMessageBasedChannel.prototype.send = function(serviceName, payload) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(payload));
  goog.asserts.assertObject(payload);

  const normalizedPayload =
      GSC.ContainerHelpers.substituteArrayBuffersRecursively(payload);

  const typedMessage = new GSC.TypedMessage(serviceName, normalizedPayload);
  const message = typedMessage.makeMessage();
  goog.log.log(
      this.logger, goog.log.Level.FINEST,
      'Posting a message: ' + GSC.DebugDump.debugDump(message));

  if (this.isDisposed()) {
    GSC.Logging.failWithLogger(
        this.logger, 'Failed to post message: the channel is already disposed');
  }

  chrome.runtime.sendMessage(this.extensionId, message);
};

/**
 * This method passes the message to the corresponding registered service.
 * @param {*} message
 */
SingleMessageBasedChannel.prototype.deliverMessage = function(message) {
  goog.log.log(
      this.logger, goog.log.Level.FINEST,
      'Received a message: ' + GSC.DebugDump.debugDump(message));

  const typedMessage = GSC.TypedMessage.parseTypedMessage(message);
  if (!typedMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the received message: ' +
            GSC.DebugDump.debugDump(message));
  }

  this.deliver(typedMessage.type, typedMessage.data);
};

/**
 * @override
 */
SingleMessageBasedChannel.prototype.disposeInternal = function() {
  this.pinger_.dispose();
  this.pinger_ = null;

  this.pingResponder_.dispose();
  this.pingResponder_ = null;

  goog.log.fine(this.logger, 'Disposed');

  SingleMessageBasedChannel.base(this, 'disposeInternal');
};

/**
 * @param {string} serviceName
 * @param {!Object|string} payload
 * @private
 */
SingleMessageBasedChannel.prototype.defaultServiceCallback_ = function(
    serviceName, payload) {
  GSC.Logging.failWithLogger(
      this.logger,
      'Unhandled message received: serviceName="' + serviceName +
          '", payload=' + GSC.DebugDump.debugDump(payload));
};

/** @private */
SingleMessageBasedChannel.prototype.pingMessageReceivedListener_ = function() {
  if (this.isDisposed())
    return;
  if (this.shouldPingOnPing_)
    this.pinger_.postPingMessage();
};
});  // goog.scope
