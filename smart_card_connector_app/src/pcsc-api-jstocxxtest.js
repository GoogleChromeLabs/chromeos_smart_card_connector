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

goog.require('GoogleSmartCard.AsyncAsserts');
goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.IntegrationTestController');
goog.require('GoogleSmartCard.LibusbProxyReceiver');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPair');
goog.require('GoogleSmartCard.Pcsc.PermissionsChecker');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTrackerThroughPcscServerHook');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.PromiseHelpers');
goog.require('GoogleSmartCard.RequesterMessage');
goog.require('GoogleSmartCard.TestingLibusbSmartCardSimulationHook');
goog.require('GoogleSmartCard.TestingLibusbSmartCardSimulationConstants');
goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.iter');
goog.require('goog.object');
goog.require('goog.testing');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;
const ReadinessTracker = GSC.PcscLiteServerClientsManagement.ReadinessTracker;
const API = GSC.PcscLiteClient.API;
const SimulationConstants = GSC.TestingLibusbSmartCardSimulationConstants;
const debugDumpFull = GSC.DebugDump.debugDumpFull;

const PCSC_REQUEST_MESSAGE_TYPE = GSC.RequesterMessage.getRequestMessageType(
    GSC.PcscLiteCommon.Constants.REQUESTER_TITLE);

const PNP_NOTIFICATION = GSC.PcscLiteCommon.Constants.PNP_NOTIFICATION;

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
        fail(`Unexpected SCardEstablishContext error ${errorCode}`);
      });
  return sCardContext;
}

/**
 * Shorthand for obtaining the PC/SC card handle.
 * @param {!API.SCARDCONTEXT} sCardContext
 * @param {string} readerName
 * @param {number} shareMode
 * @param {number} preferredProtocols
 * @param {number} assertResultProtocol
 * @return {!Promise<!API.SCARDHANDLE>}
 */
async function connectToReaderOrThrow(
    sCardContext, readerName, shareMode, preferredProtocols,
    assertResultProtocol) {
  const result = await client.api.SCardConnect(
      sCardContext, readerName, shareMode, preferredProtocols);
  let sCardHandle;
  result.get(
      (sCardHandleArg, activeProtocol) => {
        assert(Number.isInteger(sCardHandleArg));
        sCardHandle = sCardHandleArg;
        assertEquals(activeProtocol, assertResultProtocol);
      },
      (errorCode) => {
        fail(`Unexpected SCardConnect error ${errorCode}`);
      });
  return sCardHandle;
}

/**
 * @param {!Array} initialDevices
 * @return {!Promise}
 */
async function launchPcscServer(initialDevices) {
  // Set up listeners for internal reader events, to suppress spurious errors
  // about unexpected C++->JS messages. The object is not stored anywhere as it
  // manages its own lifetime itself.
  new GSC.PcscLiteServer.ReaderTrackerThroughPcscServerHook(
      GSC.Logging.getScopedLogger('ReaderTracker'),
      testController.executableModule.getMessageChannel(),
      /*updateListener=*/ () => {});

  await testController.setUpCppHelper(
      SimulationConstants.CPP_HELPER_NAME, initialDevices);
}

/**
 * @param {!Array} devices
 * @return {!Promise}
 */
