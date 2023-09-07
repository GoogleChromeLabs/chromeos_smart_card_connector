/**
 * @license
 * Copyright 2021 Google Inc. All Rights Reserved.
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

goog.provide('GoogleSmartCard.ExtensionId');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Returns whether the given string's format corresponds to the one of the
 * Extension IDs. This function does NOT guarantee that there's a real-world
 * extension with this ID.
 * @param {string} string
 * @return {boolean}
 */
GSC.ExtensionId.looksLikeExtensionId = function(string) {
  return string.length === 32 && !!string.match(/[a-p]/i);
};
});  // goog.scope
