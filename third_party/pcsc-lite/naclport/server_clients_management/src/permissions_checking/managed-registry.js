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

goog.require('GoogleSmartCard.Logging');
goog.require('goog.Promise');
goog.require('goog.array');
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
  /**
   * @type {!goog.promise.Resolver}
   * @private
   */
  this.managedStoragePromiseResolver_ = goog.Promise.withResolver();
  /**
   * @type {!Set.<string>}
   * @private
   */
  this.allowedClientAppIds_ = new Set;

  this.loadManagedStorage_();
  this.listenForStorageChanging_();
};

const ManagedRegistry = PermissionsChecking.ManagedRegistry;

/**
 * @type {!goog.log.Logger}
 * @const
 */
ManagedRegistry.prototype.logger = GSC.Logging.getScopedLogger(
    'PcscLiteServerClientsManagement.PermissionsChecking.ManagedRegistry');

/**
 * Starts the permission check for the given client app against the managed
 * storage, that contains the admin-configured policy.
 *
 * The result is returned asynchronously as a promise (which will eventually be
 * resolved if the permission is granted or rejected otherwise).
 * @param {string} clientAppId
 * @return {!goog.Promise}
 */
ManagedRegistry.prototype.getById = function(clientAppId) {
  const promiseResolver = goog.Promise.withResolver();

  this.managedStoragePromiseResolver_.promise.then(
      function() {
        if (this.allowedClientAppIds_.has(clientAppId)) {
          promiseResolver.resolve();
        } else {
          promiseResolver.reject(new Error(
              'The specified client App id is not listed in the managed ' +
              'registry'));
        }
      },
      function() {
        promiseResolver.reject(new Error(
            'Failed to load the allowed client App ids registry from the ' +
            'managed storage'));
      },
      this);

  return promiseResolver.promise;
};

/** @private */
ManagedRegistry.prototype.loadManagedStorage_ = function() {
  this.logger.fine(
      'Loading managed storage data with the allowed client App ids (the key ' +
      'is "' + MANAGED_STORAGE_KEY + '")...');
  chrome.storage.managed.get(
      MANAGED_STORAGE_KEY, this.managedStorageLoadedCallback_.bind(this));
};

/**
 * @param {!Object} items
 * @private
 */
ManagedRegistry.prototype.managedStorageLoadedCallback_ = function(items) {
  this.logger.fine(
      'Loaded the following data from the managed storage: ' +
      GSC.DebugDump.dump(items));

  if (this.setAllowedClientAppIdsFromStorageData_(
          goog.object.get(items, MANAGED_STORAGE_KEY, []))) {
    this.logger.info(
        'Loaded managed storage data with the allowed client App ids: ' +
        GSC.DebugDump.dump(this.allowedClientAppIds_));
    this.managedStoragePromiseResolver_.resolve();
  } else {
    this.managedStoragePromiseResolver_.reject(new Error(
        'Failed to load the allowed client App ids data from the managed ' +
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
  this.logger.fine(
      'Received the managed storage update event: ' +
      GSC.DebugDump.dump(changes));

  if (changes[MANAGED_STORAGE_KEY]) {
    if (this.setAllowedClientAppIdsFromStorageData_(
            goog.object.get(changes[MANAGED_STORAGE_KEY], 'newValue', []))) {
      this.logger.info(
          'Loaded the updated managed storage data with the allowed client ' +
          'App ids: ' + GSC.DebugDump.dump(this.allowedClientAppIds_));
    }
  }
};

/**
 * @param {*} storageData
 * @return {boolean}
 * @private
 */
ManagedRegistry.prototype.setAllowedClientAppIdsFromStorageData_ = function(
    storageData) {
  if (!goog.isArray(storageData)) {
    this.logger.warning(
        'Failed to load the allowed client App ids data from the managed ' +
        'storage: expected an array, instead got: ' +
        GSC.DebugDump.dump(storageData));
    return false;
  }

  const newAllowedClientAppIds = new Set;
  let success = true;
  goog.array.forEach(/** @type {!Array} */ (storageData), function(item) {
    if (typeof item !== 'string') {
      this.logger.warning(
          'Failed to load the allowed client App id from the managed ' +
          'storage item: expected a string, instead got: ' +
          GSC.DebugDump.dump(item));
      success = false;
      return;
    }
    newAllowedClientAppIds.add(item);
  });

  if (!success)
    return false;
  this.allowedClientAppIds_ = newAllowedClientAppIds;
  return true;
};
});  // goog.scope
