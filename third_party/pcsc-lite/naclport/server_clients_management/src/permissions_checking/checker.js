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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.Checker');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.ManagedRegistry');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');
goog.require('goog.Promise');
goog.require('goog.log.Logger');
goog.require('goog.promise.Resolver');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;

/**
 * Provides an interface for performing permission checks for the given client
 * apps, which gates access to sending PC/SC requests to our Smart Card
 * Connector app.
 *
 * Criteria for granting the permissions, in the order of their application:
 * 1. Allow if admin force-allowed the permission via policy for extensions (the
 *    "force_allowed_client_app_ids" key).
 * 2. Allow/reject if the user previously allowed/rejected the permission. (The
 *    "allow" decisions are stored persistently in the local storage; the
 *    "reject" decisions are stored in-memory till the next restart.)
 * 3. Show the dialog to the user and allow/reject the permission based on the
 *    result.
 * @constructor
 */
PermissionsChecking.Checker = function() {
  /** @private */
  this.managedRegistry_ = new PermissionsChecking.ManagedRegistry;
  /** @private */
  this.userPromptingChecker_ = new PermissionsChecking.UserPromptingChecker;
};

/** @const */
var Checker = PermissionsChecking.Checker;

/**
 * @type {!goog.log.Logger}
 * @const
 */
Checker.prototype.logger = GSC.Logging.getScopedLogger(
    'PcscLiteServerClientsManagement.PermissionsChecking.Checker');

/**
 * Starts the permission check for the given client app.
 *
 * The result is returned asynchronously as a promise (which will be eventually
 * resolved if the permission is granted or rejected otherwise).
 * @param {string|null} clientAppId ID of the client app, or null if the client
 * is our own app.
 * @return {!goog.Promise}
 */
Checker.prototype.check = function(clientAppId) {
  this.logger.finer('Checking permissions for client App with id ' +
                    GSC.DebugDump.dump(clientAppId) + '...');

  if (goog.isNull(clientAppId)) {
    this.logger.finer('Granted permissions for client with null App id');
    return goog.Promise.resolve();
  }

  var checkPromiseResolver = goog.Promise.withResolver();

  this.checkByManagedRegistry_(clientAppId, checkPromiseResolver);

  return checkPromiseResolver.promise;
};

/**
 * @param {string} clientAppId
 * @param {!goog.promise.Resolver} checkPromiseResolver
 * @private
 */
Checker.prototype.checkByManagedRegistry_ = function(
    clientAppId, checkPromiseResolver) {
  this.logger.finer('Checking permissions for the client App with id "' +
                    clientAppId + '" through the managed registry...');

  this.managedRegistry_.getById(clientAppId).then(
      function() {
        this.logger.finer('Granted permissions for client App with id "' +
                          clientAppId + '" through the managed registry');
        checkPromiseResolver.resolve();
      },
      function() {
        this.logger.finer('No permissions found for client App with id "' +
                          clientAppId + '" through the managed registry');
        this.checkByUserPromptingChecker_(clientAppId, checkPromiseResolver);
      },
      this);
};

/**
 * @param {string} clientAppId
 * @param {!goog.promise.Resolver} checkPromiseResolver
 * @private
 */
Checker.prototype.checkByUserPromptingChecker_ = function(
    clientAppId, checkPromiseResolver) {
  this.userPromptingChecker_.check(clientAppId).then(
      checkPromiseResolver.resolve, checkPromiseResolver.reject);
};

});  // goog.scope
