/**
 * @license
 * Copyright 2023 Google Inc.
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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.ServerRequestHandler');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.DeferredProcessor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.iter');
goog.require('goog.iter.Iterator');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.string');

goog.scope(function() {

/**
 * Service name that is used for the message to the PC/SC server that a new
 * client handler has to be created.
 *
 * The message data will contain the unique client handler identifier
 * and the client app id (see the CLIENT_HANDLER_ID_MESSAGE_KEY and the
 * CLIENT_NAME_FOR_LOG_MESSAGE_KEY constants).
 */
const CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME =
    'pcsc_lite_create_client_handler';
/**
 * Service name that is used for the message to the PC/SC server that the
 * previously added client handler has to be deleted.
 *
 * The message data will contain the unique client handler identifier (see the
 * CLIENT_HANDLER_ID_MESSAGE_KEY constant).
 */
const DELETE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME =
    'pcsc_lite_delete_client_handler';

/**
 * Template of the service name that is used when sending a message to the PC/SC
 * server containing the PC/SC request from the client.
 *
 * The template pattern is substituted with the client identifier.
 */
const CALL_FUNCTION_SERVER_REQUESTER_TITLE_FORMAT =
    'pcsc_lite_client_handler_%s_call_function';


/**
 * Key under which the unique client handler identifier is stored in the
 * messages sent to the PC/SC server (see also the
 * CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME and the
 * DELETE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME constants).
 */
const CLIENT_HANDLER_ID_MESSAGE_KEY = 'handler_id';

/**
 * Key under which the client logging name is stored in the messages sent to the
 * PC/SC server (see also the
 * CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME constant).
 */
const CLIENT_NAME_FOR_LOG_MESSAGE_KEY = 'client_name_for_log';

const GSC = GoogleSmartCard;
const Pcsc = GoogleSmartCard.PcscLiteServerClientsManagement;
const DeferredProcessor = GSC.DeferredProcessor;
const RemoteCallMessage = GSC.RemoteCallMessage;

/**
 * Generator that is used by the ServerRequestHandler class to generate unique
 * client handler identifiers.
 *
 * This is a singleton object, because the identifiers have to be unique among
 * all client handler instances.
 * @type {!goog.iter.Iterator.<number>}
 */
const idGenerator = goog.iter.count();

/**
 * This class forwards the requests to the PC/SC server (channel to it is
 * accepted as one of the constructor arguments). The result given by PC/SC
 * server is returned as a promise (see the handleRequest method).
 *
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
 * *  Tracks the lifetimes of the server message channel.
 *
 * *  Delays the execution of the client requests until the server gets ready.
 */
