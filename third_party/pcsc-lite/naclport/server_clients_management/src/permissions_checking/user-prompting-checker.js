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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');

goog.require('GoogleSmartCard.ContainerHelpers');
goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.KnownAppsRegistry');
goog.require('GoogleSmartCard.PopupWindow.Server');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.functions');
goog.require('goog.iter');
goog.require('goog.log.Logger');
goog.require('goog.object');
goog.require('goog.promise.Resolver');
goog.require('goog.structs');

goog.scope(function() {

/** @const */
var LOCAL_STORAGE_KEY = 'pcsc_lite_clients_user_selections';

/** @const */
var USER_PROMPT_DIALOG_URL =
    'pcsc_lite_server_clients_management/user-prompt-dialog.html';

/** @const */
var USER_PROMPT_DIALOG_WINDOW_OPTIONS_OVERRIDES = {
  'innerBounds': {
    'width': 300
  }
};

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;

/**
 * This class encapsulates the part of the client app permission check that is
 * related to prompting the user about permissions and loading/storing these
 * prompt results.
 * @constructor
 */
PermissionsChecking.UserPromptingChecker = function() {
  /** @private */
  this.knownAppsRegistry_ = new PermissionsChecking.KnownAppsRegistry;

  /**
   * @type {!goog.promise.Resolver.<!Map.<string,boolean>>}
   * @private
   */
  this.localStoragePromiseResolver_ = goog.Promise.withResolver();
  this.loadLocalStorage_();

  /**
   * @type {!Map.<string, !goog.Promise>}
   * @private
   */
  this.checkPromiseMap_ = new Map;
};

/** @const */
var UserPromptingChecker = PermissionsChecking.UserPromptingChecker;

/**
 * @type {!goog.log.Logger}
 * @const
 */
UserPromptingChecker.prototype.logger = GSC.Logging.getScopedLogger(
    'PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');

/**
 * Starts the permission check for the given client app - against the previously
 * stored user prompt results and via prompting the user.
 *
 * The result is returned asynchronously as a promise (which will eventually be
 * resolved if the permission is granted or rejected otherwise).
 * @param {string} clientAppId
 * @return {!goog.Promise}
 */
UserPromptingChecker.prototype.check = function(clientAppId) {
  this.logger.finest('Checking permissions for client App with id "' +
                     clientAppId + '"...');

  var existingPromise = this.checkPromiseMap_.get(clientAppId);
  if (goog.isDef(existingPromise)) {
    this.logger.finest(
        'Found the existing promise for the permission checking of the ' +
        'client App with id "' + clientAppId + '", returning it');
    return existingPromise;
  }

  this.logger.fine(
      'Checking permissions for client App with id "' + clientAppId + '": no ' +
      'existing promise was found, performing the actual check...');

  var promiseResolver = goog.Promise.withResolver();
  this.checkPromiseMap_.set(clientAppId, promiseResolver.promise);

  this.localStoragePromiseResolver_.promise.then(
      function(storedUserSelections) {
        if (storedUserSelections.has(clientAppId)) {
          var userSelection = storedUserSelections.get(clientAppId);
          if (userSelection) {
            this.logger.info(
                'Granted permission to client App with id "' + clientAppId +
                '" due to the stored user selection');
            promiseResolver.resolve();
          } else {
            this.logger.info(
                'Rejected permission to client App with id "' + clientAppId +
                '" due to the stored user selection');
            this.notifyAboutRejectionByStoredSelection_(clientAppId);
            promiseResolver.reject(new Error(
                'Rejected permission due to the stored user selection'));
          }
        } else {
          this.logger.fine(
              'No stored user selection was found for the client App with id ' +
              '"' + clientAppId + '", going to show the user prompt...');
          this.promptUser_(clientAppId, promiseResolver);
        }
      },
      function() {
        promiseResolver.reject('Failed to load the local storage data');
      },
      this);

  return promiseResolver.promise;
};

/** @private */
UserPromptingChecker.prototype.loadLocalStorage_ = function() {
  this.logger.fine(
      'Loading local storage data with the stored user selections (the key ' +
      'is "' + LOCAL_STORAGE_KEY + '")...');
  chrome.storage.local.get(
      LOCAL_STORAGE_KEY, this.localStorageLoadedCallback_.bind(this));
};

/**
 * @param {!Object} items
 * @private
 */
UserPromptingChecker.prototype.localStorageLoadedCallback_ = function(items) {
  this.logger.finer('Loaded the following data from the local storage: ' +
                    GSC.DebugDump.dump(items));
  var storedUserSelections = this.parseLocalStorageUserSelections_(items);
  var itemsForLog = [];
  storedUserSelections.forEach(function(userSelection, extensionId) {
    itemsForLog.push((userSelection ? 'allow' : 'deny') + ' for extension "' +
                     extensionId + '"');
  });
  this.logger.info(
      'Loaded local storage data with the stored user selections: ' +
      (itemsForLog.length ? goog.iter.join(itemsForLog, ', ') : 'no data'));

  this.localStoragePromiseResolver_.resolve(storedUserSelections);
};

/**
 * @param {!Object} localStorageItems
 * @return {!Map.<string, boolean>}
 * @private
 */
UserPromptingChecker.prototype.parseLocalStorageUserSelections_ = function(
    localStorageItems) {
  /** @type {!Map.<string, boolean>} */
  var storedUserSelections = new Map;
  if (!localStorageItems[LOCAL_STORAGE_KEY])
    return storedUserSelections;
  var contents = localStorageItems[LOCAL_STORAGE_KEY];
  if (!goog.isObject(contents)) {
    this.logger.warning(
        'Corrupted local storage data - expected an object, received: ' +
        GSC.DebugDump.dump(contents));
    return storedUserSelections;
  }
  goog.object.forEach(contents, function(userSelection, appId) {
    if (!goog.isBoolean(userSelection)) {
      this.logger.warning(
          'Corrupted local storage entry - expected the object value to ' +
          'be a boolean, received: ' + GSC.DebugDump.dump(userSelection));
      return;
    }
    storedUserSelections.set(appId, userSelection);
  }, this);
  return storedUserSelections;
};

/**
 * @param {string} clientAppId
 * @param {!goog.promise.Resolver} promiseResolver
 * @private
 */
UserPromptingChecker.prototype.promptUser_ = function(
    clientAppId, promiseResolver) {
  this.knownAppsRegistry_.getById(clientAppId).then(function(knownApp) {
    this.promptUserForKnownApp_(clientAppId, knownApp, promiseResolver);
  }, function() {
    this.promptUserForUnknownApp_(clientAppId, promiseResolver);
  }, this);
};

/**
 * @param {string} clientAppId
 * @param {!PermissionsChecking.KnownApp} knownApp
 * @param {!goog.promise.Resolver} promiseResolver
 * @private
 */
UserPromptingChecker.prototype.promptUserForKnownApp_ = function(
    clientAppId, knownApp, promiseResolver) {
  this.logger.info(
      'Showing the user prompt for the known client App with id "' +
      clientAppId + '" and name "' + knownApp.name + '"...');
  this.runPromptDialog_(clientAppId, {
    'is_client_known': true,
    'client_app_id': clientAppId,
    'client_app_name': knownApp.name
  }, promiseResolver);
};

/**
 * @param {string} clientAppId
 * @param {!goog.promise.Resolver} promiseResolver
 * @private
 */
UserPromptingChecker.prototype.promptUserForUnknownApp_ = function(
    clientAppId, promiseResolver) {
  this.logger.info(
      'Showing the warning user prompt for the unknown client App with id "' +
      clientAppId + '"...');
  this.runPromptDialog_(
      clientAppId,
      {'is_client_known': false, 'client_app_id': clientAppId},
      promiseResolver);
};

/**
 * @param {string} clientAppId
 * @param {!Object} userPromptDialogData
 * @param {!goog.promise.Resolver} promiseResolver
 * @private
 */
UserPromptingChecker.prototype.runPromptDialog_ = function(
    clientAppId, userPromptDialogData, promiseResolver) {
  var dialogPromise = GSC.PopupWindow.Server.runModalDialog(
      USER_PROMPT_DIALOG_URL,
      USER_PROMPT_DIALOG_WINDOW_OPTIONS_OVERRIDES,
      userPromptDialogData);
  dialogPromise.then(function(dialogResult) {
    if (dialogResult) {
      this.logger.info('Granted permission to client App with id "' +
                       clientAppId + '" based on the "grant" user selection');
      this.storeUserSelection_(clientAppId, true);
      promiseResolver.resolve();
    } else {
      this.logger.info('Rejected permission to client App with id "' +
                       clientAppId + '" based on the "reject" user selection');
      this.storeUserSelection_(clientAppId, false);
      promiseResolver.reject(new Error(
          'Reject permission based on the "reject" user selection'));
    }
  }, function() {
    this.logger.info(
        'Rejected permission to client App with id "' + clientAppId +
        '" because of the user cancellation of the prompt dialog');
    this.storeUserSelection_(clientAppId, false);
    promiseResolver.reject(new Error(
        'Rejected permission because of the user cancellation of the prompt ' +
        'dialog'));
  }, this);
};

/**
 * @param {string} clientAppId
 * @param {boolean} userSelection
 */
UserPromptingChecker.prototype.storeUserSelection_ =
    function(clientAppId, userSelection) {
  // Don't remember negative user selections persistently.
  if (!userSelection)
    return;

  this.logger.info(
      'Storing user selection of the ' +
      (userSelection ? 'granted' : 'rejected') + ' permission to client App ' +
      'with id "' + clientAppId + '"');

  this.localStoragePromiseResolver_.promise.then(
      function(/** !Map */ storedUserSelections) {
        storedUserSelections.set(clientAppId, userSelection);
        var dumpedValue = GSC.ContainerHelpers.buildObjectFromMap(
            storedUserSelections);
        this.logger.finer('Storing the following data in the local storage: ' +
                          GSC.DebugDump.dump(dumpedValue));
        chrome.storage.local.set(goog.object.create(
            LOCAL_STORAGE_KEY, dumpedValue));
      },
      function() {
        this.logger.warning(
            'Failed to store the user selection in the local storage');
      },
      this);
};

/**
 * @param {string} clientAppId
 */
UserPromptingChecker.prototype.notifyAboutRejectionByStoredSelection_ =
    function(clientAppId) {
  // TODO: implement showing rich notifications to the user here
};

/**
 * @param {string} clientAppId
 */
UserPromptingChecker.prototype.notifyAboutRejectionOfUnknownApp_ = function(
    clientAppId) {
  // TODO: implement showing rich notifications to the user here
};

});  // goog.scope
