/**
 * @license
 * Copyright 2022 Google Inc.
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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.AdminPolicyService');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.DeferredProcessor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('goog.Disposable');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');

goog.scope(function() {

/**
 * Service name that is used for the message to the PC/SC server that the admin
 * policy has been updated. The constant in
 * //third_party/pcsc-lite/naclport/server_clients_management/src/admin_policy_getter.cc
 * must match this value.
 *
 * The message data will contain the data of the new admin policy.
 */
const UPDATE_ADMIN_POLICY_SERVER_MESSAGE_SERVICE_NAME = 'update_admin_policy';

const GSC = GoogleSmartCard;
const DeferredProcessor = GSC.DeferredProcessor;
const ReadinessTracker = GSC.PcscLiteServerClientsManagement.ReadinessTracker;

/**
 * This class provides the C++ layer with the AdminPolicy obtained from the
 * managed storage. Upon initialization, it reads the current value from
 * storage and sends a message containing the policy data.
 * @param {!goog.messaging.AbstractChannel} serverMessageChannel Message channel
 * to the PC/SC server into which the admin policy will be forwarded.
 * @param {!ReadinessTracker} serverReadinessTracker Tracker of the PC/SC server
 * that allows to delay forwarding of the messages to the server until it is
 * ready to receive them.
 * @constructor
 * @extends goog.Disposable
 */
GSC.PcscLiteServerClientsManagement.AdminPolicyService = function(
    serverMessageChannel, serverReadinessTracker) {
  AdminPolicyService.base(this, 'constructor');

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('Pcsc.AdminPolicyService');

  /** @private */
  this.serverMessageChannel_ = serverMessageChannel;

  /** @private */
  this.serverReadinessTracker_ = serverReadinessTracker;

  // The policy update messages are deferred until the server readiness is
  // established.
  /** @private */
  this.deferredProcessor_ =
      new DeferredProcessor(this.serverReadinessTracker_.promise);

  this.addChannelDisposedListener_();

  /**
   * @type {!Object}
   * @private
   */
  this.adminPolicy_ = {};

  this.loadManagedStorage_();
  this.listenForStorageChanging_();

  goog.log.fine(this.logger, 'Initialized');
};

const AdminPolicyService =
    GSC.PcscLiteServerClientsManagement.AdminPolicyService;

goog.inherits(AdminPolicyService, goog.Disposable);

/** @override */
AdminPolicyService.prototype.disposeInternal = function() {
  this.deferredProcessor_.dispose();
  this.deferredProcessor_ = null;

  this.serverReadinessTracker_ = null;

  this.serverMessageChannel_ = null;

  goog.log.fine(this.logger, 'Disposed');

  AdminPolicyService.base(this, 'disposeInternal');
};

/** @private */
AdminPolicyService.prototype.loadManagedStorage_ = function() {
  chrome.storage.managed.get(this.managedStorageLoadedCallback_.bind(this));
};

/**
 * @param {!Object} items
 * @private
 */
AdminPolicyService.prototype.managedStorageLoadedCallback_ = function(items) {
  if (this.isDisposed()) {
    return;
  }
  goog.log.info(
      this.logger,
      'Loaded the following data from the managed storage: ' +
          GSC.DebugDump.debugDumpFull(items));

  this.updateAdminPolicyFromStorageData_(items);
};

/**
 * Set up listener tracking storage changes.
 * @private
 */
AdminPolicyService.prototype.listenForStorageChanging_ = function() {
  chrome.storage.managed.onChanged.addListener(
      this.storageChangedListener_.bind(this));
};

/**
 * @param {!Object} changes
 * @private
 */
AdminPolicyService.prototype.storageChangedListener_ = function(changes) {
  if (this.isDisposed())
    return;

  this.loadManagedStorage_();
};

/**
 * @param {!Object} storageData
 * @private
 */
AdminPolicyService.prototype.updateAdminPolicyFromStorageData_ = function(
    storageData) {
  this.adminPolicy_ = storageData;
  this.deferredProcessor_.addJob(
      this.sendServerUpdateAdminPolicyMessage_.bind(this));
};

/**
 * Sends a message to the server containing the new admin policy.
 * @private
 */
AdminPolicyService.prototype.sendServerUpdateAdminPolicyMessage_ = function() {
  GSC.Logging.checkWithLogger(this.logger, this.serverMessageChannel_);
  GSC.Logging.checkWithLogger(
      this.logger, !this.serverMessageChannel_.isDisposed());
  this.serverMessageChannel_.send(
      UPDATE_ADMIN_POLICY_SERVER_MESSAGE_SERVICE_NAME, this.adminPolicy_);
};

/**
 * Set up the listener tracking the lifetime of the server message channel.
 * @private
 */
AdminPolicyService.prototype.addChannelDisposedListener_ = function() {
  this.serverMessageChannel_.addOnDisposeCallback(
      this.serverMessageChannelDisposedListener_.bind(this));
};

/**
 * This listener is called when the server message channel is disposed.
 * Disposes self, as no more policy updates should be sent after this point.
 * @private
 */
AdminPolicyService.prototype.serverMessageChannelDisposedListener_ =
    function() {
  if (this.isDisposed())
    return;
  goog.log.warning(
      this.logger, 'Server message channel was disposed, disposing...');

  this.serverMessageChannel_ = null;

  this.dispose();
};
});  // goog.scope
