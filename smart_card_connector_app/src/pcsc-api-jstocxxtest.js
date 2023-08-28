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
 * @fileoverview This file contains tests for the PC/SC API exposed by Smart
 * Card Connector.
 */

goog.require('GoogleSmartCard.IntegrationTestController');
goog.require('GoogleSmartCard.LibusbProxyReceiver');
goog.require('GoogleSmartCard.MessageChannelPair');
goog.require('GoogleSmartCard.Pcsc.PermissionsChecker');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.TestingLibusbSmartCardSimulationHook');
goog.require('goog.Promise');
goog.require('goog.testing');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;
const ReadinessTracker = GSC.PcscLiteServerClientsManagement.ReadinessTracker;
const API = GSC.PcscLiteClient.API;

/**
 * Stub that approves any client to make PC/SC calls.
 */
class StubPermissionsChecker extends GSC.Pcsc.PermissionsChecker {
  /** @override */
  check(clientOrigin) {
    return goog.Promise.resolve();
  }
}

/**
 * @typedef {{apiMessageChannelPair: !GSC.MessageChannelPair, clientHandler:
 * !ClientHandler, api: !API}}
 */
let FakeClient;

/** @type {GSC.IntegrationTestController?} */
let testController;
/** @type {GSC.LibusbProxyReceiver?} */
let libusbProxyReceiver;
/** @type {ReadinessTracker?} */
let pcscReadinessTracker;
const stubPermissionsChecker = new StubPermissionsChecker();
/** @type {FakeClient?} */
let client;

/**
 * Shorthand for obtaining the PC/SC context.
 * @return {!Promise<!API.SCARDCONTEXT>}
 */
async function establishContextOrThrow() {
  const result = await client.api.SCardEstablishContext(
      API.SCARD_SCOPE_SYSTEM, null, null);
  let sCardContext;
  result.get(
      (context) => {
        sCardContext = context;
      },
      (errorCode) => {
        fail(`Unexpected error ${errorCode}`);
      });
  return sCardContext;
}

/**
 * @param {!Array} initialDevices
 * @return {!Promise}
 */
async function launchPcscServer(initialDevices) {
  await testController.setUpCppHelper(
      'SmartCardConnectorApplicationTestHelper', initialDevices);
}

/**
 * @return {!FakeClient}
 */
function createFakeClient() {
  const apiMessageChannelPair = new GSC.MessageChannelPair();
  const clientHandler = new ClientHandler(
      testController.executableModule.getMessageChannel(),
      pcscReadinessTracker.promise, apiMessageChannelPair.getFirst(),
      /*clientOrigin=*/ undefined);
  const api = new API(apiMessageChannelPair.getSecond());
  return {apiMessageChannelPair, clientHandler, api};
}

/**
 * @param {!FakeClient} fakeClient
 */
function disposeFakeClient(fakeClient) {
  fakeClient.api.dispose();
  fakeClient.clientHandler.dispose();
  fakeClient.apiMessageChannelPair.dispose();
}

