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

goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.KnownApp');
goog.provide('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.KnownAppsRegistry');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Json');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.log.Logger');
goog.require('goog.net.XhrIo');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

const KNOWN_CLIENT_APPS_JSON_URL =
    'pcsc_lite_server_clients_management/known_client_apps.json';

const KNOWN_CLIENT_APP_NAME_FIELD = 'name';

const GSC = GoogleSmartCard;

const PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;

/**
 * This structure holds the information about the known client app.
 * @param {string} id
 * @param {string} name
 * @constructor
 */
PermissionsChecking.KnownApp = function(id, name) {
  this.id = id;
  this.name = name;
};

const KnownApp = PermissionsChecking.KnownApp;

/**
 * This class provides an interface for querying the information about the known
 * client apps (i.e., the apps that are listed in the manually maintained
 * //third_party/pcsc-lite/naclport/server_clients_management/src/known_client_apps.json
 * config).
 * @constructor
 */
PermissionsChecking.KnownAppsRegistry = function() {
  /**
   * @type {!goog.promise.Resolver.<!Map.<string,!KnownApp>>}
   * @private
   */
  this.promiseResolver_ = goog.Promise.withResolver();

  this.startLoadingJson_();
};

const KnownAppsRegistry = PermissionsChecking.KnownAppsRegistry;

/**
 * @type {!goog.log.Logger}
 * @const
 */
KnownAppsRegistry.prototype.logger = GSC.Logging.getScopedLogger(
    'PcscLiteServerClientsManagement.PermissionsChecking.KnownAppsRegistry');

/**
 * Requests information about the given app from the config that contains the
 * list of known client apps.
 *
 * The result is returned asynchronously as a promise (which will be rejected if
 * the given app isn't present in the config).
 * @param {string} id
 * @return {!goog.Promise.<!KnownApp>}
 */
KnownAppsRegistry.prototype.getById = function(id) {
  const promiseResolver = goog.Promise.withResolver();

  this.tryGetByIds([id]).then(
      function(knownApps) {
        const knownApp = knownApps[0];
        if (knownApp) {
          promiseResolver.resolve(knownApp);
        } else {
          promiseResolver.reject(new Error(
              'The specified App id is not in the known Apps registry'));
        }
      },
      function(error) {
        promiseResolver.reject(error);
      });

  return promiseResolver.promise;
};

/**
 * Similar to getById(), but performs a batch request for the list of given IDs
 * at once.
 *
 * The result is returned asynchronously as a promise that, when resolved, will
 * contain the same number of elements as |idList|, with the i-th value
 * containing either the information for the i-th app in |idList| or |null| if
 * the app isn't present in the config.
 * @param {!Array.<string>} idList
 * @return {!goog.Promise.<!Array.<?KnownApp>>}
 */
KnownAppsRegistry.prototype.tryGetByIds = function(idList) {
  const promiseResolver = goog.Promise.withResolver();

  this.promiseResolver_.promise.then(
      function(knownAppsMap) {
        const knownApps = [];
        for (let id of idList) {
          const knownApp = knownAppsMap.get(id);
          knownApps.push(knownApp !== undefined ? knownApp : null);
        }
        promiseResolver.resolve(knownApps);
      },
      function() {
        promiseResolver.reject(
            new Error('Failed to load the known Apps registry'));
      });

  return promiseResolver.promise;
};

/** @private */
KnownAppsRegistry.prototype.startLoadingJson_ = function() {
  this.logger.fine(
      'Loading registry from JSON file (URL: "' + KNOWN_CLIENT_APPS_JSON_URL +
      '")...');
  goog.net.XhrIo.send(
      KNOWN_CLIENT_APPS_JSON_URL, this.jsonLoadedCallback_.bind(this));
};

/** @private */
KnownAppsRegistry.prototype.jsonLoadedCallback_ = function(e) {
  /** @type {!goog.net.XhrIo} */
  const xhrio = e.target;

  if (!xhrio.isSuccess()) {
    this.promiseResolver_.reject(
        new Error('Failed to load the known Apps registry'));
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to load the known Apps registry from JSON file (URL: "' +
            KNOWN_CLIENT_APPS_JSON_URL +
            '") with the following error: ' + xhrio.getLastError());
  }

  // Note: not using the xhrio.getResponseJson method as it breaks in the Chrome
  // App environment (for details, see the GSC.Json.parse method).
  GSC.Json.parse(xhrio.getResponseText())
      .then(
          function(json) {
            GSC.Logging.checkWithLogger(this.logger, goog.isObject(json));
            goog.asserts.assertObject(json);
            this.parseJsonAndApply_(json);
          },
          function(exc) {
            this.promiseResolver_.reject(
                new Error('Failed to load the known Apps registry'));
            GSC.Logging.failWithLogger(
                this.logger,
                'Failed to load the known Apps registry from JSON file (URL: "' +
                    KNOWN_CLIENT_APPS_JSON_URL +
                    '"): parsing failed with the ' +
                    'following error: ' + exc);
          },
          this);
};

/**
 * @param {!Object} json
 * @private
 */
KnownAppsRegistry.prototype.parseJsonAndApply_ = function(json) {
  /** @type {!Map.<string, !KnownApp>} */
  const knownClientApps = new Map;
  let success = true;
  goog.object.forEach(json, function(value, key) {
    const knownApp = this.tryParseKnownAppJson_(key, value);
    if (knownApp) {
      knownClientApps.set(knownApp.id, knownApp);
    } else {
      this.logger.warning(
          'Failed to parse the following known Apps registry JSON item: key="' +
          key + '", value=' + GSC.DebugDump.dump(value));
      success = false;
    }
  }, this);

  if (success) {
    this.logger.fine(
        'Successfully loaded registry from JSON file: ' +
        GSC.DebugDump.dump(knownClientApps));
    this.promiseResolver_.resolve(knownClientApps);
  } else {
    this.promiseResolver_.reject(
        new Error('Failed to load the known Apps registry'));
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to load the known Apps registry from JSON file (URL: "' +
            KNOWN_CLIENT_APPS_JSON_URL + '"): parsing of some of the items ' +
            'finished with errors');
  }
};

/**
 * @param {string} key
 * @param {*} value
 * @return {KnownApp?}
 * @private
 */
KnownAppsRegistry.prototype.tryParseKnownAppJson_ = function(key, value) {
  if (!goog.isObject(value))
    return null;

  if (!goog.object.containsKey(value, KNOWN_CLIENT_APP_NAME_FIELD))
    return null;
  const nameField = value[KNOWN_CLIENT_APP_NAME_FIELD];
  if (typeof nameField !== 'string')
    return null;

  return new KnownApp(key, nameField);
};
});  // goog.scope
