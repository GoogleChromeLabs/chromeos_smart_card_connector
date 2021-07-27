/** @license
 * Copyright 2016 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

goog.provide('GoogleSmartCard.PcscLiteClient.NaclClientBackend');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteClient.Context');
goog.require('GoogleSmartCard.PcscLiteClient.NaclClientBackend');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Promise');
goog.require('goog.Timer');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');
goog.require('goog.structs.Queue');

goog.scope(function() {

const REQUESTER_NAME = 'pcsc_lite';

/**
 * Time interval between GoogleSmartCard.PcscLiteClient.Context instance
 * reinitialization attempts.
 *
 * The throttling is just for avoiding the system overload when connection to
 * the server App for some reason cannot be established, but the code in NaCl
 * module keeps constantly sending requests.
 */
const REINITIALIZATION_INTERVAL_SECONDS = 10;

const GSC = GoogleSmartCard;

/**
 * This class handles the PC/SC-Lite client API requests (which are normally
 * received from the NaCl module) and implements them through the instances of
 * GoogleSmartCard.PcscLiteClient.Context and GoogleSmartCard.PcscLiteClient.API
 * classes.
 * TODO(#220): Get rid of hardcoded references to NaCl.
 * @param {!goog.messaging.AbstractChannel} naclModuleMessageChannel
 * @param {string} clientTitle Client title for the connection. Currently this
 * is only used for the debug logs produced by the server app.
 * @param {string=} opt_serverAppId ID of the server App. By default, the ID of
 * the official server App distributed through WebStore is used.
 * @constructor
 */
GSC.PcscLiteClient.NaclClientBackend = function(
    naclModuleMessageChannel, clientTitle, opt_serverAppId) {
  /** @private */
  this.clientTitle_ = clientTitle;

  /** @private */
  this.serverAppId_ = opt_serverAppId;

  /**
   * @type {GSC.PcscLiteClient.Context?}
   * @private
   */
  this.context_ = null;

  /**
   * @type {GSC.PcscLiteClient.API?}
   * @private
   */
  this.api_ = null;

  /**
   * @type {!goog.structs.Queue.<!BufferedRequest>}
   * @private
   */
  this.bufferedRequestsQueue_ = new goog.structs.Queue;

  /**
   * @type {number|null}
   * @private
   */
  this.initializationTimerId_ = null;

  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      REQUESTER_NAME, naclModuleMessageChannel, this.handleRequest_.bind(this));

  this.logger.fine('Constructed');
};

const NaclClientBackend = GSC.PcscLiteClient.NaclClientBackend;

/**
 * @type {!goog.log.Logger}
 * @const
 */
NaclClientBackend.prototype.logger = GSC.Logging.getScopedLogger(
    'PcscLiteClient.NaclClientBackend');

/**
 * @param {!Object} payload
 * @return {!goog.Promise}
 * @private
 */
NaclClientBackend.prototype.handleRequest_ = function(payload) {
  const remoteCallMessage = GSC.RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the remote call message: ' +
        GSC.DebugDump.debugDump(payload));
  }

  this.logger.fine('Received a remote call request: ' +
                   remoteCallMessage.getDebugRepresentation());

  const promiseResolver = goog.Promise.withResolver();

  this.bufferedRequestsQueue_.enqueue(new BufferedRequest(
      remoteCallMessage, promiseResolver));

  // Run the request immediately if the API is already initialized.
  if (this.api_) {
    this.flushBufferedRequestsQueue_();
  } else {
    this.logger.fine('The request was queued, as API is not initialized yet');
    // Several cases are possible here:
    // - this.context_ is not null - which means that either it's still
    //   initializing or is disposing right now - in both cases it's necessary
    //   to wait until it goes into some stable state (which is either
    //   initialized or failed);
    // - this.context_ is null, but this.initializationTimerId_ is not null -
    //   which means that initialization is already scheduled in some future,
    //   so, in order to have some throttling of the initialization attempts,
    //   don't run it immediately;
    // - both this.context_ and this.initializationTimerId_ are null - which
    //   means that the context is neither created nor its creation is scheduled
    //   yet.
    //
    // Only the last case is the case when the immediate initialization makes
    // sense.
    if (!this.context_ && this.initializationTimerId_ === null)
      this.initialize_();
  }

  return promiseResolver.promise;
};

/**
 * @param {!GSC.RemoteCallMessage} remoteCallMessage
 * @param {!goog.promise.Resolver} promiseResolver
 * @constructor
 */
function BufferedRequest(remoteCallMessage, promiseResolver) {
  this.remoteCallMessage = remoteCallMessage;
  this.promiseResolver = promiseResolver;
}

/** @private */
NaclClientBackend.prototype.initialize_ = function() {
  this.initializationTimerId_ = null;
  if (this.api_) {
    this.api_.dispose();
    this.api_ = null;
  }
  if (this.context_) {
    this.context_.dispose();
    this.context_ = null;
  }

  this.logger.fine('Initializing...');

  this.context_ = new GSC.PcscLiteClient.Context(
      this.clientTitle_, this.serverAppId_);
  this.context_.addOnInitializedCallback(this.contextInitializedListener_.bind(
      this));
  this.context_.addOnDisposeCallback(this.contextDisposedListener_.bind(this));
  this.context_.initialize();
};

