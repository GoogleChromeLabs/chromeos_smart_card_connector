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

goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientInfo');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistry');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.UserPromptingChecker');
goog.require('GoogleSmartCard.Pcsc.PolicyOrPromptingPermissionsChecker');
goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.asserts');
goog.require('goog.promise.Resolver');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const PolicyOrPromptingPermissionsChecker =
    GSC.Pcsc.PolicyOrPromptingPermissionsChecker;
const TrustedClientInfo = GSC.PcscLiteServer.TrustedClientInfo;
const UserPromptingChecker = GSC.PcscLiteServerClientsManagement
                                 .PermissionsChecking.UserPromptingChecker;

const ignoreArgument = goog.testing.mockmatchers.ignoreArgument;

const LOCAL_STORAGE_KEY = 'pcsc_lite_clients_user_selections';
const MANAGED_STORAGE_KEY = 'force_allowed_client_app_ids';
const MANAGED_STORAGE_AREA_NAME = 'managed';

const CLIENT_EXTENSION_ID = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
const CLIENT_ORIGIN = `chrome-extension://${CLIENT_EXTENSION_ID}`;
const CLIENT_NAME = 'Application';
const TRUSTED_CLIENT_INFO =
    new TrustedClientInfo(CLIENT_ORIGIN, CLIENT_NAME, /* autoapprove= */ false);

/** @implements {GSC.PcscLiteServer.TrustedClientsRegistry} @constructor */
function FakeTrustedClientsRegistry() {}

/** @override */
FakeTrustedClientsRegistry.prototype.getByOrigin = function(origin) {
  if (origin === CLIENT_ORIGIN)
    return goog.Promise.resolve(TRUSTED_CLIENT_INFO);
  return goog.Promise.reject();
};

/** @override */
FakeTrustedClientsRegistry.prototype.tryGetByOrigins = function(unused) {
  fail('Unexpected tryGetByOrigins() call');
  goog.asserts.fail();
};

/** @type {!Array<function(!Object, string)>} */
const onStorageChangedListeners = [];

/**
 * Sets up mocks for get() and addListener() methods of the chrome.storage API.
 * @param {!goog.testing.MockControl} mockControl
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 */
function setUpChromeStorageMock(mockControl, propertyReplacer) {
  propertyReplacer.set(chrome, 'storage', {
    'local': {
      'get': mockControl.createFunctionMock('chrome.storage.local.get'),
    },
    'managed': {
      'get': mockControl.createFunctionMock('chrome.storage.managed.get'),
    },
    'onChanged': {
      'addListener': (listener) => {
        onStorageChangedListeners.push(listener);
      },
    }
  });

  // Suppress compiler's signature verifications locally, to be able to use
  // mock-specific functionality.
  /** @type {?} */ chrome.storage.local.get;
  /** @type {?} */ chrome.storage.managed.get;

  chrome.storage.local.get(LOCAL_STORAGE_KEY, ignoreArgument)
      .$does(function(key, callback) {
        callback({});
      });
  chrome.storage.managed.get(MANAGED_STORAGE_KEY, ignoreArgument)
      .$does(function(key, callback) {
        callback({[MANAGED_STORAGE_KEY]: []});
      });
}

/**
 * Sets up UserPromptingChecker for testing with a fake TrustedClientsRegistry
 * and a fake implementation for the user prompt dialog.
 * @param {!Function} runModalDialogFake
 */
function setUpUserPromptingChecker(runModalDialogFake) {
  UserPromptingChecker.overrideTrustedClientsRegistryForTesting(
      new FakeTrustedClientsRegistry());
  UserPromptingChecker.overrideModalDialogRunnerForTesting(runModalDialogFake);
}

