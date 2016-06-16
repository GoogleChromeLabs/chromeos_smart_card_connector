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

/** @const */
var SERVER_ADD_CLIENT_SERVICE_NAME = 'pcsc_lite_add_client';
/** @const */
var SERVER_REMOVE_CLIENT_SERVICE_NAME = 'pcsc_lite_remove_client';

/** @const */
var SERVER_CALL_FUNCTION_REQUESTER_TITLE_FORMAT =
    'pcsc_lite_client_%s_call_function';

/** @const */
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
 * Additionally, this class deals with the lifetimes of the client and the
 * server message channels, delays the execution of the requests until the PC/SC
 * server gets ready, notifies the server about the new client (at the very
 * beginning) and about the client disposal (at the very end).
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

  /** @private */
  this.serverMessageChannel_ = serverMessageChannel;

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

  this.addChannelDisposedListeners_(serverMessageChannel, clientMessageChannel);

  this.logger.fine('Initialized');
};

/** @const */
var ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;

goog.inherits(ClientHandler, goog.Disposable);

/**
 * @type {!goog.iter.Iterator.<number>}
 */
var clientIdGenerator = goog.iter.count();

/**
 * @type {PermissionsChecking.Checker}
 */
var permissionsChecker = null;

/**
 * @param {!RemoteCallMessage} remoteCallMessage
 * @param {!goog.promise.Resolver} promiseResolver
 * @constructor
 */
function BufferedRequest(remoteCallMessage, promiseResolver) {
  this.remoteCallMessage = remoteCallMessage;
  this.promiseResolver = promiseResolver;
}

/** @override */
ClientHandler.prototype.disposeInternal = function() {
  // FIXME(emaxx): Shouldn't we reject all queued requests instead of just
  // dropping them?
  this.bufferedRequestsQueue_.clear();
  this.bufferedRequestsQueue_ = null;

  if (this.serverRequester_) {
    this.serverRequester_ = null;
    this.postRemoveClientServerCommand_();
  }

  this.serverReadinessTracker_ = null;

  this.serverMessageChannel_ = null;

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

  this.logger.finer('Received a remote call request: ' +
                    remoteCallMessage.getDebugRepresentation());

  var promiseResolver = goog.Promise.withResolver();

  this.bufferedRequestsQueue_.enqueue(new BufferedRequest(
      remoteCallMessage, promiseResolver));

  if (!permissionsChecker)
    permissionsChecker = new PermissionsChecking.Checker;
  permissionsChecker.check(this.clientAppId_).then(
      this.clientPermissionGrantedListener_.bind(this),
      this.clientPermissionDeniedListener_.bind(this));

  return promiseResolver.promise;
};

/** @private */
ClientHandler.prototype.addChannelDisposedListeners_ = function(
    serverMessageChannel, clientMessageChannel) {
  serverMessageChannel.addOnDisposeCallback(
      this.serverMessageChannelDisposedListener_.bind(this));
  clientMessageChannel.addOnDisposeCallback(
      this.clientMessageChannelDisposedListener_.bind(this));
};

/** @private */
ClientHandler.prototype.serverMessageChannelDisposedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.info('Server message channel was disposed, disposing...');
  this.dispose();
};

/** @private */
ClientHandler.prototype.clientMessageChannelDisposedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.info('Client message channel was disposed, disposing...');
  this.dispose();
};

/** @private */
ClientHandler.prototype.clientPermissionGrantedListener_ = function() {
  if (this.isDisposed())
    return;
  this.flushRequestsQueueWhenServerReady_();
};

/** @private */
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

/** @private */
ClientHandler.prototype.flushRequestsQueueWhenServerReady_ = function() {
  this.logger.finer('Waiting until the server is ready...');
  this.serverReadinessTracker_.promise.then(
      this.serverReadyListener_.bind(this),
      this.serverReadinessFailedListener_.bind(this));
};

/** @private */
ClientHandler.prototype.serverReadyListener_ = function() {
  this.logger.finer('The server is ready, processing the buffered requests...');
  this.flushbufferedRequestsQueue_();
};

/** @private */
ClientHandler.prototype.serverReadinessFailedListener_ = function() {
  if (this.isDisposed())
    return;
  this.logger.finer('The server failed to become ready, disposing...');
  this.dispose();
};

/** @private */
ClientHandler.prototype.flushbufferedRequestsQueue_ = function() {
  if (this.bufferedRequestsQueue_.isEmpty())
    return;
  this.logger.finer('Starting processing the buffered requests...');
  while (!this.bufferedRequestsQueue_.isEmpty()) {
    var request = this.bufferedRequestsQueue_.dequeue();
    this.startRequest_(request.remoteCallMessage, request.promiseResolver);
  }
};

/**
 * @param {!GSC.RemoteCallMessage} remoteCallMessage
 * @param {!goog.promise.Resolver} promiseResolver
 * @private
 */
ClientHandler.prototype.startRequest_ = function(
    remoteCallMessage, promiseResolver) {
  this.logger.fine('Started processing the remote call request: ' +
                   remoteCallMessage.getDebugRepresentation());

  this.createServerRequesterIfNeed_();

  var requestPromise = this.serverRequester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  requestPromise.then(promiseResolver.resolve, promiseResolver.reject);
};

/** @private */
ClientHandler.prototype.createServerRequesterIfNeed_ = function() {
  if (this.serverRequester_)
    return;

  this.postAddClientServerCommand_();
  GSC.Logging.checkWithLogger(
      this.logger, !goog.isNull(this.serverMessageChannel_));
  goog.asserts.assert(this.serverMessageChannel_);

  var requesterTitle = goog.string.subs(
      SERVER_CALL_FUNCTION_REQUESTER_TITLE_FORMAT, this.clientId_);
  this.serverRequester_ = new GSC.Requester(
      requesterTitle, this.serverMessageChannel_);
};

/** @private */
ClientHandler.prototype.postAddClientServerCommand_ = function() {
  this.serverMessageChannel_.send(
      SERVER_ADD_CLIENT_SERVICE_NAME,
      goog.object.create(CLIENT_ID_MESSAGE_KEY, this.clientId_));
};

/** @private */
ClientHandler.prototype.postRemoveClientServerCommand_ = function() {
  this.serverMessageChannel_.send(
      SERVER_REMOVE_CLIENT_SERVICE_NAME,
      goog.object.create(CLIENT_ID_MESSAGE_KEY, this.clientId_));
};

});  // goog.scope
