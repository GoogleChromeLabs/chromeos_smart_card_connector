/**
 * @license
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
 * @fileoverview This file contains common functionality for object
 * manipulations.
 */

goog.provide('GoogleSmartCard.ObjectHelpers');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const LOGGER = GSC.Logging.getScopedLogger('ObjectHelpers');

/**
 * Helper function which extracts key from object (first it makes sure the key
 * is actually present).
 * @param {!Object} object
 * @param {string} key
 * @return {?}
 */
GSC.ObjectHelpers.extractKey = function(object, key) {
  if (!goog.object.containsKey(object, key)) {
    GSC.Logging.failWithLogger(
        LOGGER,
        'Key "' + key + '" not present in object ' +
            GSC.DebugDump.debugDumpSanitized(object));
  }

  return object[key];
};
});  // goog.scope
