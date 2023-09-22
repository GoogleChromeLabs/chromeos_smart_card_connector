/**
 * @license
 * Copyright 2023 Google Inc. All Rights Reserved.
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
 * @fileoverview This file provides a helper for doing "await" on a Chrome
 * Extension API call.
 */

goog.provide('GoogleSmartCard.PromisifyExtensionApi');

goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Calls the API method and waits until its callback gets run.
 *
 * Note: This will become unnecessary once we fully migrate to Manifest v3,
 * which changes the Extension APIs to return promises themselves.
 * @param {!Object} apiThat Object on which the method should be called, e.g.,
 * `chrome.runtime`.
 * @param {!Function} apiMethod The method to call, e.g.,
 * `chrome.runtime.getBackgroundPage()`.
 * @param {...*} apiArguments The parameters to pass to the called method.
 * @return {!Promise} Resolved with the result passed to the callback, or
 * rejected with an error on failure.
 */
GSC.PromisifyExtensionApi.promisify = function(
    apiThat, apiMethod, ...apiArguments) {
  return new Promise((resolve, reject) => {
    apiMethod.call(apiThat, ...apiArguments, function(apiResult) {
      if (chrome && chrome.runtime && chrome.runtime.lastError) {
        const apiError = goog.object.get(
            chrome.runtime.lastError, 'message', 'Unknown error');
        reject(apiError);
        return;
      }
      resolve(apiResult);
    });
  });
};
});
