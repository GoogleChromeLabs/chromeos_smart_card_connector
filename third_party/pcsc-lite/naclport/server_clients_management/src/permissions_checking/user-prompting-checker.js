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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');

goog.require('GoogleSmartCard.ContainerHelpers');
goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.ExtensionId');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessagingOrigin');
goog.require('GoogleSmartCard.Packaging');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientInfo');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistry');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistryImpl');
goog.require('GoogleSmartCard.PopupOpener');
goog.require('GoogleSmartCard.PromisifyExtensionApi');
goog.require('goog.Promise');
goog.require('goog.iter');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

const LOCAL_STORAGE_KEY = 'pcsc_lite_clients_user_selections';

const USER_PROMPT_DIALOG_URL =
    'pcsc_lite_server_clients_management/user-prompt-dialog.html';

const USER_PROMPT_DIALOG_WINDOW_WIDTH = 300;

const GSC = GoogleSmartCard;

const PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;
const promisify = GSC.PromisifyExtensionApi.promisify;

/** @type {!GSC.PcscLiteServer.TrustedClientsRegistry|null} */
let trustedClientsRegistryOverrideForTesting = null;
/** @type {function(string,!GSC.PopupOpener.WindowOptions,!Object=)|null} */
let modalDialogRunnerOverrideForTesting = null;

/**
 * This class encapsulates the part of the client app permission check that is
 * related to prompting the user about permissions and loading/storing these
 * prompt results.
 * @constructor
 */
PermissionsChecking.UserPromptingChecker = function() {
  /** @private @const */
  this.trustedClientsRegistry_ =
      trustedClientsRegistryOverrideForTesting !== null ?
      trustedClientsRegistryOverrideForTesting :
      new GSC.PcscLiteServer.TrustedClientsRegistryImpl();

  // Start asynchronously loading the previous user approvals stored in the
  // local storage.
  /** @type {!Promise<!Map<string,boolean>>} @private @const */
  this.localStoragePromise_ = this.loadLocalStorage_();

  /** @type {!Map.<string, !Promise<void>>} @private @const */
  this.checkPromiseMap_ = new Map();

  /** @type {!Set.<string>} @private @const */
  this.pendingClientOrigins_ = new Set();
};

const UserPromptingChecker = PermissionsChecking.UserPromptingChecker;

/**
 * @param {!GSC.PcscLiteServer.TrustedClientsRegistry|null}
 *     trustedClientsRegistry
 */
UserPromptingChecker.overrideTrustedClientsRegistryForTesting = function(
    trustedClientsRegistry) {
  trustedClientsRegistryOverrideForTesting = trustedClientsRegistry;
};

/**
 * @param {function(string,!GSC.PopupOpener.WindowOptions,!Object=)|null}
 *     modalDialogRunner
 */
UserPromptingChecker.overrideModalDialogRunnerForTesting = function(
    modalDialogRunner) {
  modalDialogRunnerOverrideForTesting = modalDialogRunner;
};

/**
 * @type {!goog.log.Logger}
 * @const
 */
UserPromptingChecker.prototype.logger =
    GSC.Logging.getScopedLogger('Pcsc.PermissionsPromptingChecker');

/**
 * Starts the permission check for the given client app - against the previously
 * stored user prompt results and via prompting the user.
 *
 * The result is returned asynchronously as a promise (which will eventually be
 * resolved if the permission is granted or rejected otherwise).
 * @param {string} clientOrigin
 * @return {!Promise<void>}
 */
