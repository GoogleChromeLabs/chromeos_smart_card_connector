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

/**
 * @fileoverview This file contains the Closure-style message channel (see
 * <http://google.github.io/closure-library/api/interface_goog_messaging_MessageChannel.html>)
 * wrapper around the Chrome long-lived message connections (see
 * <https://developer.chrome.com/extensions/messaging#connect>).
 */

goog.provide('GoogleSmartCard.PortMessageChannel');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPinging.PingResponder');
goog.require('GoogleSmartCard.MessageChannelPinging.Pinger');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.asserts');
goog.require('goog.events');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var Pinger = GSC.MessageChannelPinging.Pinger;
/** @const */
var PingResponder = GSC.MessageChannelPinging.PingResponder;

/**
 * This class is a wrapper around the Chrome long-lived message connections (see
 * <https://developer.chrome.com/extensions/messaging#connect>) that transforms
 * them into Closure-style message channels (see
 * <http://google.github.io/closure-library/api/interface_goog_messaging_MessageChannel.html>).
 *
 * Apart from adapting the Port communication mechanisms into the methods of the
 * goog.messaging.AbstractChannel class, this class enables pinging over this
 * message channel (see the message-channel-pinging.js file for details).
 * @param {!Port} port
 * @param {function()=} opt_onEstablished
 * @constructor
 * @extends goog.messaging.AbstractChannel
 */
GSC.PortMessageChannel = function(port, opt_onEstablished) {
  PortMessageChannel.base(this, 'constructor');

  /**
   * @type {Port}
   * @private
   */
  this.port_ = port;

  /** @type {string|null} */
  this.extensionId = this.getPortExtensionId_(port);

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger(
      'PortMessageChannel<"' + port.name + '"' +
      (goog.isNull(this.extensionId) ? '' : ', id="' + this.extensionId + '"') +
      '>');

  /** @private */
  this.boundDisconnectEventHandler_ = this.disconnectEventHandler_.bind(this);
  this.port_.onDisconnect.addListener(this.boundDisconnectEventHandler_);

  /** @private */
  this.boundMessageEventHandler_ = this.messageEventHandler_.bind(this);
  this.port_.onMessage.addListener(this.boundMessageEventHandler_);

  this.registerDefaultService(this.defaultServiceCallback_.bind(this));

  /** @private */
  this.pingResponder_ = new PingResponder(this, this.logger);

  /** @private */
  this.pinger_ = new Pinger(this, this.logger, opt_onEstablished);

  this.logger.fine('Initialized successfully');
};

/** @const */
var PortMessageChannel = GSC.PortMessageChannel;

goog.inherits(PortMessageChannel, goog.messaging.AbstractChannel);

/** @override */
PortMessageChannel.prototype.send = function(serviceName, payload) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(payload));
  goog.asserts.assertObject(payload);

  var typedMessage = new GSC.TypedMessage(serviceName, payload);
  var message = typedMessage.makeMessage();
  this.logger.finest('Posting a message: ' + GSC.DebugDump.debugDump(message));

  if (this.isDisposed()) {
    GSC.Logging.failWithLogger(
        this.logger, 'Failed to post message: the channel is already disposed');
  }

  /** @preserveTry */
  try {
    this.port_.postMessage(message);
  } catch (exc) {
    this.dispose();
    GSC.Logging.failWithLogger(this.logger, 'Failed to post message: ' + exc);
  }
};

/** @override */
PortMessageChannel.prototype.disposeInternal = function() {
  this.pinger_.dispose();
  this.pinger_ = null;

  this.pingResponder_ = null;

  this.port_.onMessage.removeListener(this.boundMessageEventHandler_);
  this.boundMessageEventHandler_ = null;

  this.port_.onDisconnect.removeListener(this.boundDisconnectEventHandler_);
  this.boundDisconnectEventHandler_ = null;

  this.port_.disconnect();
  this.port_ = null;

  this.logger.fine('Disposed');

  PortMessageChannel.base(this, 'disposeInternal');
};

/**
 * @param {!Port} port
 * @return {string|null}
 * @private
 */
PortMessageChannel.prototype.getPortExtensionId_ = function(port) {
  if (!goog.object.containsKey(port, 'sender'))
    return null;
  var sender = port['sender'];
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(sender));
  if (!goog.object.containsKey(sender, 'id'))
    return null;
  var senderId = sender['id'];
  GSC.Logging.checkWithLogger(this.logger, goog.isString(senderId));
  return senderId;
};

/** @private */
PortMessageChannel.prototype.disconnectEventHandler_ = function() {
  this.logger.fine('Port was disconnected, disposing...');
  this.dispose();
};

/**
 * @param {*} message
 * @private
 */
PortMessageChannel.prototype.messageEventHandler_ = function(message) {
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

/**
 * @param {string} serviceName
 * @param {!Object|string} payload
 * @private
 */
PortMessageChannel.prototype.defaultServiceCallback_ = function(
    serviceName, payload) {
  GSC.Logging.failWithLogger(
      this.logger,
      'Unhandled message received: serviceName="' + serviceName +
      '", payload=' + GSC.DebugDump.debugDump(payload));
};

});  // goog.scope
