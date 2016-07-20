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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.Checker');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestHandler');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.iter');
goog.require('goog.iter.Iterator');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');
goog.require('goog.structs.Queue');

goog.scope(function() {

/**
 * Service name that is used when sending a notification message to the PC/SC
 * server that a new PC/SC client is connected and that the requests from this
 * client may follow.
 *
 * The notification message will contain the unique client identifier (see also
 * the CLIENT_ID_MESSAGE_KEY constant).
 * @const
 */
var ADD_CLIENT_SERVER_NOTIFICATION_SERVICE_NAME = 'pcsc_lite_add_client';
/**
 * Service name that is used when sending a notification message to the PC/SC
 * server that the previously added PC/SC client should be removed (no requests
 * from this client are allowed to be sent after this notification).
 *
 * The notification message will contain the unique client identifier (see also
 * the CLIENT_ID_MESSAGE_KEY constant).
 * @const
 */
var REMOVE_CLIENT_SERVER_NOTIFICATION_SERVICE_NAME = 'pcsc_lite_remove_client';

/**
 * Template of the service name that is used when sending a message to the PC/SC
 * server containing the PC/SC request from the client.
 *
 * The template pattern is substituted with the client identifier.
 * @const
 */
var CALL_FUNCTION_SERVER_REQUESTER_TITLE_FORMAT =
    'pcsc_lite_client_%s_call_function';

/**
 * Key under which the unique client identifier is stored in the notifications
 * sent to the PC/SC server regarding adding of a new client or removal of the
 * previously added client (see also the SERVER_ADD_CLIENT_SERVICE_NAME and the
 * SERVER_REMOVE_CLIENT_SERVICE_NAME constants).
 * @const
 */
var CLIENT_ID_MESSAGE_KEY = 'client_id';

/** @const */
var GSC = GoogleSmartCard;
/** @const */
var PcscLiteServerClientsManagement = GSC.PcscLiteServerClientsManagement;
/** @const */
var RemoteCallMessage = GSC.RemoteCallMessage;
/** @const */
var PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;

/**
 * This class implements handling of requests received from the PC/SC client.
 *
 * The main tasks solved by this class are:
 *
 * 1. Verifying the client permissions.
 *
 *    Each client has to have a special permission in order to be able to
 *    execute requests.
 *
 *    The permissions checking is performed using the
 *    GSC.PcscLiteServerClientsManagement.PermissionsChecking.Checker class.
 *
 *    During the permissions check (which may be a long lasting operation - e.g.
 *    showing a user prompt), all incoming requests are held in an internal
 *    buffer.
 *
 *    If the permissions check finishes successfully, then all buffered requests
 *    (and all requests that may come later) are being executed (see item #2).
 *
 *    If the permissions check fails, then all buffered requests (and all
 *    requests that may come later) immediately get the error response.
 *
 * 2. Performing of the PC/SC requests received from the client.
 *
 *    After ensuring that the client has the required permission (see item #1),
 *    the PC/SC requests from the client are being actually executed.
 *
 *    This is performed by forwarding the requests to the PC/SC server (channel
 *    to it is accepted as one of the constructor arguments).
 *
 *    The responses from the PC/SC server, once they are received, are forwarded
 *    then back to the client.
 *
 *    In other words, this instance behaves as a "pipe connector" between the
 *    client message channel and the server message channel.
 *
 * Additionally, this class:
 *
 * *  Generates unique client identifiers which are used when talking to the
 *    server (for isolating the clients from each other inside the server
 *    correctly).
 *
 * *  Notifies the server about the new client (at the very beginning) and about
 *    the client disposal (at the very end).
 *
 * *  Tracks the lifetimes of the client and the server message channels.
 *
 * *  Delays the execution of the client requests until the server gets ready.
 *
 * *  Destroys the client message channel when the client handler is destroyed
 *    (which may happen due to various reasons, including the reasons making
 *    handling of the client requests impossible).
 * @param {!goog.messaging.AbstractChannel} serverMessageChannel Message channel
 * to the PC/SC server into which the PC/SC requests will be forwarded.
 * @param {!GSC.PcscLiteServerClientsManagement.ReadinessTracker}
 * serverReadinessTracker Tracker of the PC/SC server that allows to delay
 * forwarding of the PC/SC requests to the PC/SC server until it is ready to
 * receive them.
 * @param {!goog.messaging.AbstractChannel} clientMessageChannel Message channel
 * to the client.
 * @param {string=} clientAppId Identifier of the client App (the undefined
 * value means that the own App is talking to itself).
 * @constructor
 * @extends goog.Disposable
 * @implements {GSC.RequestHandler}
 */
GSC.PcscLiteServerClientsManagement.ClientHandler = function(
    serverMessageChannel,
    serverReadinessTracker,
    clientMessageChannel,
    clientAppId) {
  ClientHandler.base(this, 'constructor');

  this.clientId_ = clientIdGenerator.next();

  /** @private */
  this.clientAppId_ = goog.isDef(clientAppId) ? clientAppId : null;

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger(
      'PcscLiteServerClientsManagement.ClientHandler<id=' + this.clientId_ +
      (goog.isNull(this.clientAppId_) ?
           '' : ', app_id="' + this.clientAppId_ + '"') + '>');

  /** @private */
  this.requestReceiver_ = new GSC.RequestReceiver(
      GSC.PcscLiteCommon.Constants.REQUESTER_TITLE,
      clientMessageChannel,
      this);
  this.requestReceiver_.setShouldDisposeOnInvalidMessage(true);

  /** @private */
  this.serverMessageChannel_ = serverMessageChannel;

  /** @private */
  this.clientMessageChannel_ = clientMessageChannel;

  /** @private */
  this.serverReadinessTracker_ = serverReadinessTracker;

  /**
   * @type {GSC.Requester}
   * @private
   */
  this.serverRequester_ = null;

  /**
   * @type {goog.structs.Queue.<!BufferedRequest>}
   * @private
   */
  this.bufferedRequestsQueue_ = new goog.structs.Queue;

  this.addChannelDisposedListeners_();

  this.logger.fine('Initialized');
};

/** @const */
var ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;

goog.inherits(ClientHandler, goog.Disposable);

/**
 * Generator that is used by the
 * GSC.PcscLiteServerClientsManagement.ClientHandler class to generate unique
 * client identifiers.
 *
 * This is a singleton object, because each client handler should obtain a
 * unique client identifier.
 * @type {!goog.iter.Iterator.<number>}
 */
var clientIdGenerator = goog.iter.count();

/**
 * Client permission checker that is used by the
 * GSC.PcscLiteServerClientsManagement.ClientHandler class for verifying that
 * the client is allowed to issue PC/SC requests.
 *
 * This is a lazily initialized singleton object for the optimization purposes
 * (construction of the checker is a relatively expensive operation).
 * @type {PermissionsChecking.Checker}
 */
var permissionsChecker = null;

/**
 * This structure is used to store the received client requests in a buffer.
 * @param {!RemoteCallMessage} remoteCallMessage The remote call message
 * containing the request contents.
 * @param {!goog.promise.Resolver} promiseResolver The promise (with methods to
 * resolve it) which should be used for sending the request response.
 * @constructor
 */
function BufferedRequest(remoteCallMessage, promiseResolver) {
  this.remoteCallMessage = remoteCallMessage;
  this.promiseResolver = promiseResolver;
}

/** @override */
ClientHandler.prototype.disposeInternal = function() {
  while (!this.bufferedRequestsQueue_.isEmpty()) {
    var request = this.bufferedRequestsQueue_.dequeue();
    // TODO(emaxx): It may be helpful to provide the additional information
    // regarding the disposal reason.
    request.promiseResolver.reject(new Error(
        'The client handler was disposed'));
  }
  this.bufferedRequestsQueue_ = null;

  if (this.serverRequester_) {
    this.serverRequester_.dispose();
    this.serverRequester_ = null;

    // Having the this.serverRequester_ member initialized means that the
    // notification about the new client has been sent to the server. This is
    // the right place to send to the server the notification that the client
    // should be removed.
    //
    // However, there is an additional condition checked here, which skips
    // sending the message to the server if this.serverMessageChannel_ is null.
    // That's because the reason of client handler disposal may be the disposal
    // of the server message channel, in which case this.serverMessageChannel_
    // will be cleared before reaching this point (see the
    // serverMessageChannelDisposedListener_ method).
    if (this.serverMessageChannel_)
      this.sendRemoveClientServerNotification_();
  }

  this.serverReadinessTracker_ = null;

  // Note that this statement has to be executed after the
  // sendRemoveClientServerNotification_ method is called, because the latter
  // accesses this member.
  this.serverMessageChannel_ = null;

  this.clientMessageChannel_.dispose();
  this.clientMessageChannel_ = null;

  this.requestReceiver_ = null;

  this.logger.fine('Disposed');

  ClientHandler.base(this, 'disposeInternal');
};

/**
 * @override
 * @private
 */
ClientHandler.prototype.handleRequest = function(payload) {
  if (this.isDisposed()) {
    return goog.Promise.reject(new Error(
        'The client handler is already disposed'));
  }

  var remoteCallMessage = RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    this.logger.warning('Failed to parse the received request payload: ' +
                        GSC.DebugDump.debugDump(payload));
    return goog.Promise.reject(new Error(
        'Failed to parse the received request payload'));
  }

  this.logger.info('Received a remote call request: ' +
                   remoteCallMessage.getDebugRepresentation());

  var promiseResolver = goog.Promise.withResolver();

  // For the sake of simplicity, all requests are emplaced in the internal
  // buffer, regardless whether the permissions check have already been
  // performed (see also the comment below).
  //
  // In particular, this guarantees that the order of the requests is preserved
  // when forwarding them to the server for being executed.
  this.bufferedRequestsQueue_.enqueue(new BufferedRequest(
      remoteCallMessage, promiseResolver));

  if (!permissionsChecker)
    permissionsChecker = new PermissionsChecking.Checker;
  // For the sake of simplicity, the result of the permissions check is not
  // cached anywhere in this instance. It's the job of the permissions checker
  // to do the appropriate caching internally.
  //
  // So it may happen that the permissions check even finishes synchronously
  // during the following call, but this is not a problem.
  //
  // Also the implementation here deals with the potential situations when
  // multiple permission check requests are resolved in the order different from
  // the how they were started.
  permissionsChecker.check(this.clientAppId_).then(
      this.clientPermissionGrantedListener_.bind(this),
      this.clientPermissionDeniedListener_.bind(this));

  return promiseResolver.promise;
};

/**
 * Sets up listeners tracking the lifetime of the server and client message
 * channels.
 * @private
 */
ClientHandler.prototype.addChannelDisposedListeners_ = function() {
  this.serverMessageChannel_.addOnDisposeCallback(
      this.serverMessageChannelDisposedListener_.bind(this));
  this.clientMessageChannel_.addOnDisposeCallback(
      this.clientMessageChannelDisposedListener_.bind(this));
};

/**
 * This listener is called when the server message channel is disposed.
 *
 * Disposes self, as it's not possible to process any client requests after this
 * point.
 * @private
 */
ClientHandler.prototype.serverMessageChannelDisposedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.warning('Server message channel was disposed, disposing...');

  // Note: this assignment is important because it prevents from sending of any
  // messages through the server message channel, which is normally happening
  // during the disposal process.
  this.serverMessageChannel_ = null;

  this.dispose();
};

