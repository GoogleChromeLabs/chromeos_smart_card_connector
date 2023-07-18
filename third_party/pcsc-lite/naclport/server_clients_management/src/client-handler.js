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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.DeferredProcessor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Pcsc.PermissionsChecker');
goog.require('GoogleSmartCard.Pcsc.PolicyOrPromptingPermissionsChecker');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ServerRequestHandler');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.string');

goog.scope(function() {

const GSC = GoogleSmartCard;
const DeferredProcessor = GSC.DeferredProcessor;
const RemoteCallMessage = GSC.RemoteCallMessage;
const ServerRequestHandler =
    GSC.PcscLiteServerClientsManagement.ServerRequestHandler;

/**
 * Returns a string that should be used for logs regarding the given client
 * emitted by the executable module. The intention is to use something shorter
 * than the origin but still preserving the most important information for logs.
 * Returns an empty string if the client is ourselves.
 * @param {string|undefined} clientOrigin
 * @return {string}
 */
function getClientNameForLog(clientOrigin) {
  if (clientOrigin === undefined) {
    // By convention, use an empty string to denote our application talking to
    // itself. This convention is used by the executable module in order to set
    // up the logging severity: in Release builds we don't want to spam the logs
    // with requests generated by our own calls to ourselves.
    return '';
  }
  const url = new URL(clientOrigin);
  if (url.protocol === GSC.MessagingOrigin.EXTENSION_PROTOCOL) {
    // The client is a Chrome extension/app. Use just the extension ID for
    // logging purposes.
    const extensionId = url.hostname;
    return extensionId;
  }
  // The client is a website. Use its host (which, unlike "hostname", includes
  // the port number) for logging purposes.
  return url.host;
}

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
 *    The permission checking is performed using the
 *    GSC.Pcsc.PolicyOrPromptingPermissionsChecker class.
 *
 *    During the permission check (which may be a long lasting operation - e.g.
 *    showing a user prompt), all incoming requests are held in an internal
 *    buffer.
 *
 *    If the permission check finishes successfully, then all buffered requests
 *    (and all requests that may come later) are being executed (see item #2).
 *
 *    If the permission check fails, then all buffered requests (and all
 *    requests that may come later) immediately get the error response.
 *
 * 2. Performing of the PC/SC requests received from the client.
 *
 *    After ensuring that the client has the required permission (see item #1),
 *    the PC/SC requests from the client are being actually executed.
 *
 *    This is performed by forwarding the requests to the ServerRequestHandler,
 *    which forwards them to PC/SC server (channel to it is accepted as one of
 *    the constructor arguments).
 *
 *    The responses from the PC/SC server, once they are received, are forwarded
 *    then back to the client.
 *
 *    In other words, this instance behaves as a "pipe connector" between the
 *    client message channel and the server message channel.
 *
 * Additionally, this class:
 * *  Tracks the lifetimes of the client and the server message channels.
 *
 * *  Destroys the client message channel when the client handler is destroyed
 *    (which may happen due to various reasons, including the reasons making
 *    handling of the client requests impossible).
 * @param {!goog.messaging.AbstractChannel} serverMessageChannel Message channel
 * to the PC/SC server into which the PC/SC requests will be forwarded.
 * @param {!goog.Promise} serverReadinessTracker Tracker of the PC/SC server
 * that allows to delay forwarding of the PC/SC requests to the PC/SC server
 * until it is ready to receive them.
 * @param {!goog.messaging.AbstractChannel} clientMessageChannel Message channel
 * to the client.
 * @param {string=} clientOrigin Origin of the client that's sending commands
 * to us (the undefined value means that our own extension/app is talking to
 * itself; otherwise, an example origin is "https://example.org" or
 * "chrome-extension://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").
 * @constructor
 * @extends goog.Disposable
 */
GSC.PcscLiteServerClientsManagement.ClientHandler = function(
    serverMessageChannel, serverReadinessTracker, clientMessageChannel,
    clientOrigin) {
  ClientHandler.base(this, 'constructor');

  /** @private @const */
  this.clientOrigin_ = clientOrigin;

  this.serverRequester_ = new ServerRequestHandler(
      serverMessageChannel, serverReadinessTracker,
      getClientNameForLog(this.clientOrigin_));

  this.id = this.serverRequester_.id;

  const nameForJsLog = this.clientOrigin_ === undefined ?
      'ourselves' :
      getClientNameForLog(this.clientOrigin_);
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger(
      `Pcsc.ClientHandler<${nameForJsLog}, id=${this.id}>`);

  /** @private */
  this.requestReceiver_ = new GSC.RequestReceiver(
      GSC.PcscLiteCommon.Constants.REQUESTER_TITLE, clientMessageChannel,
      this.handleRequest_.bind(this));
  this.requestReceiver_.setShouldDisposeOnInvalidMessage(true);

  /** @private */
  this.clientMessageChannel_ = clientMessageChannel;

  // The requests processing is deferred until the permission check is resolved.
  /** @private */
  this.deferredProcessor_ =
      new DeferredProcessor(this.getPermissionsCheckPromise_());

  this.addChannelDisposedListeners_();

  goog.log.fine(this.logger, 'Initialized');
};

const ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;

goog.inherits(ClientHandler, goog.Disposable);

/**
 * Client permission checker that is used by the
 * GSC.PcscLiteServerClientsManagement.ClientHandler class for verifying that
 * the client is allowed to issue PC/SC requests.
 *
 * This is a lazily initialized singleton object for the optimization purposes
 * (construction of the checker is a relatively expensive operation).
 * @type {GSC.Pcsc.PolicyOrPromptingPermissionsChecker?}
 */
let defaultPermissionsChecker = null;
/**
 * @type {GSC.Pcsc.PermissionsChecker?}
 */
let permissionsCheckerOverrideForTesting = null;

/**
 * @return {!GSC.Pcsc.PermissionsChecker}
 */
function getPermissionsChecker() {
  if (permissionsCheckerOverrideForTesting)
    return permissionsCheckerOverrideForTesting;
  if (!defaultPermissionsChecker) {
    defaultPermissionsChecker =
        new GSC.Pcsc.PolicyOrPromptingPermissionsChecker;
  }
  return defaultPermissionsChecker;
}

/**
 * @param {GSC.Pcsc.PermissionsChecker?} override
 */
ClientHandler.overridePermissionsCheckerForTesting = function(override) {
  permissionsCheckerOverrideForTesting = override;
};

/** @override */
ClientHandler.prototype.disposeInternal = function() {
  // TODO(emaxx): It may be helpful to provide the additional information
  // regarding the disposal reason.
  this.deferredProcessor_.dispose();
  this.deferredProcessor_ = null;

  this.serverRequester_.dispose();
  this.serverRequester_ = null;

  this.clientMessageChannel_.dispose();
  this.clientMessageChannel_ = null;

  this.requestReceiver_ = null;

  goog.log.fine(this.logger, 'Disposed');

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
    return goog.Promise.reject(
        new Error('The client handler is already disposed'));
  }

  const remoteCallMessage = RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    goog.log.warning(
        this.logger,
        'Failed to parse the received request payload: ' +
            GSC.DebugDump.debugDump(payload));
    return goog.Promise.reject(
        new Error('Failed to parse the received request payload'));
  }

  goog.log.fine(
      this.logger,
      'Received a remote call request: ' +
          remoteCallMessage.getDebugRepresentation());

  return this.deferredProcessor_.addJob(
      this.serverRequester_.handleRequest.bind(
          this.serverRequester_, remoteCallMessage));
};

/**
 * Returns a promise that will eventually contain the result of the permissions
 * check for the client.
 * @return {!goog.Promise}
 * @private
 */
ClientHandler.prototype.getPermissionsCheckPromise_ = function() {
  return getPermissionsChecker()
      .check(this.clientOrigin_ === undefined ? null : this.clientOrigin_)
      .then(
          function() {
            if (this.clientOrigin_ !== undefined) {
              goog.log.info(
                  this.logger,
                  'Client was granted permissions to issue PC/SC requests');
            }
          },
          function(error) {
            goog.log.warning(
                this.logger,
                'Client permission denied. All PC/SC requests will be rejected');
            throw error;
          },
          this);
};

/**
 * Sets up listeners tracking the lifetime of the server and client message
 * channels.
 * @private
 */
ClientHandler.prototype.addChannelDisposedListeners_ = function() {
  this.serverRequester_.addOnDisposeCallback(
      () => this.serverRequesterDisposedListener_());
  this.clientMessageChannel_.addOnDisposeCallback(
      () => this.clientMessageChannelDisposedListener_());
};

/**
 * This listener is called when the server message channel is disposed.
 *
 * Disposes self, as it's not possible to process any client requests after this
 * point.
 * @private
 */
ClientHandler.prototype.serverRequesterDisposedListener_ = function() {
  if (this.isDisposed())
    return;

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
  const logMessage = 'Client message channel was disposed, disposing...';
  if (this.clientOrigin_ === undefined)
    goog.log.fine(this.logger, logMessage);
  else
    goog.log.info(this.logger, logMessage);
  this.dispose();
};
});  // goog.scope
