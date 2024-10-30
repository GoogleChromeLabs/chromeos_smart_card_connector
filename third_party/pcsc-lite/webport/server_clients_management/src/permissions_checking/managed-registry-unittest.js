/**
 * @license
 * Copyright 2022 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.ManagedRegistry');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.StrictMock');
goog.require('goog.testing.mockmatchers');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ManagedRegistry =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking.ManagedRegistry;

const FIRST_EXTENSION_ID = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
const FIRST_ORIGIN = `chrome-extension://${FIRST_EXTENSION_ID}`;
const SECOND_EXTENSION_ID = 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
const SECOND_ORIGIN = `chrome-extension://${SECOND_EXTENSION_ID}`;

const propertyReplacer = new goog.testing.PropertyReplacer();
/** @type {!goog.testing.MockControl|undefined} */
let mockControl;
/** @type {!goog.testing.StrictMock|undefined} */
let mockedGetStorage;
/**
 * A list of listeners added via the mocked
 * chrome.storage.onChanged.addListener().
 * @type {!Array<function(!Object, string)>}
 */
const storageListeners = [];
/** @type {!ManagedRegistry|undefined} */
let managedRegistry;

/**
 * Configures the chrome.storage.managed.get() mock to respond with the
 * specified policies.
 * @param {!Array<string>|null} policies
 */
function willReturnPolicies(policies) {
  const items = {};
  if (policies !== null)
    items['force_allowed_client_app_ids'] = policies;

  // Cast the mock to the generic function type, because otherwise Closure
  // Compiler doesn't understand we're allowed to pass a mock matcher to this
  // function (neither does it understand that `mockedGetStorage` is callable).
  const callable = /** @type {!Function} */ (chrome.storage.managed.get);
  callable(
      'force_allowed_client_app_ids', goog.testing.mockmatchers.isFunction);
  mockedGetStorage.$does((key, callback) => {
    callback(items);
  });
}

/**
 * A test version of chrome.storage.onChanged.addListener().
 * @param {function(!Object, string)} listener
 */
function stubAddStorageListener(listener) {
  storageListeners.push(listener);
}

/**
 * Notifies all observers of chrome.storage.onChanged.addListener() about the
 * given changes.
 * @param {!Object} changes
 * @param {string} namespace
 */
function notifyOnChanged(changes, namespace) {
  for (const listener of storageListeners)
    listener(changes, namespace);
}

/**
 * Calls `ManagedStorage.getByOrigin()` and returns whether it succeeded or
 * failed.
 * @param {string} origin
 * @returns {!Promise<boolean>}
 */
async function booleanizedGetByOrigin(origin) {
  try {
    await managedRegistry.getByOrigin(origin);
    return true;
  } catch (exc) {
    return false;
  }
}