async function setSimulatedDevices(devices) {
  await testController.sendMessageToCppHelper(
      SimulationConstants.CPP_HELPER_NAME, devices);
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

/**
 * @param {!goog.Thenable} promise
 * @param {number} timeoutMillis
 * @return {!Promise}
 */
async function assertRemainsPending(promise, timeoutMillis) {
  let sleeping = true;
  promise.then(
      () => {
        if (sleeping)
          fail(`Unexpectedly resolved within ${timeoutMillis} ms`);
      },
      () => {
        if (sleeping)
          fail(`Unexpectedly rejected within ${timeoutMillis} ms`);
      });
  await GSC.PromiseHelpers.sleep(timeoutMillis);
  sleeping = false;
}

/**
 * @param {!API.SCARD_READERSTATE_OUT} a
 * @param {!API.SCARD_READERSTATE_OUT} b
 * @return {boolean}
 */
function readerStateEquals(a, b) {
  // Note the "atr" fields cannot be compared via "===" as they're ArrayBuffers.
  return a['reader_name'] === b['reader_name'] &&
      a['current_state'] === b['current_state'] &&
      a['event_state'] === b['event_state'] &&
      goog.iter.equals(new Uint8Array(a['atr']), new Uint8Array(b['atr'])) &&
      a['user_data'] === b['user_data'];
}

/**
 * @param {!API.SCARD_READERSTATE_OUT} a
 * @param {!API.SCARD_READERSTATE_OUT} b
 */
function assertReaderStateEquals(a, b) {
  assertTrue(
      `${debugDumpFull(a)} != ${debugDumpFull(b)}`, readerStateEquals(a, b));
}

// Test cases exposed under the "testPcscApi" key.
const SIMPLE_TEST_CASES = {
  // Test that the PC/SC server can successfully start up.
  'testStartup': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    await pcscReadinessTracker.promise;
  },

  // Test that the PC/SC server can shut down successfully when there's an
  // active client.
  'testShutdownWithActiveClient': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    createFakeClient();
    await pcscReadinessTracker.promise;
    // Intentionally don't dispose the client.
  },

  // Same as above, but the client additionally made an API call.
  // This verifies any lazily created internal state doesn't cause problems.
  'testShutdownWithActiveClientAfterApiCall': async function() {
    const BAD_CONTEXT = 123;
    await launchPcscServer(/*initialDevices=*/[]);
    const localClient = createFakeClient();
    await localClient.api.SCardIsValidContext(BAD_CONTEXT);
    // Intentionally don't dispose the client.
  },
};

