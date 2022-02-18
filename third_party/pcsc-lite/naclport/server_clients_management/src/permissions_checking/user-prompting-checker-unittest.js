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

goog.require('GoogleSmartCard.MessagingOrigin');
goog.require('GoogleSmartCard.ObjectHelpers');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientInfo');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistry');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');
goog.require('GoogleSmartCard.PopupOpener');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const TrustedClientInfo = GSC.PcscLiteServer.TrustedClientInfo;
const TrustedClientsRegistry = GSC.PcscLiteServer.TrustedClientsRegistry;
const UserPromptingChecker = GSC.PcscLiteServerClientsManagement
                                 .PermissionsChecking.UserPromptingChecker;
const ignoreArgument = goog.testing.mockmatchers.ignoreArgument;

// Fake client #1: trusted.
const FAKE_CLIENT_1_ORIGIN =
    'chrome-extension://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
const FAKE_CLIENT_1_LEGACY_STORAGE_ID = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
const FAKE_CLIENT_1_NAME = 'Application 1';
const FAKE_TRUSTED_CLIENT_INFO_1 = new TrustedClientInfo(
    FAKE_CLIENT_1_ORIGIN, FAKE_CLIENT_1_NAME, /* autoapprove= */ false);
// Fake client #2: untrusted.
const FAKE_CLIENT_2_ORIGIN =
    'chrome-extension://bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
const FAKE_CLIENT_2_LEGACY_STORAGE_ID = 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
// Fake client #3: trusted and autoapproved.
const FAKE_CLIENT_3_ORIGIN =
    'chrome-extension://autoapprovediiiiiiiiiiiiiiiiiiii';
const FAKE_CLIENT_3_NAME = 'Application 3';
const FAKE_TRUSTED_CLIENT_INFO_3 = new TrustedClientInfo(
    FAKE_CLIENT_3_ORIGIN, FAKE_CLIENT_3_NAME, /* autoapprove= */ true);

const STORAGE_KEY = 'pcsc_lite_clients_user_selections';

/**
 * @enum {number}
 */
const MockedDialogBehavior = {
  NOT_RUN: 0,
  USER_APPROVES: 1,
  USER_DENIES: 2,
  USER_CANCELS: 3,
};

/**
 * A fake implementation of `TrustedClientsRegistry`.
 *
 * It's only aware of `FAKE_CLIENT_1_ORIGIN`.
 * @implements {TrustedClientsRegistry}
 * @constructor
 */
function FakeTrustedClientsRegistry() {}

/** @override */
FakeTrustedClientsRegistry.prototype.getByOrigin = function(origin) {
  // Only fake clients ##1 and 3 are trusted.
  if (origin === FAKE_CLIENT_1_ORIGIN)
    return goog.Promise.resolve(FAKE_TRUSTED_CLIENT_INFO_1);
  if (origin === FAKE_CLIENT_3_ORIGIN)
    return goog.Promise.resolve(FAKE_TRUSTED_CLIENT_INFO_3);
  return goog.Promise.reject();
};

/** @override */
FakeTrustedClientsRegistry.prototype.tryGetByOrigins = function(originList) {
  fail('Unexpected tryGetByOrigins() call');
  goog.asserts.fail();
};

/**
 * Sets up mocks for get() and set() methods of the chrome.storage.local
 * Extensions API.
 * @param {!goog.testing.MockControl} mockControl
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 * @param {!Object} fakeInitialData Fake data to be returned from get().
 * @param {Object?} expectedDataToBeWritten If non-null, then the expected data
 * to be passed into set(); if null, then set() isn't expected to be called.
 */
function setUpChromeStorageMock(
    mockControl, propertyReplacer, fakeInitialData, expectedDataToBeWritten) {
  propertyReplacer.set(chrome, 'storage', {
    'local': {
      'get': mockControl.createFunctionMock('chrome.storage.local.get'),
      'set': mockControl.createFunctionMock('chrome.storage.local.set')
    }
  });

  // Suppress compiler's signature verifications locally, to be able to use
  // mock-specific functionality.
  /** @type {?} */ chrome.storage.local.get;
  /** @type {?} */ chrome.storage.local.set;

  chrome.storage.local.get(STORAGE_KEY, ignoreArgument)
      .$does(function(key, callback) {
        callback(fakeInitialData);
      });

  if (expectedDataToBeWritten !== null)
    chrome.storage.local.set(expectedDataToBeWritten);
}