/**
 * This listener is called when the client message channel is disposed.
 *
 * Disposes self, as no communication to the client is possible after this
 * point.
 * @private
 */
ClientHandler.prototype.clientMessageChannelDisposedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.info('Client message channel was disposed, disposing...');
  this.dispose();
};

/**
 * This callback is called when the client permissions checker finished
 * returning that the client has the necessary permission.
 *
 * Schedules the execution of all previously buffered client requests after the
 * server readiness tracker resolves successfully.
 * @private
 */
ClientHandler.prototype.clientPermissionGrantedListener_ = function() {
  if (this.isDisposed())
    return;
  this.flushRequestsQueueWhenServerReady_();
};

/**
 * This callback is called when the client permissions checker finished
 * returning that the client doesn't have the necessary permission.
 *
 * Resolves all previously buffered client requests with an error.
 * @private
 */
ClientHandler.prototype.clientPermissionDeniedListener_ = function() {
  if (this.isDisposed())
    return;

  // TODO(emaxx): Add timeout before rejecting, so that inaccurately written
  // client won't DOS the server App?

  while (!this.bufferedRequestsQueue_.isEmpty()) {
    var request = this.bufferedRequestsQueue_.dequeue();

    this.logger.warning(
        'Client permission was denied, failing the request: ' +
        request.remoteCallMessage.getDebugRepresentation());

    request.promiseResolver.reject(new Error(
        'The client permission was denied'));
  }
};

