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

goog.require('GoogleSmartCard.Logging.CrashLoopDetection');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const CrashLoopDetection = GSC.Logging.CrashLoopDetection;

const SOME_DATE = new Date(
    /*year=*/ 2001, /*monthIndex=*/ 2, /*day=*/ 3, /*hours=*/ 4, /*minutes=*/ 5,
    /*seconds=*/ 6, /*milliseconds=*/ 7);

const propertyReplacer = new goog.testing.PropertyReplacer();

/** @type {!Date} */
let currentDate = SOME_DATE;

/** @type {!Object} */
let currentStorage = {};
/** @type {boolean} */
let simulateStorageGetFailure = false;
/** @type {boolean} */
let simulateStorageSetFailure = false;

/**
 * Stub implementation of the Date.now() JavaScript API.
 * @return {number}
 */
function stubDateNow() {
  return currentDate.getTime();
}

/**
 * Stub implementation of the chrome.storage.local.get() Extensions API.
 * @param {string} key
 * @param {function(!Object=)} callback
 */
function stubChromeStorageLocalGet(key, callback) {
  if (simulateStorageGetFailure) {
    propertyReplacer.set(
        chrome, 'runtime', {'lastError': {'message': 'Error'}});
    callback();
    propertyReplacer.set(chrome, 'runtime', {'lastError': undefined});
    return;
  }
  const resultItems = {};
  if (Object.prototype.hasOwnProperty.call(currentStorage, key))
    resultItems[key] = currentStorage[key];
  callback(resultItems);
}

/**
 * Stub implementation of the chrome.storage.local.set() Extensions API.
 * @param {!Object} items
 * @param {function()} callback
 */
function stubChromeStorageLocalSet(items, callback) {
  if (simulateStorageSetFailure) {
    propertyReplacer.set(
        chrome, 'runtime', {'lastError': {'message': 'Error'}});
    callback();
    propertyReplacer.set(chrome, 'runtime', {'lastError': undefined});
    return;
  }
  for (const key in items)
    currentStorage[key] = items[key];
  callback();
}

