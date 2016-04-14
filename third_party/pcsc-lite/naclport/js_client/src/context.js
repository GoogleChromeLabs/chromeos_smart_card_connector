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

goog.provide('GoogleSmartCard.PcscLiteClient.Context');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.Disposable');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * Context for using the PC/SC-Lite client API.
 *
 * On initialization (see the initialize method), initiates a connection to the
 * the server App. When the connection gets successfully established, creates
 * the GoogleSmartCard.PcscLiteClient.API object and fires the listeners added
 * through the onInitialized method. If the connection is established
 * unsuccessfully or was dropped later, the GoogleSmartCard.PcscLiteClient.API
 * object is destroyed and the listeners added through the onDisposed method are
 * fired.
 *
 * @param {string} clientTitle Client title for the connection. Currently this
 * is only used for the debug logs produced by the server app.
 * @param {string|null=} opt_serverAppId ID of the server App. By default, the
 * ID of the official server App distributed through WebStore is used. The null
 * value means trying to connect to the own App.
 * @constructor
 * @extends {goog.Disposable}
 */
GSC.PcscLiteClient.Context = function(clientTitle, opt_serverAppId) {
  Context.base(this, 'constructor');

  /**
   * @type {GSC.PcscLiteClient.API}
   */
  this.api = null;

  /** @const */
  this.clientTitle = clientTitle;

  /**
   * @type {string|undefined}
   * @private
   */
  this.serverAppId_ = undefined;
  if (!goog.isDef(opt_serverAppId))
    this.serverAppId_ = GSC.PcscLiteCommon.Constants.SERVER_OFFICIAL_APP_ID;
  else if (!goog.isNull(opt_serverAppId))
    this.serverAppId_ = opt_serverAppId;

  /**
   * @type {GSC.PortMessageChannel}
   * @private
   */
  this.channel_ = null;

  /**
   * @type {Array.<function()|function(!GSC.PcscLiteClient.API)>}
   * @private
   */
  this.onInitializedCallbacks_ = [];

  this.logger.fine('Constructed');
};

/** @const */
var Context = GSC.PcscLiteClient.Context;

goog.inherits(Context, goog.Disposable);

goog.exportSymbol('GoogleSmartCard.PcscLiteClient.Context', Context);

/**
 * @type {!goog.log.Logger}
 * @const
 */
Context.prototype.logger = GSC.Logging.getScopedLogger(
    'PcscLiteClient.Context');

goog.exportProperty(Context.prototype, 'logger', Context.prototype.logger);

/**
 * Starts the initialization process by initiating a connection to the server
 * App.
 */
Context.prototype.initialize = function() {
  this.logger.fine(
      'Opening a connection to the server app ' +
      (goog.isDef(this.serverAppId_) ?
           '(extension id is "' + this.serverAppId_ + '")' :
           '(which is the own app)') +
      '...');
  var connectInfo = {'name': this.clientTitle};
  if (goog.isDef(this.serverAppId_)) {
    var port = chrome.runtime.connect(this.serverAppId_, connectInfo);
  } else {
    var port = chrome.runtime.connect(connectInfo);
  }

  this.channel_ = new GSC.PortMessageChannel(
      port, this.messageChannelEstablishedListener_.bind(this));
  this.channel_.addOnDisposeCallback(
      this.messageChannelDisposedListener_.bind(this));
};

goog.exportProperty(
    Context.prototype, 'initialize', Context.prototype.initialize);

/**
 * Adds a callback that will be called once the initialization succeeds.
 *
 * The API object (see the getApi method) will be created by the moment of the
 * callback execution. The API object will be additionally passed to the
 * callback as the only parameter.
 * @param {function()|function(!GSC.PcscLiteClient.API)} callback
 */
Context.prototype.addOnInitializedCallback = function(callback) {
  if (!goog.isNull(this.api))
    callback(this.api);
  else
    this.onInitializedCallbacks_.push(callback);
};

goog.exportProperty(
    Context.prototype,
    'addOnInitializedCallback',
    Context.prototype.addOnInitializedCallback);

/**
 * Returns the API object, or null if the initialization didn't succeed.
 * @return {GSC.PcscLiteClient.API}
 */
Context.prototype.getApi = function() {
  return this.api;
};

goog.exportProperty(Context.prototype, 'getApi', Context.prototype.getApi);

/**
 * Returns the client title that was passed to the constructor.
 * @return {string}
 */
Context.prototype.getClientTitle = function() {
  return this.clientTitle;
};

goog.exportProperty(
    Context.prototype, 'getClientTitle', Context.prototype.getClientTitle);

/** @override */
Context.prototype.disposeInternal = function() {
  if (this.api) {
    this.api.dispose();
    this.api = null;
  }

  if (this.channel_) {
    this.channel_.dispose();
    this.channel_ = null;
  }

  this.logger.fine('Disposed');

  Context.base(this, 'disposeInternal');
};

/** @private */
Context.prototype.messageChannelEstablishedListener_ = function() {
  this.logger.fine('Message channel was established successfully');

  GSC.Logging.checkWithLogger(this.logger, goog.isNull(this.api));
  GSC.Logging.checkWithLogger(this.logger, !goog.isNull(this.channel_));
  goog.asserts.assert(this.channel_);
  this.api = new GSC.PcscLiteClient.API(this.channel_);

  goog.array.forEach(this.onInitializedCallbacks_, function(callback) {
    callback(this.api);
  }, this);
  this.onInitializedCallbacks_ = [];
};

/** @private */
Context.prototype.messageChannelDisposedListener_ = function() {
  this.logger.fine('Message channel was disposed, disposing...');
  this.dispose();
};

});  // goog.scope
