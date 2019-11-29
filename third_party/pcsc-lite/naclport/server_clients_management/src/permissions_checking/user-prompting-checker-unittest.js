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

goog.require('GoogleSmartCard.ObjectHelpers');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.KnownApp');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.KnownAppsRegistry');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');
goog.require('GoogleSmartCard.PopupWindow.Server');
goog.require('goog.Promise');
goog.require('goog.json');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;
/** @const */
var KnownApp = PermissionsChecking.KnownApp;
/** @const */
var KnownAppsRegistry = PermissionsChecking.KnownAppsRegistry;
/** @const */
var UserPromptingChecker = PermissionsChecking.UserPromptingChecker;
/** @const */
var ignoreArgument = goog.testing.mockmatchers.ignoreArgument;

/** @const */
var FAKE_APP_1_ID = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
/** @const */
var FAKE_APP_1_NAME = 'App Name 1';
/** @const */
var FAKE_APP_1_KNOWN_APP = new KnownApp(FAKE_APP_1_ID, FAKE_APP_1_NAME);
/** @const */
var FAKE_APP_2_ID = 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
/** @const */
var STORAGE_KEY = 'pcsc_lite_clients_user_selections';

/**
 * @enum {number}
 */
var MockedDialogBehavior = {
  NOT_RUN: 0,
  USER_APPROVES: 1,
  USER_DENIES: 2,
  USER_CANCELS: 3,
};

/**
 * Sets up a mock instance of KnownAppsRegistry.
 *
 * This mock instance is only aware of |FAKE_APP_1_ID|.
 * @param {!goog.testing.MockControl} mockControl
 */
function setUpKnownAppsRegistryMock(mockControl) {
  var mockInstance = {
    getById: function(id) {
      if (id == FAKE_APP_1_ID)
        return goog.Promise.resolve(FAKE_APP_1_KNOWN_APP);
      return goog.Promise.reject();
    }
  };
  /** @type {?} */
  var mockedConstructor = mockControl.createConstructorMock(
      PermissionsChecking, 'KnownAppsRegistry');
  mockedConstructor().$returns(mockInstance);
}

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
  propertyReplacer.set(
      chrome, 'storage',
      {local: {
         get: mockControl.createFunctionMock('chrome.storage.local.get'),
         set: mockControl.createFunctionMock('chrome.storage.local.set')}});

  // Suppress compiler's signature verifications locally, to be able to use
  // mock-specific functionality.
  /** @type {?} */ chrome.storage.local.get;
  /** @type {?} */ chrome.storage.local.set;

  chrome.storage.local.get(STORAGE_KEY, ignoreArgument).$does(
      function(key, callback) {
        callback(fakeInitialData);
      });

  if (expectedDataToBeWritten !== null)
    chrome.storage.local.set(expectedDataToBeWritten);
}

/**
 * Sets up mocks for the GSC.PopupWindow.Server.runModalDialog method.
 * @param {!goog.testing.MockControl} mockControl
 * @param {!MockedDialogBehavior} mockedBehavior If |NOT_RUN|, then the method
 * is not expected to be called; otherwise, the called method will return the
 * corresponding result.
 */
function setUpDialogMock(mockControl, mockedBehavior) {
  /** @type {?} */
  var mockedFunction = mockControl.createMethodMock(
      GSC.PopupWindow.Server, 'runModalDialog');
  var mockAction;
  switch (mockedBehavior) {
    case MockedDialogBehavior.NOT_RUN:
      return;
    case MockedDialogBehavior.USER_APPROVES:
      mockAction = function() {
        return goog.Promise.resolve(true);
      };
      break;
    case MockedDialogBehavior.USER_DENIES:
      mockAction = function() {
        return goog.Promise.resolve(false);
      };
      break;
    case MockedDialogBehavior.USER_CANCELS:
      mockAction = function() {
        return goog.Promise.reject();
      };
      break;
  }
  mockedFunction(ignoreArgument, ignoreArgument, ignoreArgument).$does(
      mockAction);
}

/**
 * Returns a negation of the given promise.
 * @param {!goog.Promise} promise
 * @return {!goog.Promise.<undefined>} A promise which is rejected when
 * |promise| gets resolved, and resolved (with no data) when |promise| is
 * rejected.
 */
