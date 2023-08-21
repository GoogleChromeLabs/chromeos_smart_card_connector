/**
 * @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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
 * @fileoverview Functionality for detecting crash loops. Intended to be an
 * implementation detail of GoogleSmartCard.Logging.fail() and similar
 * functions.
 */

goog.provide('GoogleSmartCard.Logging.CrashLoopDetection');

goog.scope(function() {

const GSC = GoogleSmartCard;

// Name of the chrome.storage key that is used to track the recent crash
// timestamps. The value is an array of timestamps (as integer milliseconds).
const STORAGE_KEY = 'crash_timestamps';

// The time window, during which the crashes are counted and compared against
// |CRASH_LOOP_THRESHOLD_COUNT|.
const CRASH_LOOP_WINDOW_MILLISECONDS = 60 * 1000;
// If the number of crashes reaches this constant within the
// |CRASH_LOOP_WINDOW_MILLISECONDS| time window, then it's considered to be a
// crash loop.
//
// Note that |CRASH_LOOP_WINDOW_MILLISECONDS| and |CRASH_LOOP_THRESHOLD_COUNT|
// have to be chosen in a way that the crash loop is detected earlier than
// Chrome starts to throttle us, since that would leave the application in a
// state without any useful logs.
//
// As of 2021-06-07, two subsystems in Chrome have to be taken into account as
// follows:
// 1. Chrome's Extension system disables the extension when it reloads too
//    frequently, with the criteria currently being "5 reloads, each within 10
//    seconds after the launch":
//    <https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/extensions/api/runtime/chrome_runtime_api_delegate.cc;l=172-173;drc=d17745f350d4956a07cb1113ee19e9cbc4be699f>.
// 2. Chrome's Native Client system disables the module when it crashes too
//    frequently, with the criteria currently being "forbid subsequent load
//    attempts after 3 crashes within 2 minutes":
//    <https://source.chromium.org/chromium/chromium/src/+/main:components/nacl/browser/nacl_browser.cc;l=113-114;drc=d17745f350d4956a07cb1113ee19e9cbc4be699f>.
//
// We're choosing the largest possible value that satisfies these restrictions
// in order to reduce the risk of false positives, i.e., 3. Note that despite
// this value is exactly equal to restriction#2, it's still fine, since
// throttling breaks subsequent load attempts, which is exactly what we avoid.
const CRASH_LOOP_THRESHOLD_COUNT = 3;

/** @type {boolean} */
let crashing = false;

/**
 * Acknowledges that the extension is about to crash. Asynchronously returns the
 * boolean reporting whether a crash loop is detected. The returned promise is
 * rejected if this function was already called during the current run.
 * @return {!Promise<boolean>}
 */
GSC.Logging.CrashLoopDetection.handleImminentCrash = function() {
  if (crashing)
    return Promise.reject(new Error('Already crashing'));
  crashing = true;

  return loadRecentCrashTimestamps()
      .then(recentCrashTimestamps => {
        return storeNewCrashTimestamps(recentCrashTimestamps);
      })
      .then(newCrashTimestamps => {
        const isInCrashLoop =
            newCrashTimestamps.length >= CRASH_LOOP_THRESHOLD_COUNT;
        return Promise.resolve(isInCrashLoop);
      });
};

/**
 * Resets the in-memory state, allowing to call handleImminentCrash() multiple
 * times across different tests.
 * @package
 */
GSC.Logging.CrashLoopDetection.resetForTesting = function() {
  crashing = false;
};

/**
 * Loads the recent crash timestamps from the persistent storage.
 * @return {!Promise<!Array<number>>}
 */
function loadRecentCrashTimestamps() {
  // Note that none of the error handling below throws assertions or emits
  // logs, since this code is triggered from handling another, and more severe,
  // error.
  if (!chrome || !chrome.storage || !chrome.storage.local) {
    // Cannot load any data without access to the chrome.storage API (most
    // likely due to a missing permission).
    return Promise.resolve([]);
  }
  return new Promise(resolve => {
    chrome.storage.local.get(STORAGE_KEY, function(loadedStorage) {
      if (chrome.runtime.lastError ||
          !Object.prototype.hasOwnProperty.call(loadedStorage, STORAGE_KEY) ||
          !Array.isArray(loadedStorage[STORAGE_KEY])) {
        resolve([]);
        return;
      }
      resolve(loadedStorage[STORAGE_KEY].filter(item => {
        return Number.isInteger(item);
      }));
    });
  });
}

/**
 * Generates a new crash timestamps list (together with the current timestamp),
 * stores it in the persistent storage and returns it.
 * @param {!Array<number>} recentCrashTimestamps
 * @return {!Promise<!Array<number>>}
 */
function storeNewCrashTimestamps(recentCrashTimestamps) {
  const currentTimestamp = Date.now();
  const minTimestampToKeep = currentTimestamp - CRASH_LOOP_WINDOW_MILLISECONDS;
  let newCrashTimestamps = recentCrashTimestamps.filter(crashTimestamp => {
    return minTimestampToKeep <= crashTimestamp &&
        crashTimestamp <= currentTimestamp;
  });
  // Sanitization: Drop extra items beyond the |CRASH_LOOP_THRESHOLD_COUNT-1|
  // ones, in order to avoid any risk of the storage growing beyond all bounds.
  if (newCrashTimestamps.length >= CRASH_LOOP_THRESHOLD_COUNT) {
    newCrashTimestamps =
        newCrashTimestamps.slice(-CRASH_LOOP_THRESHOLD_COUNT + 1);
  }
  newCrashTimestamps.push(currentTimestamp);

  if (!chrome || !chrome.storage || !chrome.storage.local) {
    // Cannot store any data without access to the chrome.storage API.
    return Promise.resolve(newCrashTimestamps);
  }
  const dataToStore = {};
  dataToStore[STORAGE_KEY] = newCrashTimestamps;
  return new Promise(resolve => {
    chrome.storage.local.set(dataToStore, function() {
      if (chrome.runtime.lastError) {
        // Ignore the errors (note that we still need this branch, since Chrome
        // would otherwise generate an error about unchecked lastError).
        ignoreChromeRuntimeLastError();
      }
      // Note: It's important to do this after chrome.storage.local.set()
      // reports completion, so that our caller doesn't try to reload the
      // extension before the new crash data is actually stored.
      resolve(newCrashTimestamps);
    });
  });
}

function ignoreChromeRuntimeLastError() {}
});  // goog.scope