/**
 * Schedules the execution of all previously buffered client requests after the
 * server readiness tracker resolves successfully.
 *
 * If the readiness tracker resolves with an error, then disposes self.
 * @private
 */
ClientHandler.prototype.flushRequestsQueueWhenServerReady_ = function() {
  this.logger.finer('Waiting until the server is ready...');
  this.serverReadinessTracker_.promise.then(
      this.serverReadyListener_.bind(this),
      this.serverReadinessFailedListener_.bind(this));
};

/**
 * This callback is called when the server readiness tracker reports that the
 * server got ready.
 *
 * Starts execution of all previously buffered client requests.
 * @private
 */
ClientHandler.prototype.serverReadyListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.finer('The server is ready, processing the buffered requests...');
  this.flushBufferedRequestsQueue_();
};

/**
 * This callback is called when the server readiness tracker reports a failure.
 *
 * Disposes self, as the server is unavailable starting from this point.
 * @private
 */
ClientHandler.prototype.serverReadinessFailedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.warning('The server failed to become ready, disposing...');
  this.dispose();
};

/**
 * Executes all previously buffered client requests.
 * @private
 */
ClientHandler.prototype.flushBufferedRequestsQueue_ = function() {
  if (this.bufferedRequestsQueue_.isEmpty())
    return;
  this.logger.finer('Starting processing the buffered requests...');
  while (!this.bufferedRequestsQueue_.isEmpty()) {
    var request = this.bufferedRequestsQueue_.dequeue();
    this.startRequest_(request.remoteCallMessage, request.promiseResolver);
  }
};

