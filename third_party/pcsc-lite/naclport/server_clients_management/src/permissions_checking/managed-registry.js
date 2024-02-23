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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.ManagedRegistry');

goog.require('GoogleSmartCard.ExtensionId');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessagingOrigin');
goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.array');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

const MANAGED_STORAGE_KEY = 'force_allowed_client_app_ids';

const GSC = GoogleSmartCard;
const PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;

/**
 * This class encapsulates the part of the client app permission check that is
 * related to policy configured by the admin via policy for extensions.
 * @constructor
 */
PermissionsChecking.ManagedRegistry = function() {
  /** @type {!goog.promise.Resolver} @private @const */
  this.managedStoragePromiseResolver_ = goog.Promise.withResolver();

  /**
   * Map storing promise resolvers per client origin for pending requests.
   * @type {!Map.<string, !goog.promise.Resolver>} @private @const
   */
  this.pendingPermissionResolvers_ = new Map();

  /**
   * @type {!Set.<string>}
   * @private
   */
  this.allowedClientOrigins_ = new Set();

  this.loadManagedStorage_();
  this.listenForStorageChanging_();
};

const ManagedRegistry = PermissionsChecking.ManagedRegistry;

/**
 * @type {!goog.log.Logger}
 * @const
 */
ManagedRegistry.prototype.logger =
    GSC.Logging.getScopedLogger('Pcsc.ManagedPermissionsRegistry');

/**
 * Starts the permission check for the given client app against the managed
 * storage, that contains the admin-configured policy.
 *
 * The result is returned asynchronously as a promise (which will eventually be
 * resolved if the permission is granted or rejected otherwise).
 * @param {string} clientOrigin Origin of the client application
 * @return {!goog.Thenable}
 */
ManagedRegistry.prototype.getByOrigin = function(clientOrigin) {
  const promiseResolver = goog.Promise.withResolver();

  this.managedStoragePromiseResolver_.promise.then(
      function() {
        if (this.allowedClientOrigins_.has(clientOrigin)) {
          promiseResolver.resolve();
        } else {
          promiseResolver.reject(new Error(
              'The specified client origin is not listed in the managed ' +
              'registry'));
        }
      },
      function() {
        promiseResolver.reject(new Error(
            'Failed to load the allowed client origins registry from the ' +
            'managed storage'));
      },
      this);

  return promiseResolver.promise;
};

/**
 * Adds the passed clientOrigin and promise resolver corresponding to a
 * currently pending permissions request to a map. If the managed registry is
 * updated whilst the request is still pending and a permission for the client
 * origin is added, the promise will be resolved.
 * @param {string} clientOrigin
 * @param {!goog.promise.Resolver} promiseResolver
 */
ManagedRegistry.prototype.startMonitoringPermissionsForClientOrigin = function(
    clientOrigin, promiseResolver) {
  this.pendingPermissionResolvers_.set(clientOrigin, promiseResolver);
};

/**
 * Removes the client origin and its corresponding permissions resolver from the
 * map.
 * @param {string} clientOrigin
 */
ManagedRegistry.prototype.stopMonitoringPermissionsForClientOrigin = function(
    clientOrigin) {
  this.pendingPermissionResolvers_.delete(clientOrigin);
};

/** @private */
ManagedRegistry.prototype.loadManagedStorage_ = function() {
  goog.log.fine(
      this.logger,
      'Loading managed storage data with the allowed client origins (the key ' +
          'is "' + MANAGED_STORAGE_KEY + '")...');
  chrome.storage.managed.get(
      MANAGED_STORAGE_KEY, this.managedStorageLoadedCallback_.bind(this));
};

/**
 * @param {!Object} items
 * @private
 */
