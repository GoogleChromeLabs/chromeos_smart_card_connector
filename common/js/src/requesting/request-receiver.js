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
 * @fileoverview This file contains definition of the request receiver - the
 * counterpart of the requester (see the requester.js file).
 */

goog.provide('GoogleSmartCard.RequestReceiver');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RequesterMessage');
goog.require('GoogleSmartCard.RequesterMessage.RequestMessageData');
goog.require('GoogleSmartCard.RequesterMessage.ResponseMessageData');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

const RequesterMessage = GSC.RequesterMessage;
const RequestMessageData = RequesterMessage.RequestMessageData;
const ResponseMessageData = RequesterMessage.ResponseMessageData;

/**
 * This class is a request receiver, that receives and handles request messages
 * received through the specified message channel.
 *
 * The actual request handling is delegated to the specified request handler.
 * The results it returns (either successful or not) are transformed by this
 * class into response messages and sent back through the message channel.
 * @param {string} name Name of the requester whose requests will be handled by
 * this instance.
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {function(!Object):(!goog.Promise|!Promise)} requestHandler
 * @constructor
 */
GSC.RequestReceiver = function(name, messageChannel, requestHandler) {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger(`RequestReceiver<"${name}">`);

  /** @private @const */
  this.name_ = name;

  /** @private @const */
  this.messageChannel_ = messageChannel;

  /** @private @const */
  this.requestHandler_ = requestHandler;

  /**
   * @type {boolean}
   * @private
   */
  this.shouldDisposeOnInvalidMessage_ = false;

  this.registerRequestMessagesService_();
};

const RequestReceiver = GSC.RequestReceiver;

/**
 * Sets whether the message channel should be disposed when an invalid message
 * is received.
 *
 * By default, the message channel is not disposed in case of an invalid
 * message, and a fatal error is raised instead.
 * @param {boolean} shouldDisposeOnInvalidMessage
 */
RequestReceiver.prototype.setShouldDisposeOnInvalidMessage = function(
    shouldDisposeOnInvalidMessage) {
  this.shouldDisposeOnInvalidMessage_ = shouldDisposeOnInvalidMessage;
};

/** @private */
RequestReceiver.prototype.registerRequestMessagesService_ = function() {
  const serviceName = RequesterMessage.getRequestMessageType(this.name_);
  this.messageChannel_.registerService(
      serviceName, this.requestMessageReceivedListener_.bind(this), true);
};

/**
 * @param {string|!Object} messageData
 * @private
 */
RequestReceiver.prototype.requestMessageReceivedListener_ = function(
    messageData) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(messageData));
  goog.asserts.assertObject(messageData);

  const debugDump = GSC.DebugDump.debugDump(messageData);

  const requestMessageData = RequestMessageData.parseMessageData(messageData);
  if (requestMessageData === null) {
    if (this.shouldDisposeOnInvalidMessage_) {
      goog.log.warning(
          this.logger,
          `Failed to parse the received request message: ${debugDump}, ` +
              `disposing of the message channel...`);
      this.messageChannel_.dispose();
      return;
    }
    GSC.Logging.failWithLogger(
        this.logger,
        `Failed to parse the received request message: ${debugDump}`);
  }

  goog.log.fine(
      this.logger,
      `Received a request with requestId ${requestMessageData.requestId}, ` +
          `the payload is: ${debugDump}`);

  const promise = this.requestHandler_(requestMessageData.payload);
  promise.then(
      this.responseResolvedListener_.bind(this, requestMessageData),
      this.responseRejectedListener_.bind(this, requestMessageData));
};

/**
 * @param {!RequestMessageData} requestMessageData
 * @param {*} payload
 * @private
 */
RequestReceiver.prototype.responseResolvedListener_ = function(
    requestMessageData, payload) {
  if (this.messageChannel_.isDisposed()) {
    // Discard the response if it's impossible to send it anymore.
    goog.log.fine(
        this.logger,
        `Cannot send result for requestId ${requestMessageData.requestId} ` +
            `as message channel is shutdown`);
    return;
  }

  const debugDump = GSC.DebugDump.debugDump(payload);
  goog.log.fine(
      this.logger,
      `Sending result for requestId ${requestMessageData.requestId}: ` +
          `${debugDump}`);
  this.sendResponse_(
      new ResponseMessageData(requestMessageData.requestId, payload));
};

/**
 * @param {!RequestMessageData} requestMessageData
 * @param {*} error
 * @private
 */
RequestReceiver.prototype.responseRejectedListener_ = function(
    requestMessageData, error) {
  if (this.messageChannel_.isDisposed()) {
    // Discard the response if it's impossible to send it anymore.
    goog.log.fine(
        this.logger,
        `Cannot send error for requestId ${requestMessageData.requestId} as ` +
            `message channel is shutdown`);
    return;
  }

  const stringifiedError = String(error);
  goog.log.fine(
      this.logger,
      `Sending error for requestId ${requestMessageData.requestId}: ` +
          `${stringifiedError}`);
  this.sendResponse_(new ResponseMessageData(
      requestMessageData.requestId, /*opt_payload=*/ undefined,
      stringifiedError));
};

/**
 * @param {!ResponseMessageData} responseMessageData
 * @private
 */
RequestReceiver.prototype.sendResponse_ = function(responseMessageData) {
  GSC.Logging.checkWithLogger(this.logger, !this.messageChannel_.isDisposed());

  const messageData = responseMessageData.makeMessageData();
  const serviceName = RequesterMessage.getResponseMessageType(this.name_);
  this.messageChannel_.send(serviceName, messageData);
};
});  // goog.scope