/**
 * Returns a mocks for the `GSC.PopupOpener.runModalDialog()` method.
 * @param {!goog.testing.MockControl} mockControl
 * @param {!MockedDialogBehavior} mockedBehavior If |NOT_RUN|, then the method
 * is not expected to be called; otherwise, the called method will return the
 * corresponding result.
 * @return {!Function}
 */
function getDialogMock(mockControl, mockedBehavior) {
  switch (mockedBehavior) {
    case MockedDialogBehavior.NOT_RUN:
      return function() {
        fail(`Unexpected dialog request`);
      };
    case MockedDialogBehavior.USER_APPROVES:
      return function() {
        return goog.Promise.resolve(true);
      };
      break;
    case MockedDialogBehavior.USER_DENIES:
      return function() {
        return goog.Promise.resolve(false);
      };
      break;
    case MockedDialogBehavior.USER_CANCELS:
      return function() {
        return goog.Promise.reject();
      };
  }
  goog.asserts.fail();
}

/**
 * Returns a negation of the given promise.
 * @param {!Promise} promise
 * @return {!Promise.<undefined>} A promise which is rejected when
 * |promise| gets resolved, and resolved (with no data) when |promise| is
 * rejected.
 */
function negatePromise(promise) {
  return promise.then(
      function() {
        fail('Unexpected successful response');
      },
      function() {
        // Resolve the resulting promise - by simply returning from this
        // callback.
      });
}

/**
 * Wraps the given test function, providing the necessary setup and teardown.
 * @param {!Object} fakeInitialStorageData Fake data to be returned from
 * chrome.storage.local.get().
 * @param {Object?} expectedStorageDataToBeWritten If non-null, then the
 * expected data to be passed into chrome.storage.local.set(); if null, then
 * set() isn't expected to be called.
 * @param {!MockedDialogBehavior} mockedDialogBehavior If |NOT_RUN|, then the
 * GSC.PopupOpener.runModalDialog method is not expected to be called;
 * otherwise, the called method will return the corresponding result.
 * @param {function(!UserPromptingChecker):!Promise} testCallback The test
 * function to be run after the needed setup; must return a promise of the test
 * result.
 * @return {function():!goog.Promise} The wrapped test function, which returns a
 * promise of the test result and the result of teardown.
 */
function makeTest(
    fakeInitialStorageData, expectedStorageDataToBeWritten,
    mockedDialogBehavior, testCallback) {
  return function() {
    const mockControl = new goog.testing.MockControl;
    const propertyReplacer = new goog.testing.PropertyReplacer;

    function cleanup() {
      mockControl.$tearDown();
      mockControl.$resetAll();
      propertyReplacer.reset();
      UserPromptingChecker.overrideTrustedClientsRegistryForTesting(null);
      UserPromptingChecker.overrideModalDialogRunnerForTesting(null);
    }

    function verifyAndCleanup() {
      /** @preserveTry */
      try {
        mockControl.$verifyAll();
      } finally {
        // Cleanup regardless of the mock verification outcome.
        cleanup();
      }
    }

    setUpChromeStorageMock(
        mockControl, propertyReplacer, fakeInitialStorageData,
        expectedStorageDataToBeWritten);
    UserPromptingChecker.overrideTrustedClientsRegistryForTesting(
        new FakeTrustedClientsRegistry());
    UserPromptingChecker.overrideModalDialogRunnerForTesting(
        getDialogMock(mockControl, mockedDialogBehavior));

    mockControl.$replayAll();

    let testPromise;
    /** @preserveTry */
    try {
      testPromise = testCallback(new UserPromptingChecker);
    } catch (exc) {
      // Cleanup after the test fatally failed synchronously.
      cleanup();
      throw exc;
    }
    // Verify mocks and cleanup after the test completes or fails. Note that
    // it's crucial that the returned promise is a child promise which is tied
    // with the success of verification and cleanup.
    return testPromise.then(verifyAndCleanup, verifyAndCleanup);
  };
}