const TEST_CASES_WITH_SINGLE_CLIENT = {
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

    const result = await client.api.SCardListReaders(context, /*groups=*/ null);
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
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
        ]);
    const context = await establishContextOrThrow();

    const result = await client.api.SCardListReaders(context, /*groups=*/ null);
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
  },

  // Test `SCardListReaders()` returns two items when there're two attached
  // devices.
  'testSCardListReaders_twoDevices': async function() {
    await launchPcscServer(
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE},
          {'id': 124, 'type': SimulationConstants.DELL_DEVICE_TYPE},
        ]);
    const context = await establishContextOrThrow();

    const result = await client.api.SCardListReaders(context, /*groups=*/ null);
    let readers = null;
    result.get(
        (readersArg) => {
          readers = readersArg;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assertObjectEquals(readers, [
      SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
      'Dell Dell Smart Card Reader Keyboard 01 00'
    ]);
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardGetStatusChange()` detects when a reader is plugged in.
  'testSCardGetStatusChange_deviceAppearing': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    const context = await establishContextOrThrow();

    const resultPromise = client.api.SCardGetStatusChange(
        context, API.INFINITE, [new API.SCARD_READERSTATE_IN(
                                   PNP_NOTIFICATION, API.SCARD_STATE_UNAWARE)]);
    await setSimulatedDevices(
        [{'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}]);
    const result = await resultPromise;

    let readerStates = null;
    result.get(
        (readerStatesArg) => {
          readerStates = readerStatesArg;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assertEquals(readerStates.length, 1);
    assertReaderStateEquals(
        readerStates[0],
        new API.SCARD_READERSTATE_OUT(
            /*readerName=*/ PNP_NOTIFICATION,
            /*currentState=*/ API.SCARD_STATE_UNAWARE,
            /*eventState=*/ API.SCARD_STATE_CHANGED,
            /*atr=*/ new ArrayBuffer(0)));
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardGetStatusChange()` detects when a reader is unplugged.
  'testSCardGetStatusChange_deviceRemoving': async function() {
    await launchPcscServer(
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
        ]);
    const context = await establishContextOrThrow();

    const resultPromise = client.api.SCardGetStatusChange(
        context, API.INFINITE,
        [new API.SCARD_READERSTATE_IN(
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            API.SCARD_STATE_EMPTY)]);
    await setSimulatedDevices([]);
    const result = await resultPromise;

    let readerStates = null;
    result.get(
        (readerStatesArg) => {
          readerStates = readerStatesArg;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assertEquals(readerStates.length, 1);
    const readerState = readerStates[0];
    // Depending on the timing, PC/SC may or may not report the
    // `SCARD_STATE_UNKNOWN` flag (this depends on whether it already removed
    // the "dead" reader from internal lists by the time SCardGetStatusChange
    // is replied to).
    const expected1 = new API.SCARD_READERSTATE_OUT(
        SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        /*currentState=*/ API.SCARD_STATE_EMPTY,
        /*eventState=*/ API.SCARD_STATE_CHANGED | API.SCARD_STATE_UNKNOWN |
            API.SCARD_STATE_UNAVAILABLE,
        /*atr=*/ new ArrayBuffer(0));
    const expected2 = new API.SCARD_READERSTATE_OUT(
        SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        /*currentState=*/ API.SCARD_STATE_EMPTY,
        /*eventState=*/ API.SCARD_STATE_CHANGED | API.SCARD_STATE_UNAVAILABLE,
        /*atr=*/ new ArrayBuffer(0));
    assertTrue(
        `${debugDumpFull(readerState)} equals neither ${
            debugDumpFull(expected1)} nor ${debugDumpFull(expected2)}`,
        readerStateEquals(readerState, expected1) ||
            readerStateEquals(readerState, expected2));

    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardGetStatusChange()` returns the reader and card information.
  'testSCardGetStatusChange_withCardInitially': async function() {
    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const context = await establishContextOrThrow();

    const result = await client.api.SCardGetStatusChange(
        context, API.INFINITE,
        [new API.SCARD_READERSTATE_IN(
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            API.SCARD_STATE_UNKNOWN)]);

    let readerStates = null;
    result.get(
        (readerStatesArg) => {
          readerStates = readerStatesArg;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assertEquals(readerStates.length, 1);
    assertReaderStateEquals(
        readerStates[0],
        new API.SCARD_READERSTATE_OUT(
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            /*currentState=*/ API.SCARD_STATE_UNKNOWN,
            /*eventState=*/ API.SCARD_STATE_CHANGED | API.SCARD_STATE_PRESENT,
            SimulationConstants.COSMO_ID_70_ATR));
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardGetStatusChange()` detects when a card is inserted.
  'testSCardGetStatusChange_cardInserting': async function() {
    // Start with a connected empty reader.
    await launchPcscServer(
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
        ]);
    const context = await establishContextOrThrow();

    const resultPromise = client.api.SCardGetStatusChange(
        context, API.INFINITE,
        [new API.SCARD_READERSTATE_IN(
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            API.SCARD_STATE_EMPTY)]);
    // Simulate card insertion.
    await setSimulatedDevices([{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const result = await resultPromise;

    let readerStates = null;
    result.get(
        (readerStatesArg) => {
          readerStates = readerStatesArg;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assertEquals(readerStates.length, 1);
    // The "eventState" field contains the number of card insertion/removal
    // events in the higher 16 bits.
    assertReaderStateEquals(
        readerStates[0],
        new API.SCARD_READERSTATE_OUT(
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            /*currentState=*/ API.SCARD_STATE_EMPTY,
            /*eventState=*/ API.SCARD_STATE_CHANGED | API.SCARD_STATE_PRESENT |
                0x10000,
            SimulationConstants.COSMO_ID_70_ATR));
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardGetStatusChange()` detects when a card is removed.
  'testSCardGetStatusChange_cardRemoving': async function() {
    // Start with a connected reader and a card.
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();

    const resultPromise = client.api.SCardGetStatusChange(
        context, API.INFINITE,
        [new API.SCARD_READERSTATE_IN(
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            API.SCARD_STATE_PRESENT)]);
    // Simulate the card removal.
    await setSimulatedDevices(
        [{'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}]);
    const result = await resultPromise;

    let readerStates = null;
    result.get(
        (readerStatesArg) => {
          readerStates = readerStatesArg;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assertEquals(readerStates.length, 1);
    // The "event_state" field contains the number of card insertion/removal
    // events in the higher 16 bits.
    assertReaderStateEquals(
        readerStates[0],
        new API.SCARD_READERSTATE_OUT(
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            /*currentState=*/ API.SCARD_STATE_PRESENT,
            /*eventState=*/ API.SCARD_STATE_CHANGED | API.SCARD_STATE_EMPTY |
                0x10000,
            /*atr=*/ new ArrayBuffer(0)));
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardCancel()` terminates a running `SCardGetStatusChange()` call.
  'testSCardCancel_success': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    const context = await establishContextOrThrow();

    // Start the long-running call.
    const statusPromise = client.api.SCardGetStatusChange(
        context, API.INFINITE,
        [new API.SCARD_READERSTATE_IN(
            /*readerName=*/ PNP_NOTIFICATION, API.SCARD_STATE_UNAWARE)]);
    // Check that the call is actually blocked (either until a reader event or
    // cancellation happen). The exact interval isn't important here - we just
    // want some reasonably big probability of catching a bug if it's
    // introduced.
    await assertRemainsPending(statusPromise, /*timeoutMillis=*/ 1000);

    // Trigger `SCardCancel()` to abort the long-running call.
    const cancelResult = await client.api.SCardCancel(context);
    // Wait until the `SCardGetStatusChange()` call completes.
    const statusResult = await statusPromise;

    // Check `SCardCancel()` result.
    let called = false;
    cancelResult.get(
        () => {
          called = true;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode} in SCardCancel`);
        });
    assertTrue(called);
    assertEquals(cancelResult.getErrorCode(), API.SCARD_S_SUCCESS);
    // Check `SCardGetStatusChange()` result.
    called = false;
    statusResult.get(
        () => {
          fail('Unexpectedly succeeded in SCardGetStatusChange');
        },
        (errorCode) => {
          called = true;
          assertEquals(errorCode, API.SCARD_E_CANCELLED);
        });
    assertTrue(called);
    assertEquals(statusResult.getErrorCode(), API.SCARD_E_CANCELLED);
  },

  // Test `SCardCancel()` succeeds even when there's no pending
  // `SCardGetStatusChange()` call.
  'testSCardCancel_successNoOp': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    const context = await establishContextOrThrow();

    const result = await client.api.SCardCancel(context);

    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardCancel()` fails when no contexts are obtained.
  'testSCardCancel_errorNoContext': async function() {
    const BAD_CONTEXT = 123;
    await launchPcscServer(/*initialDevices=*/[]);

    const result = await client.api.SCardCancel(BAD_CONTEXT);

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

  // Test `SCardCancel()` fails on a wrong context when there's another
  // established context.
  'testSCardCancel_errorDifferentContext': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    const context = await establishContextOrThrow();
    const badContext = context ^ 1;

    const result = await client.api.SCardCancel(badContext);

    assertEquals(result.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
  },

  // Test `SCardCancel()` fails on an already released context.
  'testSCardCancel_errorAlreadyReleasedContext': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    const context = await establishContextOrThrow();
    const releaseResult = await client.api.SCardReleaseContext(context);
    assertTrue(releaseResult.isSuccessful());

    const cancelResult = await client.api.SCardCancel(context);

    assertEquals(cancelResult.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
  },

  // Test `SCardConnect()` succeeds for dwShareMode `SCARD_SHARE_DIRECT` even
  // when there's no card inserted.
  'testSCardConnect_successDirectNoCard': async function() {
    await launchPcscServer(
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
        ]);
    const context = await establishContextOrThrow();

    const result = await client.api.SCardConnect(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_DIRECT, /*preferred_protocols=*/ 0);

    let called = false;
    result.get(
        (sCardHandle, activeProtocol) => {
          called = true;
          assert(Number.isInteger(sCardHandle));
          assertEquals(activeProtocol, 0);
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardConnect()` successfully connects to a card using the "T1"
  // protocol.
  'testSCardConnect_successT1Card': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();

    const result = await client.api.SCardConnect(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED, API.SCARD_PROTOCOL_ANY);

    let called = false;
    result.get(
        (sCardHandle, activeProtocol) => {
          called = true;
          assert(Number.isInteger(sCardHandle));
          assertEquals(activeProtocol, API.SCARD_PROTOCOL_T1);
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardConnect()` successfully connects via the "T1" protocol if the
  // previous connection via the "RAW" protocol was terminated by
  // `SCardDisconnect` with `SCARD_RESET_CARD`.
  'testSCardConnect_successProtocolChangeAfterReset': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    // Connect via the "RAW" protocol and disconnect.
    const firstResult = await client.api.SCardConnect(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferred_protocols=*/ API.SCARD_PROTOCOL_RAW);
    let sCardHandle;
    firstResult.get(
        (sCardHandleArg) => {
          sCardHandle = sCardHandleArg;
        },
        (errorCode) => {
          fail(`Unexpected error in first SCardConnect: ${errorCode}`);
        });
    const disconnectResult =
        await client.api.SCardDisconnect(sCardHandle, API.SCARD_RESET_CARD);
    assertEquals(disconnectResult.getErrorCode(), API.SCARD_S_SUCCESS);

    // Attempt connecting via a different protocol ("ANY" denotes "either T0
    // or T1").
    const secondResult = await client.api.SCardConnect(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferred_protocols=*/ API.SCARD_PROTOCOL_ANY);

    let called = false;
    secondResult.get(
        (sCardHandleArg, activeProtocol) => {
          called = true;
          assert(Number.isInteger(sCardHandle));
          assertEquals(activeProtocol, API.SCARD_PROTOCOL_T1);
        },
        (errorCode) => {
          fail(`Unexpected error in first SCardConnect: ${errorCode}`);
        });
    assertTrue(called);
    assertEquals(secondResult.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardConnect()` fails for dwShareMode `SCARD_SHARE_SHARED` when
  // there's no card inserted.
  'testSCardConnect_errorNoCard': async function() {
    await launchPcscServer(
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
        ]);
    const context = await establishContextOrThrow();

    const result = await client.api.SCardConnect(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferred_protocols=*/ API.SCARD_PROTOCOL_ANY);

    let called = false;
    result.get(
        () => {
          fail('Unexpectedly succeeded in SCardConnect');
        },
        (errorCode) => {
          called = true;
          assertEquals(errorCode, API.SCARD_E_NO_SMARTCARD);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_E_NO_SMARTCARD);
  },

  // Test `SCardConnect()` fails to connect via the "T1" protocol if the
  // previous connection was using the "RAW" protocol.
  'testSCardConnect_errorProtocolMismatch': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    // Simulate an empty UpdateAdminPolicy message to unblock the WaitAndGet()
    // call. This is normally sent when admin-policy-service.js is first
    // initialized.
    // TODO(emaxx): Replace this by actually creating AdminPolicyService here.
    testController.executableModule.getMessageChannel().send(
        'update_admin_policy', /*payload=*/ {});
    // Connect via the "RAW" protocol and disconnect.
    const firstResult = await client.api.SCardConnect(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferred_protocols=*/ API.SCARD_PROTOCOL_RAW);
    let sCardHandle;
    firstResult.get(
        (sCardHandleArg) => {
          sCardHandle = sCardHandleArg;
        },
        (errorCode) => {
          fail(`Unexpected error in first SCardConnect: ${errorCode}`);
        });
    const disconnectResult =
        await client.api.SCardDisconnect(sCardHandle, API.SCARD_LEAVE_CARD);
    assertEquals(disconnectResult.getErrorCode(), API.SCARD_S_SUCCESS);

    // Attempt connecting via a different protocol ("ANY" denotes "either T0
    // or T1").
    const secondResult = await client.api.SCardConnect(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferred_protocols=*/ API.SCARD_PROTOCOL_ANY);

    assertEquals(secondResult.getErrorCode(), API.SCARD_E_PROTO_MISMATCH);
  },

  // Test `SCardDisconnect()` succeeds for a handle previously obtained via an
  // `SCardConnect()` call.
  'testSCardDisconnect_successLeave': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    const cardHandle = await connectToReaderOrThrow(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferredProtocols=*/ API.SCARD_PROTOCOL_T1,
        /*assertResultProtocol=*/ API.SCARD_PROTOCOL_T1);

    const result =
        await client.api.SCardDisconnect(cardHandle, API.SCARD_LEAVE_CARD);

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

  // Test `SCardReconnect()` succeeds when using the same parameters as the
  // previous `SCardConnect()` call.
  'testSCardReconnect_success': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    const cardHandle = await connectToReaderOrThrow(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferredProtocols=*/ API.SCARD_PROTOCOL_ANY,
        /*assertResultProtocol=*/ API.SCARD_PROTOCOL_T1);

    const result = await client.api.SCardReconnect(
        cardHandle, API.SCARD_SHARE_SHARED,
        /*preferredProtocols=*/ API.SCARD_PROTOCOL_ANY, API.SCARD_LEAVE_CARD);

    let called = false;
    result.get(
        (activeProtocol) => {
          called = true;
          assertEquals(activeProtocol, API.SCARD_PROTOCOL_T1);
        },
        (errorCode) => {
          fail(`Unexpected SCardReconnect error ${errorCode}`);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardStatus()` succeeds and returns information about the card.
  'testSCardStatus_success': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    const cardHandle = await connectToReaderOrThrow(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferredProtocols=*/ API.SCARD_PROTOCOL_ANY,
        /*assertResultProtocol=*/ API.SCARD_PROTOCOL_T1);

    const result = await client.api.SCardStatus(cardHandle);

    let called = false;
    result.get(
        (readerName, state, protocol, atr) => {
          called = true;
          assertEquals(
              readerName,
              SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0);
          assertEquals(
              state,
              API.SCARD_NEGOTIABLE | API.SCARD_POWERED | API.SCARD_PRESENT);
          assertEquals(protocol, API.SCARD_PROTOCOL_T1);
          assertObjectEquals(atr, SimulationConstants.COSMO_ID_70_ATR);
        },
        (errorCode) => {
          fail(`Unexpected SCardStatus error ${errorCode}`);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardGetAttrib()` succeeds and returns information about the card.
  'testSCardGetAttrib_successAtr': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    const cardHandle = await connectToReaderOrThrow(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferredProtocols=*/ API.SCARD_PROTOCOL_ANY,
        /*assertResultProtocol=*/ API.SCARD_PROTOCOL_T1);

    const result =
        await client.api.SCardGetAttrib(cardHandle, API.SCARD_ATTR_ATR_STRING);

    let called = false;
    result.get(
        (attr) => {
          called = true;
          assertObjectEquals(attr, SimulationConstants.COSMO_ID_70_ATR);
        },
        (errorCode) => {
          fail(`Unexpected SCardGetAttrib error ${errorCode}`);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_S_SUCCESS);
  },

  // Test `SCardGetAttrib()` fails when unknown attribute ID is specified.
  'testSCardGetAttrib_errorWrongAttrId': async function() {
    const BAD_ATTR_ID = 1;

    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    const cardHandle = await connectToReaderOrThrow(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferredProtocols=*/ API.SCARD_PROTOCOL_ANY,
        /*assertResultProtocol=*/ API.SCARD_PROTOCOL_T1);

    const result = await client.api.SCardGetAttrib(cardHandle, BAD_ATTR_ID);

    let called = false;
    result.get(
        (attr) => {
          fail('Unexpectedly succeeded in SCardGetAttrib');
        },
        (errorCode) => {
          called = true;
          assertEquals(errorCode, API.SCARD_E_UNSUPPORTED_FEATURE);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_E_UNSUPPORTED_FEATURE);
  },

  // Test `SCardGetAttrib()` fails when there's no connected card handle.
  'testSCardGetAttrib_errorNoHandles': async function() {
    const BAD_HANDLE = 123;

    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    await establishContextOrThrow();

    const result =
        await client.api.SCardGetAttrib(BAD_HANDLE, API.SCARD_ATTR_ATR_STRING);

    let called = false;
    result.get(
        () => {
          fail('Unexpectedly succeeded in SCardGetAttrib');
        },
        (errorCode) => {
          called = true;
          assertEquals(errorCode, API.SCARD_E_INVALID_HANDLE);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
  },

  // Test `SCardGetAttrib()` fails when the card handle is already
  // disconnected.
  'testSCardGetAttrib_errorDisconnectedHandle': async function() {
    await launchPcscServer(
        /*initialDevices=*/[{
          'id': 123,
          'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
          'cardType': SimulationConstants.COSMO_CARD_TYPE
        }]);
    const context = await establishContextOrThrow();
    const cardHandle = await connectToReaderOrThrow(
        context, SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        API.SCARD_SHARE_SHARED,
        /*preferredProtocols=*/ API.SCARD_PROTOCOL_ANY,
        /*assertResultProtocol=*/ API.SCARD_PROTOCOL_T1);
    await client.api.SCardDisconnect(cardHandle, API.SCARD_LEAVE_CARD);

    const result =
        await client.api.SCardGetAttrib(cardHandle, API.SCARD_ATTR_ATR_STRING);

    let called = false;
    result.get(
        () => {
          fail('Unexpectedly succeeded in SCardGetAttrib');
        },
        (errorCode) => {
          called = true;
          assertEquals(errorCode, API.SCARD_E_INVALID_HANDLE);
        });
    assert(called);
    assertEquals(result.getErrorCode(), API.SCARD_E_INVALID_HANDLE);
  },

  // Test that no unexpected exceptions occur when the C++ module is crashing
  // while there's a connected PC/SC client.
  'testCrashWithActiveClient': async function() {
    const BAD_CONTEXT = 123;
    await testController.setUpCppHelper(
        'LoggingTestHelper', /*helperArgument=*/ {});
    await launchPcscServer(/*initialDevices=*/[]);
    // Make a random call, to make sure all lazily created per-client state
    // gets initialized.
    await client.api.SCardIsValidContext(BAD_CONTEXT);

    // Trigger the C++ module crash.
    const error = await GSC.AsyncAsserts.assertThrows(async () => {
      return await testController.sendMessageToCppHelper(
          'LoggingTestHelper', 'crash-via-check');
    });

    assertContains('requester is disposed', error.toString());
    assert(testController.executableModule.isDisposed());
    assert(client.clientHandler.isDisposed());
  },
};

const MALFORMED_REQUEST_TEST_CASES = {
  // Test no exception happens when receiving a message with completely
  // unexpected fields.
  'testGarbage': function() {
    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(/*serviceName=*/ 'foo', /*payload=*/ {});
  },

  // Test no exception happens when receiving a message with correct type,
  // but empty data.
  'testEmptyData': function() {
    const MESSAGE_DATA = {};

    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(PCSC_REQUEST_MESSAGE_TYPE, MESSAGE_DATA);
  },

  // Test no exception happens when receiving a message with no payload in
  // the message data.
  'testNoRequestPayload': function() {
    const MESSAGE_DATA = {
      'request_id': 1,
    };

    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(PCSC_REQUEST_MESSAGE_TYPE, MESSAGE_DATA);
  },

  // Test no exception happens when receiving a message with no request ID
  // in the message data.
  'testNoRequestId': function() {
    const MESSAGE_DATA = {
      'payload': {
        'function_name': 'SCardIsValidContext',
        'arguments': [0],
      },
    };

    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(PCSC_REQUEST_MESSAGE_TYPE, MESSAGE_DATA);
  },

  // Test no exception happens when receiving a message with a wrong
  // function name, not being part of the PC/SC API.
  'testWrongFunction': function() {
    const MESSAGE_DATA = {
      'request_id': 1,
      'payload': {
        'function_name': 'SCardDemolish',
        'arguments': [0],
      },
    };

    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(PCSC_REQUEST_MESSAGE_TYPE, MESSAGE_DATA);
  },

  // Test no exception happens when receiving a message with function
  // arguments not being an array.
  'testNonArrayArguments': function() {
    const MESSAGE_DATA = {
      'request_id': 1,
      'payload': {
        'function_name': 'SCardIsValidContext',
        'arguments': 0,
      },
    };

    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(PCSC_REQUEST_MESSAGE_TYPE, MESSAGE_DATA);
  },

  // Test no exception happens when receiving a message with the function
  // argument having an incorrect type.
  'testWrongArgumentType': function() {
    const MESSAGE_DATA = {
      'request_id': 1,
      'payload': {
        'function_name': 'SCardIsValidContext',
        'arguments': ['garbage'],
      },
    };

    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(PCSC_REQUEST_MESSAGE_TYPE, MESSAGE_DATA);
  },

  // Test no exception happens when receiving a message with the function
  // argument is unexpectedly negative number.
  'testOutOfRangeArgument': function() {
    const MESSAGE_DATA = {
      'request_id': 1,
      'payload': {
        'function_name': 'SCardIsValidContext',
        'arguments': [-1],
      },
    };

    const senderMessageChannel = client.apiMessageChannelPair.getSecond();
    senderMessageChannel.send(PCSC_REQUEST_MESSAGE_TYPE, MESSAGE_DATA);
  },
};

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
      if (testController) {
        await testController.disposeAsync();
      }
      if (pcscReadinessTracker) {
        pcscReadinessTracker.dispose();
      }
    } finally {
      pcscReadinessTracker = null;
      ClientHandler.overridePermissionsCheckerForTesting(null);
      testController = null;
    }
  },

  ...SIMPLE_TEST_CASES,

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

    ...TEST_CASES_WITH_SINGLE_CLIENT,

    // Group of tests that simulate incorrect requests sent by clients.
    'testMalformedRequest': {
      'setUp': async function() {
        await launchPcscServer(/*initialDevices=*/[]);
        await pcscReadinessTracker.promise;
        // Dispose of the API object, because it'd complain about unexpected
        // responses (to malformed requests we simulate below).
        client.api.dispose();
      },

      'tearDown': async function() {
        // Wait for some time before exiting, to let any possible bug have a
        // chance to trigger an error.
        await GSC.PromiseHelpers.sleep(/*delayMilliseconds=*/ 1000);
        // Verify the C/C++ module hasn't crashed.
        assertFalse(testController.executableModule.isDisposed());
      },

      ...MALFORMED_REQUEST_TEST_CASES,
    },
  },
});
});  // goog.scope
