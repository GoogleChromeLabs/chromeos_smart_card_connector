/**
 * @license
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

goog.provide('GoogleSmartCard.PcscLiteClient.WasmClientBackend');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteClient.Context');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.Timer');
goog.require('goog.functions');
goog.require('goog.log');
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
 * the server App for some reason cannot be established, but the code in the
 * WebAssembly module keeps constantly sending requests.
 */
const REINITIALIZATION_INTERVAL_SECONDS = 10;

const GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.getScopedLogger('PcscLiteClient.WasmClientBackend');

class BufferedRequest {
  /**
   * @param {!GSC.RemoteCallMessage} remoteCallMessage
   * @param {!goog.promise.Resolver} promiseResolver
   */
  constructor(remoteCallMessage, promiseResolver) {
    /** @const */
    this.remoteCallMessage = remoteCallMessage;
    /** @const */
    this.promiseResolver = promiseResolver;
  }
}

/**
 * This class handles the PC/SC-Lite client API requests (which are normally
 * received from the WebAssembly module) and implements them through the
 * instances of GoogleSmartCard.PcscLiteClient.Context and
 * GoogleSmartCard.PcscLiteClient.API classes.
 */
GSC.PcscLiteClient.WasmClientBackend = class {
  /**
   * @param {!goog.messaging.AbstractChannel} executableModuleMessageChannel
   * @param {string} clientTitle Client title for the connection. Currently this
   * is only used for the debug logs produced by the server app.
   * @param {string=} opt_serverAppId ID of the server App. By default, the ID
   *     of
   * the official server App distributed through WebStore is used.
   */
  constructor(executableModuleMessageChannel, clientTitle, opt_serverAppId) {
    /** @private @const */
    this.clientTitle_ = clientTitle;

    /** @private @const */
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

    /** @type {!goog.structs.Queue.<!BufferedRequest>} @private @const */
    this.bufferedRequestsQueue_ = new goog.structs.Queue();

    /**
     * @type {number|null}
     * @private
     */
    this.initializationTimerId_ = null;

    // Note: the request receiver instance is not stored anywhere, as it makes
    // itself being owned by the message channel.
    new GSC.RequestReceiver(
        REQUESTER_NAME, executableModuleMessageChannel,
        this.handleRequest_.bind(this));

    goog.log.fine(logger, 'Constructed');
  }

  /**
   * @param {!Object} payload
   * @return {!goog.Thenable}
   * @private
   */
  handleRequest_(payload) {
    const remoteCallMessage =
        GSC.RemoteCallMessage.parseRequestPayload(payload);
    if (!remoteCallMessage) {
      GSC.Logging.failWithLogger(
          logger,
          'Failed to parse the remote call message: ' +
              GSC.DebugDump.debugDumpSanitized(payload));
    }

    goog.log.fine(
        logger,
        'Received a remote call request: ' +
            remoteCallMessage.getDebugRepresentation());

    const promiseResolver = goog.Promise.withResolver();

    this.bufferedRequestsQueue_.enqueue(
        new BufferedRequest(remoteCallMessage, promiseResolver));

    // Run the request immediately if the API is already initialized.
    if (this.api_) {
      this.flushBufferedRequestsQueue_();
    } else {
      goog.log.fine(
          logger, 'The request was queued, as API is not initialized yet');
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
      //   means that the context is neither created nor its creation is
      //   scheduled yet.
      //
      // Only the last case is the case when the immediate initialization makes
      // sense.
      if (!this.context_ && this.initializationTimerId_ === null)
        this.initialize_();
    }

    return promiseResolver.promise;
  }

  /** @private */
  initialize_() {
    this.initializationTimerId_ = null;
    if (this.api_) {
      this.api_.dispose();
      this.api_ = null;
    }
    if (this.context_) {
      this.context_.dispose();
      this.context_ = null;
    }

    goog.log.fine(logger, 'Initializing...');

    this.context_ =
        new GSC.PcscLiteClient.Context(this.clientTitle_, this.serverAppId_);
    this.context_.addOnInitializedCallback(
        this.contextInitializedListener_.bind(this));
    this.context_.addOnDisposeCallback(
        this.contextDisposedListener_.bind(this));
    this.context_.initialize();
  }

  /** @private */
  contextInitializedListener_(api) {
    GSC.Logging.checkWithLogger(logger, this.initializationTimerId_ === null);

    GSC.Logging.checkWithLogger(logger, !this.api_);
    this.api_ = api;

    api.addOnDisposeCallback(this.apiDisposedListener_.bind(this));

    this.flushBufferedRequestsQueue_();
  }

  /** @private */
  contextDisposedListener_() {
    goog.log.fine(
        logger,
        'PC/SC-Lite Client Context instance was disposed, cleaning up, ' +
            'rejecting all queued requests and scheduling reinitialization...');
    GSC.Logging.checkWithLogger(logger, this.initializationTimerId_ === null);
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
  }

  /** @private */
  apiDisposedListener_() {
    goog.log.fine(
        logger, 'PC/SC-Lite Client API instance was disposed, cleaning up...');
    this.api_ = null;
    if (this.context_) {
      this.context_.dispose();
      this.context_ = null;
    }
  }

  /** @private */
  scheduleReinitialization_() {
    goog.log.fine(
        logger,
        'Scheduled reinitialization in ' + REINITIALIZATION_INTERVAL_SECONDS +
            ' seconds if new requests will arrive...');
    this.initializationTimerId_ = goog.Timer.callOnce(
        this.reinitializationTimeoutCallback_,
        REINITIALIZATION_INTERVAL_SECONDS * 1000, this);
  }

  /** @private */
  reinitializationTimeoutCallback_() {
    this.initializationTimerId_ = null;
    if (!this.bufferedRequestsQueue_.isEmpty()) {
      this.initialize_();
    } else {
      goog.log.log(
          logger, goog.log.Level.FINER,
          'Reinitialization timeout passed, but not initializing ' +
              'as no new requests arrived');
    }
  }

  /** @private */
  flushBufferedRequestsQueue_() {
    GSC.Logging.checkWithLogger(logger, this.api_);
    if (this.bufferedRequestsQueue_.isEmpty())
      return;
    goog.log.log(
        logger, goog.log.Level.FINER,
        'Starting processing the queued requests...');
    while (!this.bufferedRequestsQueue_.isEmpty()) {
      const request = this.bufferedRequestsQueue_.dequeue();
      this.startRequest_(request.remoteCallMessage, request.promiseResolver);
    }
  }

  /** @private */
  rejectAllBufferedRequests_() {
    if (this.bufferedRequestsQueue_.isEmpty())
      return;
    goog.log.fine(logger, 'Rejecting all queued requests...');
    while (!this.bufferedRequestsQueue_.isEmpty()) {
      const request = this.bufferedRequestsQueue_.dequeue();
      request.promiseResolver.reject(
          new Error('PC/SC-Lite Client API instance was disposed'));
    }
  }

  /**
   * @param {!GSC.RemoteCallMessage} remoteCallMessage
   * @param {!goog.promise.Resolver} promiseResolver
   * @private
   */
  startRequest_(remoteCallMessage, promiseResolver) {
    goog.log.fine(
        logger,
        'Started processing the remote call request: ' +
            remoteCallMessage.getDebugRepresentation());

    GSC.Logging.checkWithLogger(logger, this.api_);

    const method = this.getApiMethod_(remoteCallMessage.functionName);
    const resultPromise =
        method.apply(this.api_, remoteCallMessage.functionArguments);
    resultPromise.then(
        this.apiMethodResolvedCallback_.bind(
            this, remoteCallMessage, promiseResolver),
        this.apiMethodRejectedCallback_.bind(
            this, remoteCallMessage, promiseResolver));
  }

  /**
   * @param {!GSC.RemoteCallMessage} remoteCallMessage
   * @param {!goog.promise.Resolver} promiseResolver
   * @param {!GSC.PcscLiteClient.API.ResultOrErrorCode} apiMethodResult
   * @private
   */
  apiMethodResolvedCallback_(
      remoteCallMessage, promiseResolver, apiMethodResult) {
    goog.log.fine(
        logger,
        'The remote call completed: ' +
            remoteCallMessage.getDebugRepresentation() +
            ' with the result: ' + apiMethodResult.getDebugRepresentation());
    promiseResolver.resolve(apiMethodResult.responseItems);
  }

  /**
   * @param {!GSC.RemoteCallMessage} remoteCallMessage
   * @param {!goog.promise.Resolver} promiseResolver
   * @param {*} apiMethodError
   * @private
   */
  apiMethodRejectedCallback_(
      remoteCallMessage, promiseResolver, apiMethodError) {
    goog.log.fine(
        logger,
        'The remote call failed: ' +
            remoteCallMessage.getDebugRepresentation() +
            ' with the error: ' + apiMethodError);
    promiseResolver.reject(apiMethodError);
  }

  /**
   * @param {string} methodName
   * @return {!Function}
   * @private
   */
  getApiMethod_(methodName) {
    GSC.Logging.checkWithLogger(logger, this.api_);
    if (!goog.object.containsKey(this.api_, methodName) ||
        !goog.functions.isFunction(this.api_[methodName])) {
      GSC.Logging.failWithLogger(
          logger,
          'Unknown PC/SC-Lite Client API method requested: ' + methodName);
    }
    return this.api_[methodName];
  }
};
});  // goog.scope
