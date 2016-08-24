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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PromiseHelpers');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.functions');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.promise.Resolver');

goog.scope(function() {

/** @const */
var SERVICE_NAME = 'pcsc_lite_ready';

/** @const */
var GSC = GoogleSmartCard;

/**
 * Tracker of the readiness state of the PC/SC-Lite server.
 *
 * Provides a promise that is fulfilled once the corresponding message is
 * received from the NaCl module with the server, and is rejected if the channel
 * to the NaCl module is disposed (e.g. in case the NaCl module crashes).
 * @param {!goog.messaging.AbstractChannel} naclModuleMessageChannel
 * @constructor
 * @extends goog.Disposable
 */
GSC.PcscLiteServerClientsManagement.ReadinessTracker = function(
    naclModuleMessageChannel) {
  ReadinessTracker.base(this, 'constructor');

  /**
   * @type {!goog.log.Logger}
   * @const
   * @private
   */
  this.logger_ = GSC.Logging.getScopedLogger(
      'PcscLiteServerClientsManagement.ReadinessTracker');

  /**
   * @type {!goog.promise.Resolver.<null>}
   * @private
   */
  this.promiseResolver_ = goog.Promise.withResolver();

  /**
   * Promise that is fulfilled once the PC/SC-Lite server gets ready, or is
   * rejected if it failed to initialize.
   * @type {!goog.Promise.<null>}
   */
  this.promise = this.promiseResolver_.promise;
  GSC.PromiseHelpers.suppressUnhandledRejectionError(this.promise);

  /**
   * Whether the promise is resolved (either fulfilled or rejected).
   * @type {boolean}
   */
  this.isPromiseResolved = false;

  naclModuleMessageChannel.registerService(
      SERVICE_NAME, this.serviceCallback_.bind(this));
  naclModuleMessageChannel.addOnDisposeCallback(
      this.messageChannelDisposedListener_.bind(this));

  this.logger_.fine('Waiting for the "' + SERVICE_NAME + '" message from the ' +
                    'NaCl module...');
};

/** @const */
var ReadinessTracker = GSC.PcscLiteServerClientsManagement.ReadinessTracker;

goog.inherits(ReadinessTracker, goog.Disposable);

/** @override */
ReadinessTracker.prototype.disposeInternal = function() {
  this.reject_(new Error('The readiness tracker is disposed'));

  ReadinessTracker.base(this, 'disposeInternal');
};

/** @private */
ReadinessTracker.prototype.serviceCallback_ = function() {
  this.resolve_();
};

/** @private */
ReadinessTracker.prototype.messageChannelDisposedListener_ = function() {
  this.reject_(new Error('The NaCl module message channel was disposed'));
  this.dispose();
};

/** @private */
ReadinessTracker.prototype.resolve_ = function() {
  if (!this.isPromiseResolved) {
    this.logger_.fine('Setting readiness state to "ready"');
    this.isPromiseResolved = true;
    this.promiseResolver_.resolve(null);
  }
};

/**
 * @param {*} error
 * @private
 */
ReadinessTracker.prototype.reject_ = function(error) {
  if (!this.isPromiseResolved) {
    this.logger_.fine('Setting readiness state to "failed"');
    this.isPromiseResolved = true;
    this.promiseResolver_.reject(error);
  }
};

});  // goog.scope