UserPromptingChecker.prototype.check = function(clientOrigin) {
  goog.log.log(
      this.logger, goog.log.Level.FINEST,
      'Checking permissions for client App ' + clientOrigin + '...');

  // Avoid concurrent checks for the same origin, as they might trigger
  // duplicate user prompts. Hence check against `checkPromiseMap_` whether this
  // origin is/was already processed.
  const existingPromise = this.checkPromiseMap_.get(clientOrigin);
  if (existingPromise !== undefined) {
    // Found a previously started request, so attach to its promise: it'll make
    // the caller wait for its completion, and additionally serves as a cache in
    // case the check already completed.
    goog.log.log(
        this.logger, goog.log.Level.FINEST,
        'Found the existing promise for the permission checking of the ' +
            'client ' + clientOrigin + ', returning it');
    return existingPromise;
  }

  // Start a new asynchronous operation. Track it in `checkPromiseMap_` to let
  // all subsequent requests wait for our completion and reuse our result.
  goog.log.fine(
      this.logger,
      'Checking permissions for client ' + clientOrigin + ': no ' +
          'existing promise was found, performing the actual check...');
  const promise = this.doCheck(clientOrigin);
  this.checkPromiseMap_.set(clientOrigin, promise);
  return promise;
};

/**
 * @param {string} clientOrigin
 * @return {!Promise<void>}
 */
UserPromptingChecker.prototype.doCheck = async function(clientOrigin) {
  /** @type {!GSC.PcscLiteServer.TrustedClientInfo|null} */
  let trustedClient = null;
  try {
    trustedClient =
        await this.trustedClientsRegistry_.getByOrigin(clientOrigin);
  } catch (exc) {
    // The client is untrusted, so leave `trustedClient` null.
  }

  if (trustedClient && trustedClient.autoapprove) {
    // The permission is auto-approved for this origin according to the config.
    return;
  }

  const storedUserSelections = await this.localStoragePromise_;

  if (!storedUserSelections.has(clientOrigin)) {
    goog.log.fine(
        this.logger,
        'No stored user selection was found for the client ' + clientOrigin +
            ', going to show the user prompt...');
    if (!trustedClient)
      return await this.promptUserForUntrustedClient_(clientOrigin);
    return await this.promptUserForTrustedClient_(clientOrigin, trustedClient);
  }

  const userSelection = storedUserSelections.get(clientOrigin);
  if (userSelection) {
    goog.log.info(
        this.logger,
        'Granted permission to client ' + clientOrigin +
            ' due to the stored user selection');
    return;
  }

  goog.log.info(
      this.logger,
      'Rejected permission to client ' + clientOrigin +
          ' due to the stored user selection');
  // Note: Not using "throw" here, to avoid confusing the JavaScript debugger.
  return Promise.reject(
      new Error('Rejected permission due to the stored user selection'));
};

/**
 * @param {string} clientOrigin
 * @return {!Promise<void>}
 */
UserPromptingChecker.prototype.cancelUserPromptIfPending = async function(
    clientOrigin) {
  if (this.pendingClientOrigins_.has(clientOrigin)) {
    goog.log.info(
      this.logger,
      'Close the prompt dialog for client ' + clientOrigin +
          ' as permissions have been granted via the managed registry');
    this.pendingClientOrigins_.delete(clientOrigin);
    await GSC.PopupOpener.closeModalDialog(clientOrigin);
  }
}

/**
 * @return {!Promise<!Map<string, boolean>>}
 * @private
 */
UserPromptingChecker.prototype.loadLocalStorage_ = async function() {
  /** @type {!Object} */
  const rawData = await this.getLocalStorageRawData_();
  goog.log.log(
      this.logger, goog.log.Level.FINER,
      'Loaded the following data from the local storage: ' +
          GSC.DebugDump.debugDumpFull(rawData));

  const storedUserSelections = this.parseLocalStorageUserSelections_(rawData);

  const itemsForLog = [];
  storedUserSelections.forEach(function(userSelection, clientOrigin) {
    itemsForLog.push(
        (userSelection ? 'allow' : 'deny') + ' for ' + clientOrigin + '"');
  });
  goog.log.info(
      this.logger,
      'Loaded local storage data with the stored user selections: ' +
          (itemsForLog.length ? goog.iter.join(itemsForLog, ', ') : 'no data'));

  return storedUserSelections;
};

