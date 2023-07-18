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

goog.provide('GoogleSmartCard.Pcsc.PolicyOrPromptingPermissionsChecker');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessagingOrigin');
goog.require('GoogleSmartCard.Pcsc.PermissionsChecker');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.ManagedRegistry');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.promise.Resolver');

goog.scope(function() {

const GSC = GoogleSmartCard;
const PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('Pcsc.PermissionsChecker');

/**
 * Provides an interface for performing permission checks for the given client
 * applications, which gates access to sending PC/SC requests to our Smart Card
 * Connector.
 *
 * Criteria for granting the permissions, in the order of their application:
 * 1. Allow if admin force-allowed the permission via policy for extensions (the
 *    "force_allowed_client_app_ids" key).
 * 2. Allow/reject if the user previously allowed/rejected the permission. (The
 *    "allow" decisions are stored persistently in the local storage; the
 *    "reject" decisions are stored in-memory till the next restart.)
 * 3. Show the dialog to the user and allow/reject the permission based on the
 *    result.
 */
GSC.Pcsc.PolicyOrPromptingPermissionsChecker =
    class extends GSC.Pcsc.PermissionsChecker {
  constructor() {
    super();
    /** @private @const */
    this.managedRegistry_ = new PermissionsChecking.ManagedRegistry;
    /** @private @const */
    this.userPromptingChecker_ = new PermissionsChecking.UserPromptingChecker;
  }

  /**
   * @override
   */
  check(clientOrigin) {
    goog.log.log(
        logger, goog.log.Level.FINER,
        'Checking permissions for client ' + GSC.DebugDump.dump(clientOrigin) +
            '...');

    if (clientOrigin === null) {
      goog.log.log(
          logger, goog.log.Level.FINER,
          'Granted permissions for client with null origin');
      return goog.Promise.resolve();
    }

    const checkPromiseResolver = goog.Promise.withResolver();

    this.checkByManagedRegistry_(clientOrigin, checkPromiseResolver);

    return checkPromiseResolver.promise;
  }

  /**
   * @param {string} clientOrigin
   * @param {!goog.promise.Resolver} checkPromiseResolver
   * @private
   */
  checkByManagedRegistry_(clientOrigin, checkPromiseResolver) {
    goog.log.log(
        logger, goog.log.Level.FINER,
        'Checking permissions for the client ' + clientOrigin +
            ' through the managed registry...');

    this.managedRegistry_.getByOrigin(clientOrigin)
        .then(
            function() {
              goog.log.log(
                  logger, goog.log.Level.FINER,
                  'Granted permissions for client ' + clientOrigin +
                      ' through the managed registry');
              checkPromiseResolver.resolve();
            },
            function() {
              goog.log.log(
                  logger, goog.log.Level.FINER,
                  'No permissions found for client ' + clientOrigin +
                      ' through the managed registry');
              this.checkByUserPromptingChecker_(
                  clientOrigin, checkPromiseResolver);
            },
            this);
  }

  /**
   * @param {string} clientOrigin
   * @param {!goog.promise.Resolver} checkPromiseResolver
   * @private
   */
  checkByUserPromptingChecker_(clientOrigin, checkPromiseResolver) {
    this.userPromptingChecker_.check(clientOrigin)
        .then(checkPromiseResolver.resolve, checkPromiseResolver.reject);
  }
};
});  // goog.scope