function negatePromise(promise) {
  return promise.then(function() {
    fail('Unexpected successful response');
  }, function() {
    // Resolve the resulting promise - by simply returning from this callback.
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
 * GSC.PopupWindow.Server.runModalDialog method is not expected to be called;
 * otherwise, the called method will return the corresponding result.
 * @param {function(!UserPromptingChecker):!goog.Promise} testCallback The test
 * function to be run after the needed setup; must return a promise of the test
 * result.
 * @return {function():!goog.Promise} The wrapped test function, which returns a
 * promise of the test result and the result of teardown.
 */
function makeTest(
    fakeInitialStorageData, expectedStorageDataToBeWritten,
    mockedDialogBehavior, testCallback) {
  return function() {
    var mockControl = new goog.testing.MockControl;
    var propertyReplacer = new goog.testing.PropertyReplacer;

    function cleanup() {
      mockControl.$tearDown();
      mockControl.$resetAll();
      propertyReplacer.reset();
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

    setUpKnownAppsRegistryMock(mockControl);
    setUpChromeStorageMock(
        mockControl, propertyReplacer, fakeInitialStorageData,
        expectedStorageDataToBeWritten);
    setUpDialogMock(mockControl, mockedDialogBehavior);

    mockControl.$replayAll();

    /** @preserveTry */
    try {
      var testPromise = testCallback(new UserPromptingChecker);
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
    'test_UserPromptingChecker_EmptyStorage_UserApproves', makeTest(
      {}  /* fakeInitialStorageData */,
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true}}
          /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.USER_APPROVES  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return userPromptingChecker.check(FAKE_APP_1_ID);
      }));

goog.exportSymbol(
    'test_UserPromptingChecker_EmptyStorage_UserDenies', makeTest(
      {}  /* fakeInitialStorageData */,
      null  /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.USER_DENIES  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return negatePromise(userPromptingChecker.check(FAKE_APP_1_ID));
      }));

goog.exportSymbol(
    'test_UserPromptingChecker_EmptyStorage_UserCancels', makeTest(
      {}  /* fakeInitialStorageData */,
      null  /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.USER_CANCELS  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return negatePromise(userPromptingChecker.check(FAKE_APP_1_ID));
      }));

goog.exportSymbol(
    'test_UserPromptingChecker_AlreadyInStorage_OneItem', makeTest(
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true}}  /* fakeInitialStorageData */,
      null  /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.NOT_RUN  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return userPromptingChecker.check(FAKE_APP_1_ID);
      }));

goog.exportSymbol(
    'test_UserPromptingChecker_AlreadyInStorage_TwoItems', makeTest(
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true, [FAKE_APP_2_ID]: true}}
          /* fakeInitialStorageData */,
      null  /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.NOT_RUN  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return goog.Promise.all([
            userPromptingChecker.check(FAKE_APP_1_ID),
            userPromptingChecker.check(FAKE_APP_2_ID)]);
      }));

goog.exportSymbol(
    'test_UserPromptingChecker_OtherInStorage_UserApproves', makeTest(
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true}}  /* fakeInitialStorageData */,
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true, [FAKE_APP_2_ID] : true}}
          /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.USER_APPROVES  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return userPromptingChecker.check(FAKE_APP_2_ID);
      }));

// Regression test for issue #57.
goog.exportSymbol(
    'test_UserPromptingChecker_CorruptedStorage_NonObject', makeTest(
      {[STORAGE_KEY]: 'foo'}  /* fakeInitialStorageData */,
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true}}
          /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.USER_APPROVES  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return userPromptingChecker.check(FAKE_APP_1_ID);
      }));

// Regression test for issue #57.
goog.exportSymbol(
    'test_UserPromptingChecker_CorruptedStorage_BadItem', makeTest(
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: 'foo'}}  /* fakeInitialStorageData */,
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true}}
          /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.USER_APPROVES  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return userPromptingChecker.check(FAKE_APP_1_ID);
      }));

// Regression test for issue #57.
goog.exportSymbol(
    'test_UserPromptingChecker_CorruptedStorage_BadOtherItem', makeTest(
      {[STORAGE_KEY]: {[FAKE_APP_1_ID]: true, [FAKE_APP_2_ID]: 'foo'}}
          /* fakeInitialStorageData */,
      null  /* expectedStorageDataToBeWritten */,
      MockedDialogBehavior.NOT_RUN  /* mockedDialogBehavior */,
      function(userPromptingChecker) {
        return userPromptingChecker.check(FAKE_APP_1_ID);
      }));

});  // goog.scope
