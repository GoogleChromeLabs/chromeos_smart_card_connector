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

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Json');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.net.XhrIo');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

// The actual data is loaded from this JSON config.
const JSON_CONFIG_URL =
    'pcsc_lite_server_clients_management/known_client_apps.json';

const CLIENT_NAME_FIELD = 'name';

const GSC = GoogleSmartCard;

/**
 * This structure holds the information about a trusted client.
 * @param {string} origin
 * @param {string} name
 * @constructor
 */
GSC.PcscLiteServer.TrustedClientInfo = function(origin, name) {
  /** @const */
  this.origin = origin;
  /** @const */
  this.name = name;
};

const TrustedClientInfo = GSC.PcscLiteServer.TrustedClientInfo;

/**
 * This class provides an interface for querying the information about trusted
 * client applications (i.e., the clients that are listed in the manually
 * maintained
 * //third_party/pcsc-lite/naclport/server_clients_management/src/known_client_apps.json
 * config).
 * @constructor
 */
GSC.PcscLiteServer.TrustedClientsRegistry = function() {
  /**
   * @type {!goog.promise.Resolver.<!Map.<string,!TrustedClientInfo>>} @private @const
   */
  this.promiseResolver_ = goog.Promise.withResolver();

  this.startLoadingJson_();
};

const TrustedClientsRegistry = GSC.PcscLiteServer.TrustedClientsRegistry;

/**
 * @type {!goog.log.Logger}
 * @const
 */
TrustedClientsRegistry.prototype.logger =
    GSC.Logging.getScopedLogger('PcscLiteServer.TrustedClientsRegistry');

/**
 * Requests information about the given client from the config that contains the
 * list of known clients.
 *
 * The result is returned asynchronously as a promise (which will be rejected if
 * the given app isn't present in the config).
 * @param {string} origin
 * @return {!goog.Promise.<!TrustedClientInfo>}
 */
TrustedClientsRegistry.prototype.getByOrigin = function(origin) {
  const promiseResolver = goog.Promise.withResolver();

  this.tryGetByOrigins([origin]).then(
      infos => {
        GSC.Logging.checkWithLogger(this.logger, infos.length === 1);
        const info = infos[0];
        if (info) {
          promiseResolver.resolve(info);
        } else {
          promiseResolver.reject(new Error(
              'The specified origin is not in the trusted clients registry'));
        }
      },
      function(error) {
        promiseResolver.reject(error);
      });

  return promiseResolver.promise;
};

/**
 * Similar to getByOrigin(), but performs a batch request for the list of given
 * origins at once.
 *
 * The result is returned asynchronously as a promise that, when resolved, will
 * contain the same number of elements as |originList|, with the i-th value
 * containing either the information for the i-th client in |originList| or
 * |null| if the client isn't present in the config.
 * @param {!Array.<string>} originList
 * @return {!goog.Promise.<!Array.<?TrustedClientInfo>>}
 */
TrustedClientsRegistry.prototype.tryGetByOrigins = function(originList) {
  const promiseResolver = goog.Promise.withResolver();

  this.promiseResolver_.promise.then(
      function(originToInfoMap) {
        const infos = [];
        for (const origin of originList) {
          const info = originToInfoMap.get(origin);
          infos.push(info !== undefined ? info : null);
        }
        promiseResolver.resolve(infos);
      },
      function() {
        promiseResolver.reject(
            new Error('Failed to load the trusted clients registry'));
      });

  return promiseResolver.promise;
};

/** @private */
TrustedClientsRegistry.prototype.startLoadingJson_ = function() {
  goog.log.fine(
      this.logger,
      'Loading registry from JSON file (URL: "' + JSON_CONFIG_URL + '")...');
  goog.net.XhrIo.send(JSON_CONFIG_URL, this.jsonLoadedCallback_.bind(this));
};

/** @private */
TrustedClientsRegistry.prototype.jsonLoadedCallback_ = function(e) {
  /** @type {!goog.net.XhrIo} */
  const xhrio = e.target;

  if (!xhrio.isSuccess()) {
    this.promiseResolver_.reject(
        new Error('Failed to load the trusted clients registry'));
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to load the trusted clients registry from JSON file (URL: "' +
            JSON_CONFIG_URL +
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
                new Error('Failed to load the trusted clients registry'));
            GSC.Logging.failWithLogger(
                this.logger,
                'Failed to load the trusted clients registry from JSON file (URL: "' +
                    JSON_CONFIG_URL + '"): parsing failed with the ' +
                    'following error: ' + exc);
          },
          this);
};

/**
 * @param {!Object} json
 * @private
 */
TrustedClientsRegistry.prototype.parseJsonAndApply_ = function(json) {
  /** @type {!Map.<string, !TrustedClientInfo>} */
  const originToInfoMap = new Map;
  let success = true;
  goog.object.forEach(json, function(value, key) {
    const info = this.tryParseTrustedClientInfo_(key, value);
    if (info) {
      originToInfoMap.set(info.origin, info);
    } else {
      goog.log.warning(
          this.logger,
          'Failed to parse the following trusted clients registry JSON item: key="' +
              key + '", value=' + GSC.DebugDump.dump(value));
      success = false;
    }
  }, this);

  if (success) {
    goog.log.fine(
        this.logger,
        'Successfully loaded registry from JSON file: ' +
            GSC.DebugDump.dump(originToInfoMap));
    this.promiseResolver_.resolve(originToInfoMap);
  } else {
    this.promiseResolver_.reject(
        new Error('Failed to load the trusted clients registry'));
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to load the trusted clients registry from JSON file (URL: "' +
            JSON_CONFIG_URL + '"): parsing of some of the items ' +
            'finished with errors');
  }
};

/**
 * @param {string} key
 * @param {*} value
 * @return {TrustedClientInfo?}
 * @private
 */
TrustedClientsRegistry.prototype.tryParseTrustedClientInfo_ = function(
    key, value) {
  if (!goog.isObject(value))
    return null;

  if (!goog.object.containsKey(value, CLIENT_NAME_FIELD))
    return null;
  const nameField = value[CLIENT_NAME_FIELD];
  if (typeof nameField !== 'string')
    return null;

  return new TrustedClientInfo(key, nameField);
};
});  // goog.scope