ManagedRegistry.prototype.managedStorageLoadedCallback_ = function(items) {
  goog.log.fine(
      this.logger,
      'Loaded the following data from the managed storage: ' +
          GSC.DebugDump.debugDumpFull(items));

  if (this.setAllowedClientOriginsFromStorageData_(
          goog.object.get(items, MANAGED_STORAGE_KEY, []))) {
    goog.log.info(
        this.logger,
        'Loaded managed storage data with the allowed client origins: ' +
            GSC.DebugDump.debugDumpFull(this.allowedClientOrigins_));
    this.managedStoragePromiseResolver_.resolve();
  } else {
    this.managedStoragePromiseResolver_.reject(new Error(
        'Failed to load the allowed client origins data from the managed ' +
        'storage'));
  }
};

/** @private */
ManagedRegistry.prototype.listenForStorageChanging_ = function() {
  chrome.storage.onChanged.addListener(this.storageChangedListener_.bind(this));
};

/**
 * @param {!Object<string,!StorageChange>} changes
 * @param {string} areaName
 * @private
 */
ManagedRegistry.prototype.storageChangedListener_ = function(
    changes, areaName) {
  if (areaName != 'managed')
    return;
  goog.log.fine(
      this.logger,
      'Received the managed storage update event: ' +
          GSC.DebugDump.debugDumpFull(changes));

  if (changes[MANAGED_STORAGE_KEY]) {
    if (this.setAllowedClientOriginsFromStorageData_(
            goog.object.get(changes[MANAGED_STORAGE_KEY], 'newValue', []))) {
      goog.log.info(
          this.logger,
          'Loaded the updated managed storage data with the allowed client ' +
              'origins: ' +
              GSC.DebugDump.debugDumpFull(this.allowedClientOrigins_));
      this.updatePendingPermissionResolvers_();
    }
  }
};

/**
 * @param {*} storageData
 * @return {boolean}
 * @private
 */
ManagedRegistry.prototype.setAllowedClientOriginsFromStorageData_ = function(
    storageData) {
  if (!Array.isArray(storageData)) {
    goog.log.warning(
        this.logger,
        'Failed to load the allowed client origins data from the managed ' +
            'storage: expected an array, instead got: ' +
            GSC.DebugDump.debugDumpFull(storageData));
    return false;
  }

  const newAllowedClientOrigins = new Set();
  let success = true;
  goog.array.forEach(/** @type {!Array} */ (storageData), function(item) {
    if (typeof item !== 'string') {
      goog.log.warning(
          this.logger,
          'Failed to load the allowed client origin from the managed ' +
              'storage item: expected a string, instead got: ' +
              GSC.DebugDump.debugDumpFull(item));
      success = false;
      return;
    }
    const clientOrigin = getClientOriginFromPolicyValue(item);
    newAllowedClientOrigins.add(clientOrigin);
  });

  if (!success)
    return false;
  this.allowedClientOrigins_ = newAllowedClientOrigins;
  return true;
};


/**
 * Resolves any pending permissions requests for allowed client origins.
 * @private
 */
ManagedRegistry.prototype.updatePendingPermissionResolvers_ = function() {
  const clientOrigins = Array.from(this.pendingPermissionResolvers_.keys());
  goog.array.forEach(clientOrigins, (clientOrigin) => {
    if (this.allowedClientOrigins_.has(clientOrigin)) {
      const promiseResolver = this.pendingPermissionResolvers_.get(
          clientOrigin);
      if (promiseResolver !== undefined) {
        this.pendingPermissionResolvers_.delete(clientOrigin);
        promiseResolver.resolve();
      }
    }
  });
}

/**
 * Returns a client origin from the given policy-configured string.
 *
 * Policies use one of the following two formats: either an app/extension ID or
 * a full origin like "chrome-extension://something".
 * @param {string} stringFromPolicy
 * @return {string}
 */
function getClientOriginFromPolicyValue(stringFromPolicy) {
  if (GSC.ExtensionId.looksLikeExtensionId(stringFromPolicy)) {
    // Construct the "chrome-extension://..." origin.
    return GSC.MessagingOrigin.getFromExtensionId(stringFromPolicy);
  }
  // Assume the policy specifies an origin. TODO: Perform a sanity check.
  return stringFromPolicy;
}
});  // goog.scope
