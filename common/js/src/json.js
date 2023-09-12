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

goog.provide('GoogleSmartCard.Json');

goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.json.NativeJsonProcessor');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Parses a JSON string and returns the result or the error message through the
 * returned promise.
 *
 * This is a replacement of the goog.json.parse method, as it breaks in the
 * Chrome App environment (it tries to use eval, which is not allowed in
 * packaged Chrome Apps).
 * @param {string} jsonString
 * @return {!goog.Thenable.<*>}
 */
GSC.Json.parse = function(jsonString) {
  const processor = new goog.json.NativeJsonProcessor();
  /** @preserveTry */
  try {
    return goog.Promise.resolve(processor.parse(jsonString));
  } catch (exc) {
    return goog.Promise.reject(exc);
  }
};
});  // goog.scope