/**
 * @private
 * @return {!Promise<!Object>}
 */
UserPromptingChecker.prototype.getLocalStorageRawData_ = async function() {
  goog.log.fine(
      this.logger,
      `Loading local storage data with the stored user selections (the key ` +
          `is "${LOCAL_STORAGE_KEY}")...`);
  try {
    return await promisify(
        chrome.storage.local, chrome.storage.local.get, LOCAL_STORAGE_KEY);
  } catch (e) {
    throw new Error(`chrome.storage.local.get failed: ${e}`);
  }
};

/**
 * @param {string} identifier
 * @return {string}
 */
function fixupLegacyStoredIdentifier(identifier) {
  if (GSC.ExtensionId.looksLikeExtensionId(identifier)) {
    // Old versions of the Smart Card Connector stored user decisions as simply
    // extension IDs. Now that we've switched to using origins, automatically
    // transform from the old format into the new one.
    return GSC.MessagingOrigin.getFromExtensionId(identifier);
  }
  return identifier;
}

/**
 * @param {!Object} rawData
 * @return {!Map.<string, boolean>}
 * @private
 */
UserPromptingChecker.prototype.parseLocalStorageUserSelections_ = function(
    rawData) {
  /** @type {!Map.<string, boolean>} */
  const storedUserSelections = new Map();
  if (!rawData[LOCAL_STORAGE_KEY])
    return storedUserSelections;
  const contents = rawData[LOCAL_STORAGE_KEY];
  if (!goog.isObject(contents)) {
    goog.log.warning(
        this.logger,
        'Corrupted local storage data - expected an object, received: ' +
            GSC.DebugDump.debugDumpFull(contents));
    return storedUserSelections;
  }
  goog.object.forEach(contents, function(userSelection, identifier) {
    if (typeof userSelection !== 'boolean') {
      goog.log.warning(
          this.logger,
          'Corrupted local storage entry - expected the object value to ' +
              'be a boolean, received: ' +
              GSC.DebugDump.debugDumpFull(userSelection));
      return;
    }
    const clientOrigin = fixupLegacyStoredIdentifier(identifier);
    storedUserSelections.set(clientOrigin, userSelection);
  }, this);
  return storedUserSelections;
};

/**
 * @param {string} clientOrigin
 * @return {string}
 */
function getClientInfoLink(clientOrigin) {
  const url = new URL(clientOrigin);
  if (url.protocol === GSC.MessagingOrigin.EXTENSION_PROTOCOL) {
    // For extensions/apps, the info link should point to WebStore.
    const extensionId = url.hostname;
    return `https://chrome.google.com/webstore/detail/${extensionId}`;
  }
  // For all other origins, assume they're themselves valid and informative
  // links.
  return clientOrigin;
}

/**
 * @param {string} clientOrigin
 * @return {string}
 */
function getNameToShowForUnknownClient(clientOrigin) {
  const url = new URL(clientOrigin);
  if (url.protocol === GSC.MessagingOrigin.EXTENSION_PROTOCOL) {
    // For unknown extensions/apps, show their ID.
    const extensionId = url.hostname;
    return extensionId;
  }
  // For unknown web pages, show their host (which, unlike "hostname", includes
  // the port number).
  return url.host;
}

/**
 * @param {string} clientOrigin
 * @param {!GSC.PcscLiteServer.TrustedClientInfo} trustedClientInfo
 * @return {!Promise<void>}
 * @private
 */
UserPromptingChecker.prototype.promptUserForTrustedClient_ =
    async function(clientOrigin, trustedClientInfo) {
  goog.log.info(
      this.logger,
      'Showing the user prompt for the known client ' + clientOrigin +
          ' named "' + trustedClientInfo.name + '"...');
  return await this.runPromptDialog_(clientOrigin, {
    'is_client_known': true,
    'client_info_link': getClientInfoLink(clientOrigin),
    'client_name': trustedClientInfo.name
  });
};

