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
 * @fileoverview This file contains definition of the requester, that can be
 * used for sending requests through a message channel and asynchronously
 * waiting of the received responses.
 */

goog.provide('GoogleSmartCard.Requester');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PromiseHelpers');
goog.require('GoogleSmartCard.RequesterMessage');
goog.require('GoogleSmartCard.RequesterMessage.RequestMessageData');
goog.require('GoogleSmartCard.RequesterMessage.ResponseMessageData');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.iter');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.promise.Resolver');
goog.require('goog.string');

goog.scope(function() {

const GSC = GoogleSmartCard;

const RequesterMessage = GSC.RequesterMessage;
const RequestMessageData = RequesterMessage.RequestMessageData;
const ResponseMessageData = RequesterMessage.ResponseMessageData;

/**
 * This class is a requester, that can be used for sending requests through a
 * message channel and asynchronously waiting of the received responses.
 *
 * The requester has a name that should uniquely represent it, so that the
 * appropriate handler can be triggered on the other end of the message channel,
 * and so that its response is delivered back to this instance.
 */
GSC.Requester = class extends goog.Disposable {
  /**
   * @param {string} name
   * @param {!goog.messaging.AbstractChannel} messageChannel
   */
  constructor(name, messageChannel) {
    super();

    /**
     * @type {!goog.log.Logger}
     * @const
     */
    this.logger = GSC.Logging.getScopedLogger('Requester<"' + name + '">');

    /** @private @const */
    this.name_ = name;

    /** @private */
    this.messageChannel_ = messageChannel;

    /** @private @const */
    this.requestIdGenerator_ = goog.iter.count();

    /**
     * @type {Map.<number, !goog.promise.Resolver>?}
     * @private
     */
    this.requestIdToPromiseResolverMap_ = new Map();

    this.registerResponseMessagesService_();
    this.addChannelDisposedListener_();
  }

  /**
   * Starts a request with the specified payload data.
   *
   * Effectively sends the special-structured message through the associated
   * message channel (see the requester-message.js file) and subscribes itself
   * for receiving the corresponding response message.
   *
   * Returns a promise that will be resolved once the request response is
   * received (if the request was successful, then the promise will be fulfilled
   * with its result - otherwise it will be rejected with some error).
   * @param {!Object} payload
   * @return {!goog.Thenable}
   */
  postRequest(payload) {
    const requestId = this.requestIdGenerator_.next();

    goog.log.fine(
        this.logger,
        'Starting a request with identifier ' + requestId +
            ', the payload is: ' + GSC.DebugDump.debugDumpSanitized(payload));

    const promiseResolver = goog.Promise.withResolver();
    // Hack to prevent spurious "UnhandledPromiseRejection" errors, which happen
    // when using the native `await` against Closure Library's (now-deprecated)
    // `goog.Promise` if the latter is rejected.
    GSC.PromiseHelpers.suppressUnhandledRejectionError(promiseResolver.promise);

    if (this.isDisposed()) {
      // FIXME(emaxx): Probably add the disposal reason information into the
      // message?
      promiseResolver.reject(new Error('The requester is already disposed'));
      return promiseResolver.promise;
    }

    GSC.Logging.checkWithLogger(
        this.logger, !this.requestIdToPromiseResolverMap_.has(requestId));
    this.requestIdToPromiseResolverMap_.set(requestId, promiseResolver);

    const requestMessageData = new RequestMessageData(requestId, payload);
    const messageData = requestMessageData.makeMessageData();
    const serviceName = RequesterMessage.getRequestMessageType(this.name_);
    /** @preserveTry */
    try {
      this.messageChannel_.send(serviceName, messageData);
    } catch (e) {
      // If sending the message triggered an exception, use it to populate the
      // response. However, this is already done if the channel got immediately
      // disposed of, since `disposeInternal()` cancels all ongoing requests.
      if (!this.isDisposed()) {
        this.rejectRequest_(requestId, e);
      }
    }

    return promiseResolver.promise;
  }

  /** @override */
  disposeInternal() {
    const runningRequestIds =
        Array.from(this.requestIdToPromiseResolverMap_.keys());
    goog.array.sort(runningRequestIds);
    goog.array.forEach(runningRequestIds, (requestId) => {
      // FIXME(emaxx): Probably add the disposal reason information into the
      // message?
      this.rejectRequest_(
          goog.string.parseInt(requestId),
          new Error('The requester is disposed'));
    });
    this.requestIdToPromiseResolverMap_ = null;

    this.messageChannel_ = null;

    goog.log.fine(this.logger, 'Disposed');

    super.disposeInternal();
  }

  /** @private */
  registerResponseMessagesService_() {
    const serviceName = RequesterMessage.getResponseMessageType(this.name_);
    this.messageChannel_.registerService(
        serviceName, this.responseMessageReceivedListener_.bind(this), true);
  }

  /** @private */
  addChannelDisposedListener_() {
    this.messageChannel_.addOnDisposeCallback(
        this.channelDisposedListener_.bind(this));
  }

  /** @private */
  channelDisposedListener_() {
    if (this.isDisposed())
      return;
    goog.log.info(this.logger, 'Message channel was disposed, disposing...');
    this.dispose();
  }

  /**
   * @param {string|!Object} messageData
   * @private
   */
  responseMessageReceivedListener_(messageData) {
    GSC.Logging.checkWithLogger(this.logger, goog.isObject(messageData));
    goog.asserts.assertObject(messageData);

    if (this.isDisposed()) {
      // All running requests have already been rejected while disposing.
      return;
    }

    const responseMessageData =
        ResponseMessageData.parseMessageData(messageData);
    if (responseMessageData === null) {
      GSC.Logging.failWithLogger(
          this.logger,
          'Failed to parse the received response message: ' +
              GSC.DebugDump.debugDumpSanitized(messageData));
    }
    const requestId = responseMessageData.requestId;

    if (!this.requestIdToPromiseResolverMap_.has(requestId)) {
      GSC.Logging.failWithLogger(
          this.logger,
          'Received a response for unknown request with identifier ' +
              requestId);
    }

    if (responseMessageData.isSuccessful()) {
      this.resolveRequest_(requestId, responseMessageData.getPayload());
    } else {
      this.rejectRequest_(
          requestId, new Error(responseMessageData.getErrorMessage()));
    }
  }

  /**
   * @param {number} requestId
   * @param {*} payload
   * @private
   */
  resolveRequest_(requestId, payload) {
    goog.log.fine(
        this.logger,
        'The request with identifier ' + requestId + ' succeeded with the ' +
            'following result: ' + GSC.DebugDump.debugDumpSanitized(payload));
    this.popRequestPromiseResolver_(requestId).resolve(payload);
  }

  /**
   * @param {number} requestId
   * @param {!Error} error
   * @private
   */
  rejectRequest_(requestId, error) {
    goog.log.fine(
        this.logger,
        'The request with identifier ' + requestId + ' failed: ' + error);
    this.popRequestPromiseResolver_(requestId).reject(error);
  }

  /**
   * @param {number} requestId
   * @return {!goog.promise.Resolver}
   * @private
   */
  popRequestPromiseResolver_(requestId) {
    const result = this.requestIdToPromiseResolverMap_.get(requestId);
    GSC.Logging.checkWithLogger(this.logger, result !== undefined);
    this.requestIdToPromiseResolverMap_.delete(requestId);
    return result;
  }
};
});  // goog.scope
