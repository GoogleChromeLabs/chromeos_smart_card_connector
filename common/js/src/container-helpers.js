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

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * Converts the given Map to an Object.
 *
 * The Map must have only string or numeric keys.
 * @param {!Map} map
 * @return {!Object}
 */
GSC.ContainerHelpers.buildObjectFromMap = function(map) {
  let obj = {};
  for (let [key, value] of map) {
    GSC.Logging.check(typeof key === 'string' || typeof key === 'number',
                      'Invalid type for object key');
    obj[key] = value;
  }
  return obj;
};

});  // goog.scope
