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

goog.provide('GoogleSmartCard.PcscLiteServer.TrustedClientInfo');
goog.provide('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistry');
goog.provide('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistryImpl');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.asserts');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

// The actual data is loaded from this JSON config.
const JSON_CONFIG_URL =
    'pcsc_lite_server_clients_management/known_client_apps.json';

const CLIENT_NAME_FIELD = 'name';
const AUTOAPPROVE_FIELD = 'autoapprove';

const GSC = GoogleSmartCard;

/**
 * Test-only override of the `fetch()` API.
 * @type {(function(string):!Promise<!Response>)|null}
 */
let fetchOverrideForTesting = null;

/**
 * This structure holds the information about a trusted client.
 * @param {string} origin
 * @param {string} name
 * @param {boolean} autoapprove
 * @constructor
 */
GSC.PcscLiteServer.TrustedClientInfo = function(origin, name, autoapprove) {
  /** @const */
  this.origin = origin;
  /** @const */
  this.name = name;
  /** @const */
  this.autoapprove = autoapprove;
};

const TrustedClientInfo = GSC.PcscLiteServer.TrustedClientInfo;

/**
 * This class provides an interface for querying the information about trusted
 * client applications (i.e., the clients that are listed in the manually
 * maintained
 * //third_party/pcsc-lite/naclport/server_clients_management/src/known_client_apps.json
 * config).
 * @interface
 */
GSC.PcscLiteServer.TrustedClientsRegistry = function() {};

const TrustedClientsRegistry = GSC.PcscLiteServer.TrustedClientsRegistry;

/**
 * Requests information about the given client from the config that contains the
 * list of known clients.
 *
 * The result is returned asynchronously as a promise (which will be rejected if
 * the given app isn't present in the config).
 * @param {string} origin
 * @return {!Promise<!TrustedClientInfo>}
 */
TrustedClientsRegistry.prototype.getByOrigin = function(origin) {};

/**
 * Similar to getByOrigin(), but performs a batch request for the list of given
 * origins at once.
 *
 * The result is returned asynchronously as a promise that, when resolved, will
 * contain the same number of elements as |originList|, with the i-th value
 * containing either the information for the i-th client in |originList| or
 * |null| if the client isn't present in the config.
 * @param {!Array.<string>} originList
 * @return {!Promise<!Array.<?TrustedClientInfo>>}
 */
TrustedClientsRegistry.prototype.tryGetByOrigins = function(originList) {};

/**
 * This class provides an interface for querying the information about trusted
 * client applications (i.e., the clients that are listed in the manually
 * maintained
 * //third_party/pcsc-lite/naclport/server_clients_management/src/known_client_apps.json
 * config).
 * @implements {TrustedClientsRegistry}
 * @constructor
 */
GSC.PcscLiteServer.TrustedClientsRegistryImpl = function() {
  // Start the async operation to load the config. Remember the promise, so that
  // all methods that need the config can await on this single load operation.
  /** @type {!Promise.<!Map<string,!TrustedClientInfo>>} @private @const */
  this.configPromise_ = this.loadConfigAndLog_();
};

const TrustedClientsRegistryImpl =
    GSC.PcscLiteServer.TrustedClientsRegistryImpl;

/**
 * @type {!goog.log.Logger}
 * @const
 */
TrustedClientsRegistryImpl.prototype.logger =
    GSC.Logging.getScopedLogger('PcscLiteServer.TrustedClientsRegistry');

/** @override */
TrustedClientsRegistryImpl.prototype.getByOrigin = async function(origin) {
  // Let `tryGetByOrigins()` do the actual work.
  const infos = await this.tryGetByOrigins([origin]);
  GSC.Logging.checkWithLogger(this.logger, infos.length === 1);
  const info = infos[0];
  if (!info) {
    throw new Error(
        'The specified origin is not in the trusted clients registry');
  }
  return info;
};

/** @override */
TrustedClientsRegistryImpl.prototype.tryGetByOrigins =
    async function(originList) {
  // Wait for the config load operation to complete, if needed.
  const config = await this.configPromise_;

  const infos = [];
  for (const origin of originList) {
    const info = config.get(origin);
    infos.push(info !== undefined ? info : null);
  }
  return infos;
};

/**
 * @param {(function(string):!Promise<!Response>)|null} fetchOverride
 */
TrustedClientsRegistryImpl.overrideFetchForTesting = function(fetchOverride) {
  fetchOverrideForTesting = fetchOverride;
};

/**
 * @return {!Promise<!Map<string,!TrustedClientInfo>>}
 * @private
 */
TrustedClientsRegistryImpl.prototype.loadConfigAndLog_ = async function() {
  goog.log.fine(
      this.logger,
      `Loading registry from JSON file (URL: "${JSON_CONFIG_URL}")...`);
  let config;
  try {
    config = await this.loadConfig_();
  } catch (e) {
    goog.log.error(
        this.logger,
        `Failed to load the registry (URL: "${JSON_CONFIG_URL}"): ${e}`);
    throw new Error('Failed to load the trusted client registry');
  }
  goog.log.fine(
      this.logger,
      `Successfully loaded registry from JSON file: ${
          GSC.DebugDump.debugDumpFull(config)}`);
  return config;
};

/**
 * @return {!Promise<!Map<string,!TrustedClientInfo>>}
 * @private
 */
TrustedClientsRegistryImpl.prototype.loadConfig_ = async function() {
  const fetcher = fetchOverrideForTesting || fetch;

  const response = await fetcher(JSON_CONFIG_URL);
  if (!response.ok)
    throw new Error(`Fetching failed: ${response.statusText}`);

  const json = await response.json();
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(json));
  goog.asserts.assertObject(json);
  return parseConfig(json);
};

/**
 * @param {!Object} json
 * @return {!Map<string,!TrustedClientInfo>}
 */
function parseConfig(json) {
  /** @type {!Map.<string, !TrustedClientInfo>} */
  const originToInfoMap = new Map();
  goog.object.forEach(json, (value, key) => {
    let info;
    try {
      info = parseTrustedClientInfo(key, value);
    } catch (e) {
      throw new Error(
          `Failed to parse key=${key} value=${JSON.stringify(value)}: ${e}`);
    }
    originToInfoMap.set(info.origin, info);
  });
  return originToInfoMap;
}

/**
 * @param {string} key
 * @param {*} value
 * @return {!TrustedClientInfo}
 */
function parseTrustedClientInfo(key, value) {
  if (!goog.isObject(value))
    throw new Error('Not an object');

  if (!goog.object.containsKey(value, CLIENT_NAME_FIELD))
    throw new Error(`No ${CLIENT_NAME_FIELD} key`);
  const nameField = value[CLIENT_NAME_FIELD];
  if (typeof nameField !== 'string')
    throw new Error(`The ${CLIENT_NAME_FIELD} value is not a string`);

  let autoapproveField = value[AUTOAPPROVE_FIELD];
  if (typeof autoapproveField === 'undefined')
    autoapproveField = false;
  else if (typeof autoapproveField !== 'boolean')
    throw new Error(`The ${AUTOAPPROVE_FIELD} value is not a boolean`);

  return new TrustedClientInfo(key, nameField, autoapproveField);
}
});  // goog.scope
