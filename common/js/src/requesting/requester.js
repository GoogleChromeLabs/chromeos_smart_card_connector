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
 * @fileoverview This file contains definition of the requester, that can be
 * used for sending requests through a message channel and asynchronously
 * waiting of the received responses.
 */

goog.provide('GoogleSmartCard.Requester');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RequesterMessage');
goog.require('GoogleSmartCard.RequesterMessage.RequestMessageData');
goog.require('GoogleSmartCard.RequesterMessage.ResponseMessageData');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.iter');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.promise.Resolver');
goog.require('goog.string');
goog.require('goog.structs.Map');

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
 * This class is a requester, that can be used for sending requests through a
 * message channel and asynchronously waiting of the received responses.
 *
 * The requester has a name that should uniquely represent it, so that the
 * appropriate handler can be triggered on the other end of the message channel,
 * and so that its response is delivered back to this instance.
 * @param {string} name
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @extends {goog.Disposable}
 * @constructor
 */
GSC.Requester = function(name, messageChannel) {
  Requester.base(this, 'constructor');

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('Requester<"' + name + '">');

  /** @private */
  this.name_ = name;

  /** @private */
  this.messageChannel_ = messageChannel;

  /** @private */
  this.requestIdGenerator_ = goog.iter.count();

  /**
   * @type {goog.structs.Map.<number, !goog.promise.Resolver>}
   * @private
   */
  this.requestIdToPromiseResolverMap_ = new goog.structs.Map;

  this.registerResponseMessagesService_();
  this.addChannelDisposedListener_();
};

/** @const */
var Requester = GSC.Requester;

goog.inherits(Requester, goog.Disposable);

/**
 * Starts a request with the specified payload data.
 *
 * Effectively sends the special-structured message through the associated
 * message channel (see the requester-message.js file) and subscribes itself for
 * receiving the corresponding response message.
 *
 * Returns a promise that will be resolved once the request response is received
 * (if the request was successful, then the promise will be fulfilled with its
 * result - otherwise it will be rejected with some error).
 * @param {!Object} payload
 * @return {!goog.Promise}
 */
Requester.prototype.postRequest = function(payload) {
  var requestId = this.requestIdGenerator_.next();

  this.logger.fine(
      'Starting a request with identifier ' + requestId + ', the payload is: ' +
      GSC.DebugDump.debugDump(payload));

  var promiseResolver = goog.Promise.withResolver();

  GSC.Logging.checkWithLogger(
      this.logger, !this.requestIdToPromiseResolverMap_.containsKey(requestId));
  this.requestIdToPromiseResolverMap_.set(requestId, promiseResolver);

  if (this.isDisposed()) {
    // FIXME(emaxx): Probably add the disposal reason information into the
    // message?
    this.rejectRequest_(requestId, 'The requester is already disposed');
    return promiseResolver.promise;
  }

  var requestMessageData = new RequestMessageData(requestId, payload);
  var messageData = requestMessageData.makeMessageData();
  var serviceName = RequesterMessage.getRequestMessageType(this.name_);
  this.messageChannel_.send(serviceName, messageData);

  return promiseResolver.promise;
};

/** @override */
Requester.prototype.disposeInternal = function() {
  var runningRequestIds = this.requestIdToPromiseResolverMap_.getKeys();
  goog.array.sort(runningRequestIds);
  goog.array.forEach(runningRequestIds, function(requestId) {
    // FIXME(emaxx): Probably add the disposal reason information into the
    // message?
    this.rejectRequest_(
        goog.string.parseInt(requestId), 'The requester is disposed');
  }, this);
  this.requestIdToPromiseResolverMap_ = null;

  this.messageChannel_ = null;

  this.logger.fine('Disposed');

  Requester.base(this, 'disposeInternal');
};

/** @private */
Requester.prototype.registerResponseMessagesService_ = function() {
  var serviceName = RequesterMessage.getResponseMessageType(this.name_);
  this.messageChannel_.registerService(
      serviceName, this.responseMessageReceivedListener_.bind(this), true);
};

/** @private */
Requester.prototype.addChannelDisposedListener_ = function() {
  this.messageChannel_.addOnDisposeCallback(
      this.channelDisposedListener_.bind(this));
};

/** @private */
Requester.prototype.channelDisposedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.info('Message channel was disposed, disposing...');
  this.dispose();
};

/**
 * @param {string|!Object} messageData
 * @private
 */
Requester.prototype.responseMessageReceivedListener_ = function(messageData) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(messageData));
  goog.asserts.assertObject(messageData);

  if (this.isDisposed()) {
    // All running requests have already been rejected while disposing.
    return;
  }

  var responseMessageData = ResponseMessageData.parseMessageData(messageData);
  if (goog.isNull(responseMessageData)) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the received response message: ' +
        GSC.DebugDump.debugDump(messageData));
  }
  var requestId = responseMessageData.requestId;

  if (!this.requestIdToPromiseResolverMap_.containsKey(requestId)) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Received a response for unknown request with identifier ' + requestId);
  }

  if (responseMessageData.isSuccessful())
    this.resolveRequest_(requestId, responseMessageData.getPayload());
  else
    this.rejectRequest_(requestId, responseMessageData.getErrorMessage());
};

/**
 * @param {number} requestId
 * @param {*} payload
 * @private
 */
Requester.prototype.resolveRequest_ = function(requestId, payload) {
  this.logger.fine(
      'The request with identifier ' + requestId + ' succeeded with the ' +
      'following result: ' + GSC.DebugDump.debugDump(payload));
  this.popRequestPromiseResolver_(requestId).resolve(payload);
};

/**
 * @param {number} requestId
 * @param {string} errorMessage
 * @private
 */
Requester.prototype.rejectRequest_ = function(requestId, errorMessage) {
  this.logger.fine('The request with identifier ' + requestId + ' failed: ' +
                   errorMessage);
  this.popRequestPromiseResolver_(requestId).reject(new Error(errorMessage));
};

/**
 * @param {number} requestId
 * @return {!goog.promise.Resolver}
 * @private
 */
Requester.prototype.popRequestPromiseResolver_ = function(requestId) {
  var result = this.requestIdToPromiseResolverMap_.get(requestId);
  GSC.Logging.checkWithLogger(this.logger, goog.isDef(result));
  this.requestIdToPromiseResolverMap_.remove(requestId);
  return result;
};

});  // goog.scope
