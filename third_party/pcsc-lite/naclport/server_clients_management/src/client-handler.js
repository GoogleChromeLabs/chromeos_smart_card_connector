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
goog.require('GoogleSmartCard.DeferredProcessor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.Checker');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.iter');
goog.require('goog.iter.Iterator');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');

goog.scope(function() {

/**
 * Service name that is used for the message to the PC/SC server that a new
 * client handler has to be created.
 *
 * The message data will contain the unique client handler identifier
 * and the client app id (see the CLIENT_HANDLER_ID_MESSAGE_KEY and the
 * CLIENT_APP_ID_MESSAGE_KEY constants).
 * @const
 */
var CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME =
    'pcsc_lite_create_client_handler';
/**
 * Service name that is used for the message to the PC/SC server that the
 * previously added client handler has to be deleted.
 *
 * The message data will contain the unique client handler identifier (see the
 * CLIENT_HANDLER_ID_MESSAGE_KEY constant).
 * @const
 */
var DELETE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME =
    'pcsc_lite_delete_client_handler';

/**
 * Template of the service name that is used when sending a message to the PC/SC
 * server containing the PC/SC request from the client.
 *
 * The template pattern is substituted with the client identifier.
 * @const
 */
var CALL_FUNCTION_SERVER_REQUESTER_TITLE_FORMAT =
    'pcsc_lite_client_handler_%s_call_function';

/**
 * Key under which the unique client handler identifier is stored in the
 * messages sent to the PC/SC server (see also the
 * CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME and the
 * DELETE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME constants).
 * @const
 */
var CLIENT_HANDLER_ID_MESSAGE_KEY = 'handler_id';
/**
 * Key under which the client app ID is stored in the messages sent to the
 * PC/SC server (see also the
 * CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME constant).
 * @const
 */
var CLIENT_APP_ID_MESSAGE_KEY = 'client_app_id';

/** @const */
var GSC = GoogleSmartCard;
/** @const */
var DeferredProcessor = GSC.DeferredProcessor;
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
 * *  Generates unique client handler identifiers which are used when talking to
 *    the server (for isolating the clients from each other inside the server
 *    correctly).
 *
 * *  Sends a special message to the server to add a new client handler before
 *    sending any PC/SC requests, and send a special message to remove the
 *    client handler during the disposal process.
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
 */
GSC.PcscLiteServerClientsManagement.ClientHandler = function(
    serverMessageChannel,
    serverReadinessTracker,
    clientMessageChannel,
    clientAppId) {
  ClientHandler.base(this, 'constructor');

  /** @type {number} */
  this.id = idGenerator.next();

  /** @private */
  this.clientAppId_ = goog.isDef(clientAppId) ? clientAppId : null;

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger(
      'PcscLiteServerClientsManagement.ClientHandler<' +
      (goog.isNull(this.clientAppId_) ?
           'own app' : '"' + this.clientAppId_ + '"') +
      ', id=' + this.id + '>');

  /** @private */
  this.requestReceiver_ = new GSC.RequestReceiver(
      GSC.PcscLiteCommon.Constants.REQUESTER_TITLE,
      clientMessageChannel,
      this.handleRequest_.bind(this));
  this.requestReceiver_.setShouldDisposeOnInvalidMessage(true);

  /** @private */
  this.serverMessageChannel_ = serverMessageChannel;

  /** @private */
  this.clientMessageChannel_ = clientMessageChannel;

  /** @private */
  this.serverReadinessTracker_ = serverReadinessTracker;

  /**
   * @type {GSC.Requester?}
   * @private
   */
  this.serverRequester_ = null;

  if (!permissionsChecker)
    permissionsChecker = new PermissionsChecking.Checker;

  // The requests processing is deferred until both the server readiness is
  // established and the permissions check is resolved.
  /** @private */
  this.deferredProcessor_ = new DeferredProcessor(goog.Promise.all([
      this.serverReadinessTracker_.promise,
      this.getPermissionsCheckPromise_()]));

  this.addChannelDisposedListeners_();

  this.logger.fine('Initialized');
};

/** @const */
var ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;

goog.inherits(ClientHandler, goog.Disposable);

/**
 * Generator that is used by the ClientHandler class to generate unique client
 * handler identifiers.
 *
 * This is a singleton object, because the identifiers have to be unique among
 * all client handler instances.
 * @type {!goog.iter.Iterator.<number>}
 */
var idGenerator = goog.iter.count();

/**
 * Client permission checker that is used by the
 * GSC.PcscLiteServerClientsManagement.ClientHandler class for verifying that
 * the client is allowed to issue PC/SC requests.
 *
 * This is a lazily initialized singleton object for the optimization purposes
 * (construction of the checker is a relatively expensive operation).
 * @type {PermissionsChecking.Checker?}
 */
var permissionsChecker = null;

/** @override */
ClientHandler.prototype.disposeInternal = function() {
  // TODO(emaxx): It may be helpful to provide the additional information
  // regarding the disposal reason.
  this.deferredProcessor_.dispose();
  this.deferredProcessor_ = null;

  if (this.serverRequester_) {
    this.serverRequester_.dispose();
    this.serverRequester_ = null;

    // Having the this.serverRequester_ member initialized means that the
    // message to add a new client handler has been sent to the server. This is
    // the right place to send to the server the message to delete the client
    // handler.
    //
    // However, there is an additional condition checked here, which skips
    // sending the message to the server if this.serverMessageChannel_ is null.
    // That's because the reason of client handler disposal may be the disposal
    // of the server message channel, in which case this.serverMessageChannel_
    // will be cleared before reaching this point (see the
    // serverMessageChannelDisposedListener_ method).
    if (this.serverMessageChannel_)
      this.sendServerDeleteHandlerMessage_();
  }

  this.serverReadinessTracker_ = null;

  // Note that this statement has to be executed after the
  // sendServerDeleteHandlerMessage_ method is called, because the latter
  // accesses this member.
  this.serverMessageChannel_ = null;

  this.clientMessageChannel_.dispose();
  this.clientMessageChannel_ = null;

  this.requestReceiver_ = null;

  this.logger.fine('Disposed');

  ClientHandler.base(this, 'disposeInternal');
};

/**
 * Handles a request received from PC/SC client.
 * @param {!Object} payload
 * @return {!goog.Promise}
 * @private
 */
ClientHandler.prototype.handleRequest_ = function(payload) {
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

  this.logger.fine('Received a remote call request: ' +
                   remoteCallMessage.getDebugRepresentation());

  return this.deferredProcessor_.addJob(
      this.postRequestToServer_.bind(this, remoteCallMessage));
};

/**
 * Returns a promise that will eventually contain the result of the permissions
 * check for the client.
 * @return {!goog.Promise}
 * @private
 */
ClientHandler.prototype.getPermissionsCheckPromise_ = function() {
  return permissionsChecker.check(this.clientAppId_).then(function() {
    if (!goog.isNull(this.clientAppId_)) {
      this.logger.info(
          'Client was granted permissions to issue PC/SC requests');
    }
  }, function(error) {
    this.logger.warning(
        'Client permission denied. All PC/SC requests will be rejected');
    throw error;
  }, this);
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
  var logMessage = 'Client message channel was disposed, disposing...';
  if (goog.isNull(this.clientAppId_))
    this.logger.fine(logMessage);
  else
    this.logger.info(logMessage);
  this.dispose();
};

/**
 * Posts the specified client request to the server.
 *
 * The request result will be passed through the returned promise.
 * @param {!RemoteCallMessage} remoteCallMessage The remote call message
 * containing the request contents.
 * @return {!goog.Promise}
 * @private
 */
ClientHandler.prototype.postRequestToServer_ = function(remoteCallMessage) {
  this.logger.fine('Started processing the remote call request: ' +
                   remoteCallMessage.getDebugRepresentation());

  this.createServerRequesterIfNeed_();

  return this.serverRequester_.postRequest(
      remoteCallMessage.makeRequestPayload()).then(function(result) {
    this.logger.fine(
        'The remote call request ' +
        remoteCallMessage.getDebugRepresentation() + ' finished successfully' +
        (goog.DEBUG ?
             ' with the following result: ' + GSC.DebugDump.debugDump(result) :
             ''));
    return result;
  }, function(error) {
    this.logger.warning(
        'The remote call request ' +
        remoteCallMessage.getDebugRepresentation() + ' failed with the ' +
        'following error: ' + error);
    throw error;
  }, this);
};

/**
 * Creates, if not created before, the requester that will be used to send
 * client requests to the server.
 *
 * If the requester is created, this method also sends to the server a message
 * that a new client handler has to be created.
 * @private
 */
ClientHandler.prototype.createServerRequesterIfNeed_ = function() {
  if (this.serverRequester_)
    return;

  this.sendServerCreateHandlerMessage_();
  GSC.Logging.checkWithLogger(
      this.logger, !goog.isNull(this.serverMessageChannel_));
  goog.asserts.assert(this.serverMessageChannel_);

  var requesterTitle = goog.string.subs(
      CALL_FUNCTION_SERVER_REQUESTER_TITLE_FORMAT, this.id);
  this.serverRequester_ = new GSC.Requester(
      requesterTitle, this.serverMessageChannel_);
};

/**
 * Sends a message to the server to create a new client handler.
 *
 * Should be called not more than once for a single client handler instance.
 *
 * No client requests should be sent to the server before this message is sent.
 * @private
 */
ClientHandler.prototype.sendServerCreateHandlerMessage_ = function() {
  GSC.Logging.checkWithLogger(this.logger, this.serverMessageChannel_);
  GSC.Logging.checkWithLogger(
      this.logger, !this.serverMessageChannel_.isDisposed());
  var messageData = {};
  messageData[CLIENT_HANDLER_ID_MESSAGE_KEY] = this.id;
  if (!goog.isNull(this.clientAppId_))
    messageData[CLIENT_APP_ID_MESSAGE_KEY] = this.clientAppId_;
  this.serverMessageChannel_.send(
      CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME, messageData);
};

/**
 * Sends a message to the server to delete the client handler.
 *
 * Should be called exactly once, and only if the message to create the new
 * client handler has already been sent.
 *
 * No client requests should be sent to the server after this message is sent.
 * @private
 */
ClientHandler.prototype.sendServerDeleteHandlerMessage_ = function() {
  // Note that there are intentionally no checks here whether the self instance
  // is disposed: this method itself is called inside the disposeInternal
  // method.

  GSC.Logging.checkWithLogger(this.logger, this.serverMessageChannel_);
  GSC.Logging.checkWithLogger(
      this.logger, !this.serverMessageChannel_.isDisposed());
  var messageData = goog.object.create(CLIENT_HANDLER_ID_MESSAGE_KEY, this.id);
  this.serverMessageChannel_.send(
      DELETE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME, messageData);
};

});  // goog.scope