/**
 * Starts the specified client request by forwarding it to the server.
 *
 * The request result, once received, will be used to resolve the specified
 * promise.
 * @param {!RemoteCallMessage} remoteCallMessage The remote call message
 * containing the request contents.
 * @param {!goog.promise.Resolver} promiseResolver The promise (with methods to
 * resolve it) which should be used for sending the request response.
 * @private
 */
ClientHandler.prototype.startRequest_ = function(
    remoteCallMessage, promiseResolver) {
  this.logger.fine('Started processing the remote call request: ' +
                   remoteCallMessage.getDebugRepresentation());

  this.createServerRequesterIfNeed_();

  var requestPromise = this.serverRequester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  requestPromise.then(
      this.requestSucceededListener_.bind(
          this, remoteCallMessage, promiseResolver),
      this.requestFailedListener_.bind(
          this, remoteCallMessage, promiseResolver));
};

/**
 * This callback is called when the request to the server succeeds.
 *
 * Resolves the passed promise with the passed result, which essentially results
 * in sending the result back to the client.
 * @param {!RemoteCallMessage} remoteCallMessage The remote call message
 * containing the request contents.
 * @param {!goog.promise.Resolver} promiseResolver The promise (with methods to
 * resolve it) which should be used for sending the request response.
 * @param {*} result The request result.
 * @private
 */
