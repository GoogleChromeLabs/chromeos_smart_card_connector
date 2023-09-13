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

goog.provide('GoogleSmartCard.PromiseHelpers');

goog.require('goog.Thenable');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Disables for the specified promise throwing of an error when it's rejected
 * with no rejection callback attached to it.
 * @param {!goog.Thenable} promise
 */
GSC.PromiseHelpers.suppressUnhandledRejectionError = function(promise) {
  promise.thenCatch(function() {});
};
});  // goog.scope
