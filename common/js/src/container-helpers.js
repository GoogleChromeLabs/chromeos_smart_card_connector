/**
 * @license
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
goog.require('goog.functions');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;

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
    GSC.Logging.check(
        typeof key === 'string' || typeof key === 'number',
        'Invalid type for object key');
    obj[key] = value;
  }
  return obj;
};

/**
 * Recursively visits the given value and replaces all ArrayBuffer objects with
 * their Array views of bytes. Returns the resulting value after substitutions.
 * @param {?} value
 * @return {?}
 */
GSC.ContainerHelpers.substituteArrayBuffersRecursively = function(value) {
  const substituteArrayBuffersRecursively =
      GSC.ContainerHelpers.substituteArrayBuffersRecursively;
  if (value instanceof ArrayBuffer) {
    // Convert the array buffer into an array of bytes.
    return Array.from(new Uint8Array(value));
  }
  if (Array.isArray(value)) {
    // Recursively process array items.
    return value.map(substituteArrayBuffersRecursively);
  }
  if (goog.isObject(value) && !goog.functions.isFunction(value) &&
      !ArrayBuffer.isView(value)) {
    // This is a dictionary-like object; process it recursively.
    return goog.object.map(value, substituteArrayBuffersRecursively);
  }
  return value;
};
});  // goog.scope