ClientHandler.prototype.requestSucceededListener_ = function(
    remoteCallMessage, promiseResolver, result) {
  this.logger.info(
      'The remote call request ' + remoteCallMessage.getDebugRepresentation() +
      ' finished successfully' +
      (goog.DEBUG ?
           ' with the following result: ' + GSC.DebugDump.debugDump(result) :
           ''));

  promiseResolver.resolve(result);
};

/**
 * This callback is called when the request to the server fails.
 *
 * Resolves the passed promise with the passed error, which essentially results
 * in sending the error back to the client.
 * @param {!RemoteCallMessage} remoteCallMessage The remote call message
 * containing the request contents.
 * @param {!goog.promise.Resolver} promiseResolver The promise (with methods to
 * resolve it) which should be used for sending the request response.
 * @param {*} error The error with which the request finished.
 * @private
 */
ClientHandler.prototype.requestFailedListener_ = function(
    remoteCallMessage, promiseResolver, error) {
  this.logger.warning(
      'The remote call request ' + remoteCallMessage.getDebugRepresentation() +
      ' failed with the following error: ' + error);

  promiseResolver.reject(error);
};

/**
 * Creates, if not created before, the requester that will be used to send
 * client requests to the server.
 *
 * Together with creating the requester, also sends the notification to the
 * server that the new client should be added.
 * @private
 */
ClientHandler.prototype.createServerRequesterIfNeed_ = function() {
  if (this.serverRequester_)
    return;

  this.sendAddClientServerNotification_();
  GSC.Logging.checkWithLogger(
      this.logger, !goog.isNull(this.serverMessageChannel_));
  goog.asserts.assert(this.serverMessageChannel_);

  var requesterTitle = goog.string.subs(
      CALL_FUNCTION_SERVER_REQUESTER_TITLE_FORMAT, this.clientId_);
  this.serverRequester_ = new GSC.Requester(
      requesterTitle, this.serverMessageChannel_);
};

/**
 * Sends the notification to the server that the new client should be added.
 *
 * Should be called not more than once.
 *
 * No client requests should be sent to the server before sending this
 * notification.
 * @private
 */
ClientHandler.prototype.sendAddClientServerNotification_ = function() {
  GSC.Logging.checkWithLogger(this.logger, this.serverMessageChannel_);
  GSC.Logging.checkWithLogger(
      this.logger, !this.serverMessageChannel_.isDisposed());
  this.serverMessageChannel_.send(
      ADD_CLIENT_SERVER_NOTIFICATION_SERVICE_NAME,
      goog.object.create(CLIENT_ID_MESSAGE_KEY, this.clientId_));
};

/**
 * Sends the notification to the server that the client should be removed.
 *
 * Should be called exactly once, and only if the notification that the client
 * has to be added was sent previously.
 *
 * No client requests should be sent to the server after sending this
 * notification.
 * @private
 */
ClientHandler.prototype.sendRemoveClientServerNotification_ = function() {
  // Note that there is intentionally no check whether the self instance is
  // disposed: this method itself is called inside the disposeInternal method.

  GSC.Logging.checkWithLogger(this.logger, this.serverMessageChannel_);
  GSC.Logging.checkWithLogger(
      this.logger, !this.serverMessageChannel_.isDisposed());
  this.serverMessageChannel_.send(
      REMOVE_CLIENT_SERVER_NOTIFICATION_SERVICE_NAME,
      goog.object.create(CLIENT_ID_MESSAGE_KEY, this.clientId_));
};

});  // goog.scope
