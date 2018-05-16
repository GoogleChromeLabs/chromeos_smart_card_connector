/** @license
 * Copyright 2018 Google Inc. All Rights Reserved.
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

goog.provide('GoogleSmartCard.ContainerHelpers');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.array');
goog.require('goog.object');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * Converts the given Map to an Object.
 * @param {!Map} map
 * @return {!Object}
 */
GSC.ContainerHelpers.buildObjectFromMap = function(map) {
  var keyValuePairArray = Array.from(map.entries());
  GSC.Logging.check(keyValuePairArray.length == map.size);
  var flatKeysAndValuesArray = goog.array.flatten(keyValuePairArray);
  GSC.Logging.check(flatKeysAndValuesArray.length == map.size * 2);
  var object = goog.object.create(flatKeysAndValuesArray);
  GSC.Logging.check(goog.object.getCount(object) == map.size);
  return object;
};

});  // goog.scope
