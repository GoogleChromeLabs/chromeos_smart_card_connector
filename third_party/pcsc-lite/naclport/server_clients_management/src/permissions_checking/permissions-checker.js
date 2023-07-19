/**
 * @license
 * Copyright 2023 Google Inc.
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

goog.provide('GoogleSmartCard.Pcsc.PermissionsChecker');

goog.require('goog.Promise');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Provides an interface for performing permission checks for the given client
 * applications, which gates access to sending PC/SC requests to our Smart Card
 * Connector.
 * @abstract
 * @constructor
 */
GSC.Pcsc.PermissionsChecker = class {
  /**
   * Starts the permission check for the given client application.
   *
   * The result is returned asynchronously as a promise (which will be
   * eventually resolved if the permission is granted or rejected otherwise).
   * @param {string|null} clientOrigin Origin of the client application, or null
   * if the client is our own application.
   * @return {!goog.Promise}
   * @abstract
   */
  check(clientOrigin) {}
};
});  // goog.scope