goog.exportSymbol('testPcscClientManagedRegistry', {
  'setUp': function() {
    mockControl = new goog.testing.MockControl();
    // Mock chrome.storage.managed.get(). Cast is to work around the issue that
    // the return type of createFunctionMock() is too generic (as it depends on
    // the value of the optional second argument).
    mockedGetStorage = /** @type {!goog.testing.StrictMock} */ (
        mockControl.createFunctionMock('storage.managed.get'));
    propertyReplacer.setPath('chrome.storage.managed.get', mockedGetStorage);
    // Mock chrome.storage.onChanged.addListener().
    propertyReplacer.setPath(
        'chrome.storage.onChanged.addListener', stubAddStorageListener);
  },

  'tearDown': function() {
    // Check all mock expectations are satisfied.
    mockControl.$verifyAll();
    // Clean up.
    storageListeners.length = 0;
    if (managedRegistry) {
      // TODO: Make the class disposable to ensure listeners are torn down.
      managedRegistry = undefined;
    }
    propertyReplacer.reset();
  },

  // Test the case when no policies are configured.
  'testNoPolicy': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns empty results.
    willReturnPolicies(null);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();

    // Assert: failures for both origins.
    assertFalse(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertFalse(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test the case when the policy contains the first extension ID.
  'testFirstExtensionId': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns the first extension ID.
    willReturnPolicies([FIRST_EXTENSION_ID]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();

    // Assert: success for the first origin.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertFalse(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test the case when the policy contains the first extension's origin.
  'testFirstOrigin': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns the origin that points to the test extension.
    willReturnPolicies([FIRST_ORIGIN]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();

    // Assert: success for the first origin.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertFalse(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test the case when the policy contains both extensions' IDs.
  'testTwoExtensionIds': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns two extension IDs.
    willReturnPolicies([FIRST_EXTENSION_ID, SECOND_EXTENSION_ID]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();

    // Assert: success for both origins.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertTrue(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test the case when the policy contains both extensions' origins.
  'testTwoOrigins': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns both extension's origins.
    willReturnPolicies([FIRST_ORIGIN, SECOND_ORIGIN]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();

    // Assert: success for both origins.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertTrue(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test the scenario of dynamically changing the policy from an empty state to
  // the state with the given extension.
  'testExtensionIdAdded': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns empty results.
    willReturnPolicies([]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();
    // Verify failure for both origins initially.
    assertFalse(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertFalse(await booleanizedGetByOrigin(SECOND_ORIGIN));

    // Notify the policy is changed to have the first extension ID.
    notifyOnChanged(
        {
          'force_allowed_client_app_ids':
              {'oldValue': [], 'newValue': [FIRST_EXTENSION_ID]}
        },
        'managed');

    // Assert: success for the first origin.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertFalse(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test the scenario of dynamically changing the policy from listing both
  // extension IDs to having only the second one.
  'testExtensionIdRemoved': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns two extension IDs.
    willReturnPolicies([SECOND_EXTENSION_ID, FIRST_EXTENSION_ID]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();
    // Verify success for both origins initially.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertTrue(await booleanizedGetByOrigin(SECOND_ORIGIN));

    // Act: Notify the policy is changed to have only the second extension ID.
    notifyOnChanged(
        {
          'force_allowed_client_app_ids': {
            'oldValue': [SECOND_EXTENSION_ID, FIRST_EXTENSION_ID],
            'newValue': [SECOND_EXTENSION_ID]
          }
        },
        'managed');

    // Assert: success for the second origin.
    assertFalse(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertTrue(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test that changes of unrelated keys in the managed storage doesn't affect
  // the result.
  'testUnrelatedPolicyChange': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns the first extension ID.
    willReturnPolicies([FIRST_EXTENSION_ID]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();
    // Verify success for the first origin initially.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertFalse(await booleanizedGetByOrigin(SECOND_ORIGIN));

    // Act: Notify some unrelated policy is changed.
    notifyOnChanged({'foo': {'oldValue': ['bar'], 'newValue': []}}, 'managed');

    // Assert: still success for the first origin.
    assertTrue(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertFalse(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },

  // Test that changes of (unrelated) keys in the local storage doesn't affect
  // the result.
  'testUnrelatedNamespaceChange': async function() {
    // Arrange: set up the mock expectation that chrome.storage.managed.get() is
    // called and returns the second extension ID.
    willReturnPolicies([SECOND_EXTENSION_ID]);
    mockControl.$replayAll();
    // Create the registry (has to happen after the function mocks are set up).
    managedRegistry = new ManagedRegistry();
    // Verify success for the second origin initially.
    assertFalse(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertTrue(await booleanizedGetByOrigin(SECOND_ORIGIN));

    // Act: Notify some key in the local storage is changed.
    notifyOnChanged({'foo': {'oldValue': ['bar'], 'newValue': []}}, 'local');

    // Assert: still success for the second origin.
    assertFalse(await booleanizedGetByOrigin(FIRST_ORIGIN));
    assertTrue(await booleanizedGetByOrigin(SECOND_ORIGIN));
  },
});
});  // goog.scope
