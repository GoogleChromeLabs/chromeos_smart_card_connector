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
 * wrapper around the communication mechanisms of the NaCl embeds (see
 * <https://developer.chrome.com/native-client/devguide/coding/message-system>).
 */

goog.provide('GoogleSmartCard.NaclModuleMessageChannel');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.asserts');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * This class is a wrapper around the communication mechanisms of the NaCl
 * embeds (see
 * <https://developer.chrome.com/native-client/devguide/coding/message-system>)
 * that transforms them into Closure-style message channels (see
 * <http://google.github.io/closure-library/api/interface_goog_messaging_MessageChannel.html>).
 * @param {!Element} naclModuleElement
 * @param {!goog.log.Logger} parentLogger
 * @constructor
 * @extends goog.messaging.AbstractChannel
 */
GSC.NaclModuleMessageChannel = function(naclModuleElement, parentLogger) {
  NaclModuleMessageChannel.base(this, 'constructor');

  /** @private */
  this.logger_ = parentLogger;

  /**
   * @type {Element?}
   * @private
   */
  this.naclModuleElement_ = naclModuleElement;

  /** @private */
  this.boundMessageEventListener_ = this.messageEventListener_.bind(this);

  this.naclModuleElement_.addEventListener(
      'message', this.boundMessageEventListener_, true);

  this.registerDefaultService(this.defaultServiceCallback_.bind(this));

  this.logger_.fine('Initialized');
};

const NaclModuleMessageChannel = GSC.NaclModuleMessageChannel;

goog.inherits(NaclModuleMessageChannel, goog.messaging.AbstractChannel);

/** @override */
NaclModuleMessageChannel.prototype.send = function(serviceName, payload) {
  GSC.Logging.checkWithLogger(this.logger_, goog.isObject(payload));
  goog.asserts.assertObject(payload);
  const typedMessage = new GSC.TypedMessage(serviceName, payload);
  const message = typedMessage.makeMessage();
  if (this.isDisposed())
    return;
  this.logger_.finest(
      'Sending message to NaCl module: ' + GSC.DebugDump.debugDump(message));
  this.naclModuleElement_['postMessage'](message);
};

/** @override */
NaclModuleMessageChannel.prototype.disposeInternal = function() {
  this.naclModuleElement_.removeEventListener(
      'message', this.boundMessageEventListener_, true);
  this.boundMessageEventListener_ = null;

  this.naclModuleElement_ = null;

  NaclModuleMessageChannel.base(this, 'disposeInternal');
};

/** @private */
NaclModuleMessageChannel.prototype.messageEventListener_ = function(message) {
  const messageData = message['data'];

  const typedMessage = GSC.TypedMessage.parseTypedMessage(messageData);
  if (!typedMessage) {
    GSC.Logging.failWithLogger(
        this.logger_,
        'Failed to parse message received from NaCl module: ' +
        GSC.DebugDump.debugDump(messageData));
  }

  this.logger_.finest('Received a message from NaCl module: ' +
                      GSC.DebugDump.debugDump(messageData));
  this.deliver(typedMessage.type, typedMessage.data);
};

/** @private */
NaclModuleMessageChannel.prototype.defaultServiceCallback_ = function(
    serviceName, payload) {
  GSC.Logging.failWithLogger(
      this.logger_,
      'Unhandled message received from NaCl module: serviceName="' +
      serviceName + '", payload=' + GSC.DebugDump.debugDump(payload));
};

});  // goog.scope