goog.exportSymbol('testPcscApi', {
  'setUp': async function() {
    // Set up the controller and load the C/C++ executable module.
    testController = new GSC.IntegrationTestController();
    await testController.initAsync();
    // Stub out necessary globals.
    ClientHandler.overridePermissionsCheckerForTesting(stubPermissionsChecker);
    libusbProxyReceiver = new GSC.LibusbProxyReceiver(
        testController.executableModule.getMessageChannel());
    libusbProxyReceiver.addHook(new GSC.TestingLibusbSmartCardSimulationHook(
        testController.executableModule.getMessageChannel()));
    // Set up observers.
    pcscReadinessTracker = new ReadinessTracker(
        testController.executableModule.getMessageChannel());
  },

  'tearDown': async function() {
    try {
      await testController.disposeAsync();
      pcscReadinessTracker.dispose();
    } finally {
      pcscReadinessTracker = null;
      ClientHandler.overridePermissionsCheckerForTesting(null);
      testController = null;
    }
  },

  // Test that the PC/SC server can successfully start up.
  'testStartup': async function() {
    launchPcscServer(/*initialDevices=*/[]);
    await pcscReadinessTracker.promise;
  },

  // A group of tests that simulate a single client making PC/SC calls.
  'testWithSingleClient': {
    'setUp': function() {
      client = createFakeClient();
    },

    'tearDown': function() {
      if (client) {
        disposeFakeClient(client);
        client = null;
      }
    },

    // Test `SCardEstablishContext()`.
    'testSCardEstablishContext': async function() {
      await launchPcscServer(/*initialDevices=*/[]);

      const result = await client.api.SCardEstablishContext(
          API.SCARD_SCOPE_SYSTEM, null, null);
      let sCardContext;
      result.get(
          (context) => {
            sCardContext = context;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assert(Number.isInteger(sCardContext));
      assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
    },

    // Test `SCardEstablishContext()` when it's called without providing
    // optional arguments.
    'testSCardEstablishContext_omittedOptionalArgs': async function() {
      await launchPcscServer(/*initialDevices=*/[]);

      const result =
          await client.api.SCardEstablishContext(API.SCARD_SCOPE_SYSTEM);
      let sCardContext;
      result.get(
          (context) => {
            sCardContext = context;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assert(Number.isInteger(sCardContext));
      assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
    },

    // Test `SCardReleaseContext()` with the correct handle.
    'testSCardReleaseContext_correct': async function() {
      await launchPcscServer(/*initialDevices=*/[]);
      const context = await establishContextOrThrow();

      const result = await client.api.SCardReleaseContext(context);
      let called = false;
      result.get(
          () => {
            called = true;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assert(called);
      assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
    },

    // Test `SCardReleaseContext()` fails on a wrong handle when there's no
    // established handle at all.
    'testSCardReleaseContext_none': async function() {
      const BAD_CONTEXT = 123;
      await launchPcscServer(/*initialDevices=*/[]);

      const result = await client.api.SCardReleaseContext(BAD_CONTEXT);
      let called = false;
      result.get(
          () => {
            fail('Unexpectedly succeeded');
          },
          (errorCode) => {
            called = true;
            assertEquals(errorCode, API.SCARD_E_INVALID_HANDLE);
          });
      assert(called);
      assertEquals(result.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
    },

    // Test `SCardReleaseContext()` fails on a wrong handle when there's
    // another established handle.
    'testSCardReleaseContext_different': async function() {
      await launchPcscServer(/*initialDevices=*/[]);
      const context = await establishContextOrThrow();
      const badContext = context ^ 1;

      const result = await client.api.SCardReleaseContext(badContext);
      let called = false;
      result.get(
          () => {
            fail('Unexpectedly succeeded');
          },
          (errorCode) => {
            called = true;
            assertEquals(errorCode, API.SCARD_E_INVALID_HANDLE);
          });
      assert(called);
      assertEquals(result.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
    },

    // Test `SCardIsValidContext()` with the correct handle.
    'testSCardIsValidContext_correct': async function() {
      await launchPcscServer(/*initialDevices=*/[]);
      const context = await establishContextOrThrow();

      const result = await client.api.SCardIsValidContext(context);
      let called = false;
      result.get(
          () => {
            called = true;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assert(called);
      assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
    },

    // Test `SCardIsValidContext()` errors out on a wrong handle when there's no
    // established handle at all.
    'testSCardIsValidContext_none': async function() {
      const BAD_CONTEXT = 123;
      await launchPcscServer(/*initialDevices=*/[]);

      const result = await client.api.SCardIsValidContext(BAD_CONTEXT);
      let called = false;
      result.get(
          () => {
            fail('Unexpectedly succeeded');
          },
          (errorCode) => {
            called = true;
            assertEquals(errorCode, API.SCARD_E_INVALID_HANDLE);
          });
      assert(called);
      assertEquals(result.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
    },

    // Test `SCardIsValidContext()` errors out on a wrong handle when there's
    // another established handle.
    'testSCardIsValidContext_different': async function() {
      await launchPcscServer(/*initialDevices=*/[]);
      const context = await establishContextOrThrow();
      const badContext = context ^ 1;

      const result = await client.api.SCardIsValidContext(badContext);
      let called = false;
      result.get(
          () => {
            fail('Unexpectedly succeeded');
          },
          (errorCode) => {
            called = true;
            assertEquals(errorCode, API.SCARD_E_INVALID_HANDLE);
          });
      assert(called);
      assertEquals(result.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
    },

    // Test `SCardListReaders()` returns a specific error code when there's no
    // device attached.
    'testSCardListReaders_none': async function() {
      await launchPcscServer(/*initialDevices=*/[]);
      const context = await establishContextOrThrow();

      const result =
          await client.api.SCardListReaders(context, /*groups=*/ null);
      let called = false;
      result.get(
          (readersArg) => {
            fail('Unexpectedly succeeded');
          },
          (errorCode) => {
            called = true;
            assertEquals(errorCode, API.SCARD_E_NO_READERS_AVAILABLE);
          });
      assert(called);
      assertEquals(result.getErrorCode(), API.SCARD_E_NO_READERS_AVAILABLE);
    },

    // Test `SCardListReaders()` returns a one-item list when there's a single
    // attached device.
    'testSCardListReaders_singleDevice': async function() {
      await launchPcscServer(
          /*initialDevices=*/[{'id': 123, 'type': 'gemaltoPcTwinReader'}]);
      const context = await establishContextOrThrow();

      const result =
          await client.api.SCardListReaders(context, /*groups=*/ null);
      let readers = null;
      result.get(
          (readersArg) => {
            readers = readersArg;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assertObjectEquals(readers, ['Gemalto PC Twin Reader 00 00']);
      assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
    }
  },

  // Test that the PC/SC server can shut down successfully when there's an
  // active client.
  'testShutdownWithActiveClient': async function() {
    launchPcscServer(/*initialDevices=*/[]);
    createFakeClient();
    await pcscReadinessTracker.promise;
    // Intentionally don't dispose the client.
  },

  // Same as above, but the client additionally made an API call.
  // This verifies any lazily created internal state doesn't cause problems.
  'testShutdownWithActiveClientAfterApiCall': async function() {
    const BAD_CONTEXT = 123;
    launchPcscServer(/*initialDevices=*/[]);
    const localClient = createFakeClient();
    await localClient.api.SCardIsValidContext(BAD_CONTEXT);
    // Intentionally don't dispose the client.
  },
});
});  // goog.scope