/** @private */
NaclClientBackend.prototype.contextInitializedListener_ = function(api) {
  GSC.Logging.checkWithLogger(
      this.logger, this.initializationTimerId_ === null);

  GSC.Logging.checkWithLogger(this.logger, !this.api_);
  this.api_ = api;

  api.addOnDisposeCallback(this.apiDisposedListener_.bind(this));

  this.flushBufferedRequestsQueue_();
};

/** @private */
NaclClientBackend.prototype.contextDisposedListener_ = function() {
  this.logger.fine(
      'PC/SC-Lite Client Context instance was disposed, cleaning up, ' +
      'rejecting all queued requests and scheduling reinitialization...');
  GSC.Logging.checkWithLogger(
      this.logger, this.initializationTimerId_ === null);
  if (this.api_) {
    this.api_.dispose();
    this.api_ = null;
  }
  if (this.context_) {
    this.context_.dispose();
    this.context_ = null;
  }
  this.rejectAllBufferedRequests_();
  this.scheduleReinitialization_();
};

/** @private */
NaclClientBackend.prototype.apiDisposedListener_ = function() {
  this.logger.fine(
      'PC/SC-Lite Client API instance was disposed, cleaning up...');
  this.api_ = null;
  if (this.context_) {
    this.context_.dispose();
    this.context_ = null;
  }
};

/** @private */
NaclClientBackend.prototype.scheduleReinitialization_ = function() {
  this.logger.fine(
      'Scheduled reinitialization in ' + REINITIALIZATION_INTERVAL_SECONDS +
      ' seconds if new requests will arrive...');
  this.initializationTimerId_ = goog.Timer.callOnce(
      this.reinitializationTimeoutCallback_,
      REINITIALIZATION_INTERVAL_SECONDS * 1000,
      this);
};

/** @private */
NaclClientBackend.prototype.reinitializationTimeoutCallback_ =
    function() {
  this.initializationTimerId_ = null;
  if (!this.bufferedRequestsQueue_.isEmpty()) {
    this.initialize_();
  } else {
    this.logger.finer('Reinitialization timeout passed, but not initializing ' +
                      'as no new requests arrived');
  }
};

/** @private */
NaclClientBackend.prototype.flushBufferedRequestsQueue_ = function() {
  GSC.Logging.checkWithLogger(this.logger, this.api_);
  if (this.bufferedRequestsQueue_.isEmpty())
    return;
  this.logger.finer('Starting processing the queued requests...');
  while (!this.bufferedRequestsQueue_.isEmpty()) {
    const request = this.bufferedRequestsQueue_.dequeue();
    this.startRequest_(request.remoteCallMessage, request.promiseResolver);
  }
};

/** @private */
NaclClientBackend.prototype.rejectAllBufferedRequests_ = function() {
  if (this.bufferedRequestsQueue_.isEmpty())
    return;
  this.logger.fine('Rejecting all queued requests...');
  while (!this.bufferedRequestsQueue_.isEmpty()) {
    const request = this.bufferedRequestsQueue_.dequeue();
    request.promiseResolver.reject(
        new Error('PC/SC-Lite Client API instance was disposed'));
  }
};

/**
 * @param {!GSC.RemoteCallMessage} remoteCallMessage
 * @param {!goog.promise.Resolver} promiseResolver
 * @private
 */
NaclClientBackend.prototype.startRequest_ = function(
    remoteCallMessage, promiseResolver) {
  this.logger.fine('Started processing the remote call request: ' +
                   remoteCallMessage.getDebugRepresentation());

  GSC.Logging.checkWithLogger(this.logger, this.api_);

  const method = this.getApiMethod_(remoteCallMessage.functionName);
  const resultPromise =
      method.apply(this.api_, remoteCallMessage.functionArguments);
  resultPromise.then(
      this.apiMethodResolvedCallback_.bind(
          this, remoteCallMessage, promiseResolver),
      this.apiMethodRejectedCallback_.bind(
          this, remoteCallMessage, promiseResolver));
};

/**
 * @param {!GSC.RemoteCallMessage} remoteCallMessage
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {!GSC.PcscLiteClient.API.ResultOrErrorCode} apiMethodResult
 * @private
 */
NaclClientBackend.prototype.apiMethodResolvedCallback_ = function(
    remoteCallMessage, promiseResolver, apiMethodResult) {
  this.logger.fine(
      'The remote call completed: ' +
      remoteCallMessage.getDebugRepresentation() + ' with the result: ' +
      apiMethodResult.getDebugRepresentation());
  promiseResolver.resolve(apiMethodResult.responseItems);
};

/**
 * @param {!GSC.RemoteCallMessage} remoteCallMessage
 * @param {!goog.promise.Resolver} promiseResolver
 * @param {*} apiMethodError
 * @private
 */
NaclClientBackend.prototype.apiMethodRejectedCallback_ = function(
    remoteCallMessage, promiseResolver, apiMethodError) {
  this.logger.fine(
      'The remote call failed: ' + remoteCallMessage.getDebugRepresentation() +
      ' with the error: ' + apiMethodError);
  promiseResolver.reject(apiMethodError);
};

/**
 * @param {string} methodName
 * @return {!Function}
 * @private
 */
NaclClientBackend.prototype.getApiMethod_ = function(methodName) {
  GSC.Logging.checkWithLogger(this.logger, this.api_);
  if (!goog.object.containsKey(this.api_, methodName) ||
      !goog.isFunction(this.api_[methodName])) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Unknown PC/SC-Lite Client API method requested: ' + methodName);
  }
  return this.api_[methodName];
};

});  // goog.scope
