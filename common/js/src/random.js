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
 * @fileoverview This file contains helper definitions for performing random
 * operations.
 */

goog.provide('GoogleSmartCard.Random');

goog.require('goog.array');

goog.scope(function() {

/** @const */
var RANDOM_INTEGER_BYTE_COUNT = 6;

/** @const */
var GSC = GoogleSmartCard;

/**
 * Generates a cryptographically random integer number.
 * @return {number}
 */
GSC.Random.randomIntegerNumber = function() {
  var randomBytes = new Uint8Array(RANDOM_INTEGER_BYTE_COUNT);
  window.crypto.getRandomValues(randomBytes);
  var result = 0;
  goog.array.forEach(randomBytes, function(byteValue) {
    result = result * 256 + byteValue;
  });
  return result;
};

});  // goog.scope