/**
 * @param {string} clientOrigin
 * @return {!Promise<void>}
 * @private
 */
UserPromptingChecker.prototype.promptUserForUntrustedClient_ =
    async function(clientOrigin) {
  goog.log.info(
      this.logger,
      'Showing the warning user prompt for the unknown client ' + clientOrigin +
          '...');
  return await this.runPromptDialog_(clientOrigin, {
    'is_client_known': false,
    'client_info_link': getClientInfoLink(clientOrigin),
    'client_name': getNameToShowForUnknownClient(clientOrigin)
  });
};

/**
 * @param {string} clientOrigin
 * @param {!Object} userPromptDialogData
 * @return {!Promise<void>}
 * @private
 */
UserPromptingChecker.prototype.runPromptDialog_ =
    async function(clientOrigin, userPromptDialogData) {
  const modalDialogRunner = modalDialogRunnerOverrideForTesting !== null ?
      modalDialogRunnerOverrideForTesting :
      GSC.PopupOpener.runModalDialog;
  let dialogResult;
  try {
    const userPromptDialogWindowOptions = {
      'id': clientOrigin,
      'width': USER_PROMPT_DIALOG_WINDOW_WIDTH,
    };
    this.pendingClientOrigins_.add(clientOrigin);
    dialogResult = await modalDialogRunner(
        USER_PROMPT_DIALOG_URL, userPromptDialogWindowOptions,
        userPromptDialogData);
  } catch (exc) {
    if (!this.pendingClientOrigins_.has(clientOrigin)) {
      // The user prompt has been cancelled because permission was granted by
      // the managed registry.
      return Promise.resolve();
    }
    this.pendingClientOrigins_.delete(clientOrigin);

    // Note that we don't remember negative user selections persistently.
    goog.log.info(
        this.logger,
        'Rejected permission to client ' + clientOrigin +
            ' because of the user cancellation of the prompt dialog');
    // Note: Not using "throw" here, to avoid confusing the JavaScript debugger.
    return Promise.reject(new Error(
        'Rejected permission because of the user cancellation of the ' +
        'prompt dialog'));
  }
  if (!dialogResult) {
    this.pendingClientOrigins_.delete(clientOrigin);
    // Note that we don't remember negative user selections persistently.
    goog.log.info(
        this.logger,
        'Rejected permission to client ' + clientOrigin +
            ' based on the "reject" user selection');
    // Note: Not using "throw" here, to avoid confusing the JavaScript debugger.
    return Promise.reject(
        new Error('Reject permission based on the "reject" user selection'));
  }
  goog.log.info(
      this.logger,
      'Granted permission to client ' + clientOrigin +
          ' based on the "grant" user selection');
  // No need to wait for the persisting completion here - hence no "await".
  this.storeUserApproval_(clientOrigin);
};

/**
 * @param {string} clientOrigin
 * @return {!Promise<void>}
 */
UserPromptingChecker.prototype.storeUserApproval_ =
    async function(clientOrigin) {
  goog.log.info(
      this.logger,
      'Storing user selection of the granted permission to client ' +
          clientOrigin);

  let storedUserSelections;
  try {
    storedUserSelections = await this.localStoragePromise_;
  } catch (exc) {
    goog.log.warning(
        this.logger, 'Failed to store the user selection in the local storage');
    // The caller doesn't care whether we succeed or failed, so just return.
    return;
  }
  storedUserSelections.set(clientOrigin, true);
  const dumpedValue =
      GSC.ContainerHelpers.buildObjectFromMap(storedUserSelections);
  goog.log.log(
      this.logger, goog.log.Level.FINER,
      'Storing the following data in the local storage: ' +
          GSC.DebugDump.debugDumpFull(dumpedValue));
  chrome.storage.local.set(goog.object.create(LOCAL_STORAGE_KEY, dumpedValue));
};
});  // goog.scope
