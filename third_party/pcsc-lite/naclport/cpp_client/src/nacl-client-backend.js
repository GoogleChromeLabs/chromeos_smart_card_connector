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

goog.require('GoogleSmartCard.PcscLiteClient.NaclClientRequestHandler');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var REQUESTER_NAME = 'pcsc_lite';

/** @const */
var GSC = GoogleSmartCard;

/**
 * This class creates and subscribes a handler of PC/SC-Lite client API requests
 * that are received from the NaCl module.
 *
 * The requests are handled by an owned
 * GoogleSmartCard.PcscLiteClient.NaclClientRequestHandler instance.
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
  this.naclClientRequestHandler_ =
      new GSC.PcscLiteClient.NaclClientRequestHandler(
          clientTitle, opt_serverAppId);

  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      REQUESTER_NAME, naclModuleMessageChannel, this.naclClientRequestHandler_);
};

});  // goog.scope