goog.exportSymbol(
    'test_UserPromptingChecker_EmptyStorage_UserApproves',
    makeTest(
        {} /* fakeInitialStorageData */, {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_ORIGIN]: true}
        } /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.USER_APPROVES /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN);
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_EmptyStorage_UserDenies',
    makeTest(
        {} /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.USER_DENIES /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return negatePromise(
              userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN));
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_EmptyStorage_UserCancels',
    makeTest(
        {} /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.USER_CANCELS /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return negatePromise(
              userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN));
        }));

// Test that for an autoapproved client the check completes without any prompts
// or previously stored decisions.
goog.exportSymbol(
    'test_UserPromptingChecker_EmptyStorage_AutoApproved',
    makeTest(
        {} /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.NOT_RUN /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_3_ORIGIN);
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_AlreadyInStorage_OneItem',
    makeTest(
        {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_ORIGIN]: true}
        } /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.NOT_RUN /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN);
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_AlreadyInStorage_OneItem_InLegacyFormat',
    makeTest(
        {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_LEGACY_STORAGE_ID]: true}
        } /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.NOT_RUN /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN);
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_AlreadyInStorage_TwoItems',
    makeTest(
        {
          [STORAGE_KEY]:
              {[FAKE_CLIENT_1_ORIGIN]: true, [FAKE_CLIENT_2_ORIGIN]: true}
        } /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.NOT_RUN /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return Promise.all([
            userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN),
            userPromptingChecker.check(FAKE_CLIENT_2_ORIGIN)
          ]);
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_AlreadyInStorage_TwoItems_InLegacyFormat',
    makeTest(
        {
          [STORAGE_KEY]: {
            [FAKE_CLIENT_1_LEGACY_STORAGE_ID]: true,
            [FAKE_CLIENT_2_LEGACY_STORAGE_ID]: true
          }
        } /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.NOT_RUN /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return Promise.all([
            userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN),
            userPromptingChecker.check(FAKE_CLIENT_2_ORIGIN)
          ]);
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_OtherInStorage_UserApproves',
    makeTest(
        {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_ORIGIN]: true}
        } /* fakeInitialStorageData */,
        {
          [STORAGE_KEY]:
              {[FAKE_CLIENT_1_ORIGIN]: true, [FAKE_CLIENT_2_ORIGIN]: true}
        } /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.USER_APPROVES /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_2_ORIGIN);
        }));

goog.exportSymbol(
    'test_UserPromptingChecker_OtherInStorage_InLegacyFormat_UserApproves',
    makeTest(
        {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_LEGACY_STORAGE_ID]: true}
        } /* fakeInitialStorageData */,
        {
          [STORAGE_KEY]:
              {[FAKE_CLIENT_1_ORIGIN]: true, [FAKE_CLIENT_2_ORIGIN]: true}
        } /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.USER_APPROVES /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_2_ORIGIN);
        }));

// Regression test for issue #57.
goog.exportSymbol(
    'test_UserPromptingChecker_CorruptedStorage_NonObject',
    makeTest(
        {[STORAGE_KEY]: 'foo'} /* fakeInitialStorageData */, {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_ORIGIN]: true}
        } /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.USER_APPROVES /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN);
        }));

// Regression test for issue #57.
goog.exportSymbol(
    'test_UserPromptingChecker_CorruptedStorage_BadItem',
    makeTest(
        {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_ORIGIN]: 'foo'}
        } /* fakeInitialStorageData */,
        {
          [STORAGE_KEY]: {[FAKE_CLIENT_1_ORIGIN]: true}
        } /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.USER_APPROVES /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN);
        }));

// Regression test for issue #57.
goog.exportSymbol(
    'test_UserPromptingChecker_CorruptedStorage_BadOtherItem',
    makeTest(
        {
          [STORAGE_KEY]:
              {[FAKE_CLIENT_1_ORIGIN]: true, [FAKE_CLIENT_2_ORIGIN]: 'foo'}
        } /* fakeInitialStorageData */,
        null /* expectedStorageDataToBeWritten */,
        MockedDialogBehavior.NOT_RUN /* mockedDialogBehavior */,
        function(userPromptingChecker) {
          return userPromptingChecker.check(FAKE_CLIENT_1_ORIGIN);
        }));
});  // goog.scope
