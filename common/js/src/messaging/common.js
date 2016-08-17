/** @license
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview This file contains common functionality for the provided
 * message channels.
 */

goog.provide('GoogleSmartCard.MessagingCommon');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var LOGGER = GSC.Logging.getScopedLogger('MessagingCommon');

/**
 * @param {!goog.messaging.AbstractChannel} channel
 */
GSC.MessagingCommon.setNonFatalDefaultServiceCallback = function(channel) {
  var boundCallback = nonFatalDefaultServiceCallback.bind(undefined, channel);
  channel.registerDefaultService(boundCallback);
};

/**
 * @param {!goog.messaging.AbstractChannel} channel
 * @param {string} serviceName
 * @param {!Object|string} payload
 */
function nonFatalDefaultServiceCallback(channel, serviceName, payload) {
  LOGGER.warning(
      'Unhandled message received: serviceName="' + serviceName +
      '", payload=' + GSC.DebugDump.debugDump(payload));
  channel.dispose();
}

/**
 * Helper function which extracts key from message (first it makes sure the key
 * is actually present).
 * @param {!Object|string} message
 * @param {string} key
 */
GSC.MessagingCommon.extractKey = function(message, key) {
  GSC.Logging.checkWithLogger(LOGGER, goog.isObject(message));
  goog.asserts.assert(goog.isObject(message));
  GSC.Logging.checkWithLogger(
      LOGGER,
      goog.object.containsKey(message, key),
      'Key "' + key + '" not present in message ' +
      GSC.DebugDump.dump(message));

  return message[key];
}

});  // goog.scope