/**
 * Runs a permissions check test for the given client origin. In this test
 * setup, the managed and local storage are initially empty. When the user
 * prompt dialog would be shown, it will run a fake implementation that triggers
 * an onStorageChanged event with the given changes simulating the case where
 * the storage changes while waiting for the user's response. After the event
 * listeners are notified, the test will reject the dialog promise simulating
 * the dialog being closed by the user without granting permissions. This means
 * that if the storage change should not affect the permissions check result,
 * |expectPermissionsGranted| should be false.
 * @param {string} clientOrigin The origin for which to run the permissions
 *     check test.
 * @param {!Object} storageChanges Object with newValue and oldValue properties
 *     describing the change.
 * @param {string} storageAreaName Name of the storage area that changed.
 * @param {boolean} expectPermissionsGranted The expected result indicating
 *     whether permissions for |clientOrigin| were granted or not.
 * @return {function():!goog.Thenable} The wrapped test function, which returns
 *     a promise of the test result and the result of cleanup.
 */
function runOnStorageChangedTest(
    clientOrigin, storageChanges, storageAreaName, expectPermissionsGranted) {
  return function() {
    const mockControl = new goog.testing.MockControl();
    const propertyReplacer = new goog.testing.PropertyReplacer();

    function setup() {
      setUpChromeStorageMock(mockControl, propertyReplacer);

      // Set up a fake method that simulates emitting a chrome.storage.onChanged
      // event with the given changes when `GSC.PopupOpener.runModalDialog()`
      // gets called.
      const runModalDialogFake = function() {
        for (const listener of onStorageChangedListeners)
          listener(storageChanges, storageAreaName);
        return goog.Promise.reject();
      };
      setUpUserPromptingChecker(runModalDialogFake);

      mockControl.$replayAll();
    }

    function cleanup() {
      mockControl.$tearDown();
      mockControl.$resetAll();
      onStorageChangedListeners.length = 0;
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

    setup();

    let testPromise;
    /** @preserveTry */
    try {
      const permissionsChecker = new PolicyOrPromptingPermissionsChecker();
      const permissionsCheckPromise = permissionsChecker.check(clientOrigin);
      const resolve = function() {
        // Resolve the test promise - by simply returning from this callback.
      };
      const reject = function() {
        fail('Unexpected permissions check response');
      };
      testPromise = expectPermissionsGranted ?
          permissionsCheckPromise.then(resolve, reject) :
          permissionsCheckPromise.then(reject, resolve);
    } catch (exc) {
      // Cleanup after the test fatally failed synchronously.
      cleanup();
      throw exc;
    }
    // Verify mocks and cleanup after the test completes or fails.
    return testPromise.then(verifyAndCleanup, verifyAndCleanup);
  };
}

// Regression test for issue #41.
// Test that the user prompt is automatically closed and permissions are granted
// when permissions are added to the managed storage.
goog.exportSymbol(
    'testPolicyOrPromptingPermissionChecker_ApprovedOnManagedStorageChange',
    runOnStorageChangedTest(
        CLIENT_ORIGIN, {
          [MANAGED_STORAGE_KEY]:
              {'oldValue': [], 'newValue': [CLIENT_EXTENSION_ID]}
        } /* changes */,
        MANAGED_STORAGE_AREA_NAME /* storageAreaName */,
        true /* expectPermissionsGranted */));

// Regression test for issue #41.
// Test that permissions are not granted on unrelated policy changes.
goog.exportSymbol(
    'testPolicyOrPromptingPermissionChecker_UnrelatedOriginChange',
    runOnStorageChangedTest(
        CLIENT_ORIGIN, {
          [MANAGED_STORAGE_KEY]:
              {'oldValue': [], 'newValue': ['bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb']}
        } /* storageChanges */,
        MANAGED_STORAGE_AREA_NAME /* storageAreaName */,
        false /* expectPermissionsGranted */));

// Regression test for issue #41.
// Test that permissions are not granted on unrelated storage changes.
goog.exportSymbol(
    'testPolicyOrPromptingPermissionChecker_UnrelatedStorageChange',
    runOnStorageChangedTest(
      CLIENT_ORIGIN,
        {'foo': {'oldValue': ['bar'], 'newValue': []}} /* storageChanges */,
        'local' /* storageAreaName */, false /* expectPermissionsGranted */));
});  // goog.scope