goog.exportSymbol('testCrashLoopDetection', {
  'setUp': function() {
    CrashLoopDetection.resetForTesting();
    currentDate = SOME_DATE;
    currentStorage = {};
    propertyReplacer.set(Date, 'now', stubDateNow);
    propertyReplacer.set(chrome, 'storage', {
      'local':
          {'get': stubChromeStorageLocalGet, 'set': stubChromeStorageLocalSet}
    });
    propertyReplacer.set(chrome, 'runtime', {'lastError': undefined});
  },

  'tearDown': function() {
    propertyReplacer.reset();
    CrashLoopDetection.resetForTesting();
  },

  // Test the scenario when a crash happens for the first time.
  'testSingleCrash': function() {
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertFalse(isInCrashLoop);
      assertObjectEquals(
          {'crash_timestamps': [SOME_DATE.getTime()]}, currentStorage);
    });
  },

  // Test that calls after the first one within a single run are rejected.
  'testExtraCalls': function() {
    return CrashLoopDetection.handleImminentCrash().then(() => {
      return CrashLoopDetection.handleImminentCrash().then(() => {
        fail('Unexpected successful result');
      }, () => {});
    });
  },

  // Test the scenario when a crash happens a sufficiently long time since the
  // previous crash, so that the old crash information is discarded.
  'testDelayedSecondCrash': function() {
    const ONE_HOUR_AGO = SOME_DATE.getTime() - 60 * 60 * 1000;
    currentStorage = {'crash_timestamps': [ONE_HOUR_AGO]};
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertFalse(isInCrashLoop);
      assertObjectEquals(
          {'crash_timestamps': [SOME_DATE.getTime()]}, currentStorage);
    });
  },

  // Test the scenario when a crash happens quickly after the previous one, so
  // that information about both crashes is preserved.
  'testQuickSecondCrash': function() {
    const ONE_SECOND_AGO = SOME_DATE.getTime() - 1000;
    currentStorage = {'crash_timestamps': [ONE_SECOND_AGO]};
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertFalse(isInCrashLoop);
      assertObjectEquals(
          {'crash_timestamps': [ONE_SECOND_AGO, SOME_DATE.getTime()]},
          currentStorage);
    });
  },

  // Test the crash loop scenario, where the storage contains information about
  // a big number of recent crashes that were happening every millisecond.
  'testCrashLoop': function() {
    const RECENT_CRASH_COUNT = 5;
    const recentCrashTimestamps = [];
    for (let delta = -RECENT_CRASH_COUNT; delta < 0; ++delta)
      recentCrashTimestamps.push(SOME_DATE.getTime() + delta);
    currentStorage = {'crash_timestamps': recentCrashTimestamps};
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertTrue(isInCrashLoop);
    });
  },

  // Test that the crash loop is detected before Chrome decides to disable the
  // extension due to too many reloads. Unlike the "testCrashLoop" test, this
  // test simulates slower timing that corresponds to the restrictions hardcoded
  // in Chrome (which, as of 2020-08-13, are "5 reloads, each within 10 seconds
  // after the launch").
  'testCrashLoopNotHitsExtensionThreshold': function() {
    const TOTAL_CRASH_COUNT = 5;
    const CRASH_INTERVAL = 10 * 1000;
    const recentCrashTimestamps = [];
    for (let crashIndex = 0; crashIndex < TOTAL_CRASH_COUNT - 1; ++crashIndex) {
      const intervalsAgoCount = TOTAL_CRASH_COUNT - 1 - crashIndex;
      recentCrashTimestamps.push(
          SOME_DATE.getTime() - CRASH_INTERVAL * intervalsAgoCount);
    }
    currentStorage = {'crash_timestamps': recentCrashTimestamps};
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertTrue(isInCrashLoop);
    });
  },

  // Test that the crash loop is detected before Chrome decides to disable the
  // NaCl module due to too many crashes. Unlike the "testCrashLoop" test, this
  // test simulates slower timing that corresponds to the restrictions hardcoded
  // in Chrome (which, as of 2021-06-07, are "3 crashes within 120 seconds").
  'testCrashLoopNotHitsNaClThreshold': function() {
    const TOTAL_CRASH_COUNT = 3;
    const CRASH_INTERVAL = 30 * 1000;
    const recentCrashTimestamps = [];
    for (let crashIndex = 0; crashIndex < TOTAL_CRASH_COUNT - 1; ++crashIndex) {
      const intervalsAgoCount = TOTAL_CRASH_COUNT - 1 - crashIndex;
      recentCrashTimestamps.push(
          SOME_DATE.getTime() - CRASH_INTERVAL * intervalsAgoCount);
    }
    currentStorage = {'crash_timestamps': recentCrashTimestamps};
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertTrue(isInCrashLoop);
    });
  },

  // Test the case when the chrome.storage API is not available (e.g., due to a
  // missing permission) - the implementation should still not break in this
  // case (although no crash loop detection is possible).
  'testStorageApiMissing': function() {
    propertyReplacer.set(chrome, 'storage', undefined);
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertFalse(isInCrashLoop);
    });
  },

  // Test the case when the storage is prepopulated with unexpected data - the
  // implementation should still not break.
  'testStorageCorrupted': function() {
    currentStorage = {'crash_timestamps': 'foo'};
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertFalse(isInCrashLoop);
      // Bad data got overwritten in the storage.
      assertObjectEquals(
          {'crash_timestamps': [SOME_DATE.getTime()]}, currentStorage);
    });
  },

  // Test the case when the chrome.storage.local.get() method fails - the
  // implementation should still not break.
  'testStorageGetError': function() {
    simulateStorageGetFailure = true;
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertFalse(isInCrashLoop);
    });
  },

  // Test the case when the chrome.storage.local.set() method fails - the
  // implementation should still not break.
  'testStorageSetError': function() {
    simulateStorageSetFailure = true;
    return CrashLoopDetection.handleImminentCrash().then((isInCrashLoop) => {
      assertFalse(isInCrashLoop);
    });
  },
});
});  // goog.scope
