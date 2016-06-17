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
 * @fileoverview This file contains definition of the request receiver - the
 * counterpart of the requester (see the requester.js file).
 */

goog.provide('GoogleSmartCard.RequestReceiver');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RequesterMessage');
goog.require('GoogleSmartCard.RequesterMessage.RequestMessageData');
goog.require('GoogleSmartCard.RequesterMessage.ResponseMessageData');
goog.require('GoogleSmartCard.RequestHandler');
goog.require('goog.asserts');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var RequesterMessage = GSC.RequesterMessage;
/** @const */
var RequestMessageData = RequesterMessage.RequestMessageData;
/** @const */
var ResponseMessageData = RequesterMessage.ResponseMessageData;

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
 * @param {!GSC.RequestHandler} requestHandler
 * @constructor
 */
GSC.RequestReceiver = function(name, messageChannel, requestHandler) {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('RequestReceiver<"' + name + '">');

  /** @private */
  this.name_ = name;

  /** @private */
  this.messageChannel_ = messageChannel;

  /** @private */
  this.requestHandler_ = requestHandler;

  this.registerRequestMessagesService_();
};

/** @const */
var RequestReceiver = GSC.RequestReceiver;

/** @private */
RequestReceiver.prototype.registerRequestMessagesService_ = function() {
  var serviceName = RequesterMessage.getRequestMessageType(this.name_);
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

  var requestMessageData = RequestMessageData.parseMessageData(messageData);
  if (goog.isNull(requestMessageData)) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the received request message: ' +
        GSC.DebugDump.debugDump(messageData));
  }

  this.logger.fine(
      'Received a request with identifier ' + requestMessageData.requestId +
      ', the payload is: ' +
      GSC.DebugDump.debugDump(requestMessageData.payload));

  var promise = this.requestHandler_.handleRequest(requestMessageData.payload);
  promise.then(this.responseResolvedListener_.bind(this, requestMessageData),
               this.responseRejectedListener_.bind(this, requestMessageData));
};

/**
 * @param {!RequestMessageData} requestMessageData
 * @param {*} payload
 */
RequestReceiver.prototype.responseResolvedListener_ = function(
    requestMessageData, payload) {
  if (this.messageChannel_.isDisposed()) {
    this.logger.warning(
        'Ignoring the successful response for the request with identifier ' +
        requestMessageData.requestId + ' due to the disposal of the message ' +
        'channel');
    return;
  }

  this.logger.fine(
      'Sending the successful response for the request with identifier ' +
      requestMessageData.requestId + ', the response is: ' +
      GSC.DebugDump.debugDump(payload));
  this.sendResponse_(new ResponseMessageData(
      requestMessageData.requestId, payload));
};

/**
 * @param {!RequestMessageData} requestMessageData
 * @param {*} error
 */
RequestReceiver.prototype.responseRejectedListener_ = function(
    requestMessageData, error) {
  if (this.messageChannel_.isDisposed()) {
    this.logger.warning(
        'Ignoring the failure response for the request with identifier ' +
        requestMessageData.requestId + ' due to the disposal of the message ' +
        'channel');
    return;
  }

  var stringifiedError = error.toString();
  this.logger.fine(
      'Sending the failure response for the request with identifier ' +
      requestMessageData.requestId + ', the error is: ' + stringifiedError);
  this.sendResponse_(new ResponseMessageData(
      requestMessageData.requestId, undefined, stringifiedError));
};

/**
 * @param {!ResponseMessageData} responseMessageData
 * @private
 */
RequestReceiver.prototype.sendResponse_ = function(responseMessageData) {
  GSC.Logging.checkWithLogger(this.logger, !this.messageChannel_.isDisposed());

  var messageData = responseMessageData.makeMessageData();
  var serviceName = RequesterMessage.getResponseMessageType(this.name_);
  this.messageChannel_.send(serviceName, messageData);
};

});  // goog.scope