Pcsc.ServerRequestHandler = class extends goog.Disposable {
  /**
   * @param {!goog.messaging.AbstractChannel} serverMessageChannel Message
   *     channel
   * to the PC/SC server into which the PC/SC requests will be forwarded.
   * @param {!Pcsc.ReadinessTracker} serverReadinessTracker
   * Tracker of the PC/SC server that allows to delay
   * forwarding of the PC/SC requests to the PC/SC server until it is ready to
   * receive them.
   * @param {string} clientNameForLog
   */
  constructor(serverMessageChannel, serverReadinessTracker, clientNameForLog) {
    super();

    /**
     * @type {string}
     * @private
     */
    this.clientNameForLog_ = clientNameForLog;

    /** @type {number} @const */
    this.id = idGenerator.next();

    const nameForJsLog =
        this.clientNameForLog_ ? this.clientNameForLog_ : 'ourselves';
    /**
     * @type {!goog.log.Logger}
     * @const
     */
    this.logger = GSC.Logging.getScopedLogger(
        `Pcsc.ServerRequestHandler<${nameForJsLog}>, id=${this.id}>`);

    /** @private */
    this.serverMessageChannel_ = serverMessageChannel;

    /** @private */
    this.serverReadinessTracker_ = serverReadinessTracker;

    /**
     * @type {GSC.Requester?}
     * @private
     */
    this.serverRequester_ = null;

    // The requests processing is deferred until the server readiness is
    // established.
    /** @private */
    this.deferredProcessor_ =
        new DeferredProcessor(this.serverReadinessTracker_.promise);

    this.serverMessageChannel_.addOnDisposeCallback(
        () => this.serverMessageChannelDisposedListener_());
  }

  /** @override */
  disposeInternal() {
    super.disposeInternal();


    if (this.serverRequester_) {
      this.serverRequester_.dispose();
      this.serverRequester_ = null;

      // Having the this.serverRequester_ member initialized means that the
      // message to add a new client handler has been sent to the server. This
      // is the right place to send to the server the message to delete the
      // client handler.
      //
      // However, there is an additional condition checked here, which skips
      // sending the message to the server if this.serverMessageChannel_ is
      // null. That's because the reason of client handler disposal may be the
      // disposal of the server message channel, in which case
      // this.serverMessageChannel_ will be cleared before reaching this point
      // (see the serverMessageChannelDisposedListener_ method).
      if (this.serverMessageChannel_)
        this.sendServerDeleteHandlerMessage_();
    }

    this.serverReadinessTracker_ = null;

    // Note that this statement has to be executed after the
    // sendServerDeleteHandlerMessage_ method is called, because the latter
    // accesses this member.
    this.serverMessageChannel_ = null;
  }

  /**
   * Handles a request to PCSC server.
   *
   * The request result will be passed through the returned promise.
   * @param {!RemoteCallMessage} remoteCallMessage The remote call message
   * containing the request contents.
   * @return {!goog.Promise}
   */
  handleRequest(remoteCallMessage) {
    return this.deferredProcessor_.addJob(
        () => this.postRequestToServer_(remoteCallMessage));
  }

  /**
   * Posts the specified client request to the server.
   *
   * The request result will be passed through the returned promise.
   * @param {!RemoteCallMessage} remoteCallMessage The remote call message
   * containing the request contents.
   * @return {!goog.Promise}
   * @private
   */
  postRequestToServer_(remoteCallMessage) {
    const requestDump = remoteCallMessage.getDebugRepresentation();
    goog.log.fine(this.logger, `Processing PC/SC call: ${requestDump}`);

    this.createRequesterIfNeed_();

    return this.serverRequester_
        .postRequest(remoteCallMessage.makeRequestPayload())
        .then(
            function(result) {
              const resultDump = GSC.DebugDump.debugDump(result);
              goog.log.fine(
                  this.logger,
                  `PC/SC call ${requestDump} completed: ${resultDump}`);
              return result;
            },
            function(error) {
              goog.log.log(
                  this.logger,
                  this.isDisposed() ? goog.log.Level.FINE :
                                      goog.log.Level.WARNING,
                  `PC/SC call ${requestDump} failed: ${error}`);
              throw error;
            },
            this);
  }

  /**
   * Creates, if not created before, the requester that will be used to send
   * client requests to the server.
   *
   * If the requester is created, this method also sends to the server a message
   * that a new client handler has to be created.
   * @private
   */
  createRequesterIfNeed_() {
    if (this.serverRequester_)
      return;

    this.sendServerCreateHandlerMessage_();
    GSC.Logging.checkWithLogger(
        this.logger, this.serverMessageChannel_ !== null);
    goog.asserts.assert(this.serverMessageChannel_);

    const requesterTitle =
        goog.string.subs(CALL_FUNCTION_SERVER_REQUESTER_TITLE_FORMAT, this.id);
    this.serverRequester_ =
        new GSC.Requester(requesterTitle, this.serverMessageChannel_);
  }

  /**
   * Sends a message to the server to create a new client handler.
   *
   * Should be called not more than once for a single client handler instance.
   *
   * No client requests should be sent to the server before this message is
   * sent.
   * @private
   */
  sendServerCreateHandlerMessage_() {
    GSC.Logging.checkWithLogger(this.logger, this.serverMessageChannel_);
    GSC.Logging.checkWithLogger(
        this.logger, !this.serverMessageChannel_.isDisposed());
    const messageData = {};
    messageData[CLIENT_HANDLER_ID_MESSAGE_KEY] = this.id;
    messageData[CLIENT_NAME_FOR_LOG_MESSAGE_KEY] = this.clientNameForLog_;
    this.serverMessageChannel_.send(
        CREATE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME, messageData);
  }

  /**
   * Sends a message to the server to delete the client handler.
   *
   * Should be called exactly once, and only if the message to create the new
   * client handler has already been sent.
   *
   * No client requests should be sent to the server after this message is sent.
   * @private
   */
  sendServerDeleteHandlerMessage_() {
    // Note that there are intentionally no checks here whether the self
    // instance is disposed: this method itself is called inside the
    // disposeInternal method.

    GSC.Logging.checkWithLogger(this.logger, this.serverMessageChannel_);
    GSC.Logging.checkWithLogger(
        this.logger, !this.serverMessageChannel_.isDisposed());
    const messageData =
        goog.object.create(CLIENT_HANDLER_ID_MESSAGE_KEY, this.id);
    this.serverMessageChannel_.send(
        DELETE_CLIENT_HANDLER_SERVER_MESSAGE_SERVICE_NAME, messageData);
  }

  /**
   * This listener is called when the server message channel is disposed.
   *
   * Disposes self, as it's not possible to process any client requests after
   * this point.
   * @private
   */
  serverMessageChannelDisposedListener_() {
    if (this.isDisposed())
      return;
    goog.log.warning(
        this.logger, 'Server message channel was disposed, disposing...');

    // Note: this assignment is important because it prevents from sending of
    // any messages through the server message channel, which is normally
    // happening during the disposal process.
    this.serverMessageChannel_ = null;

    this.dispose();
  }
};
});  // goog.scope
