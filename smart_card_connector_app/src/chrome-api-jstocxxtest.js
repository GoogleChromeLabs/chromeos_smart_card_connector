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
 * @fileoverview This file contains integration tests for ChromeApiProvider.
 */
goog.require('GoogleSmartCard.ConnectorApp.ChromeApiProvider');
goog.require('GoogleSmartCard.ConnectorApp.MockChromeApi');
goog.require('GoogleSmartCard.IntegrationTestController');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTrackerThroughPcscServerHook');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.TestingLibusbSmartCardSimulationConstants');
goog.require('goog.Thenable');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ChromeApiProvider = GSC.ConnectorApp.ChromeApiProvider;
const MockChromeApi = GSC.ConnectorApp.MockChromeApi;
const ReadinessTracker = GSC.PcscLiteServerClientsManagement.ReadinessTracker;
const SimulationConstants = GSC.TestingLibusbSmartCardSimulationConstants;
const ArgMatcher = goog.testing.mockmatchers.ArgumentMatcher;

const PNP_NOTIFICATION = GSC.PcscLiteCommon.Constants.PNP_NOTIFICATION;

/** @type {GSC.IntegrationTestController?} */
let testController;
/** @type {GSC.LibusbProxyReceiver?} */
let libusbProxyReceiver;
/** @type {ReadinessTracker?} */
let pcscReadinessTracker;
/** @type {MockChromeApi?} */
let mockChromeApi;
/** @type {!goog.testing.MockControl|undefined} */
let mockControl;
/** @type {ChromeApiProvider?} */
let chromeApiProvider;
/** @type {number} */
let readerHandle;

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

function createChromeApiProvider() {
  chromeApiProvider = new ChromeApiProvider(
      testController.executableModule.getMessageChannel(),
      pcscReadinessTracker.promise);
}

/**
 * @param {number} timeoutMillis
 * @return {!Promise}
 */
async function sleep(timeoutMillis) {
  return new Promise(resolve => {
    setTimeout(resolve, timeoutMillis);
  });
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
  await sleep(timeoutMillis);
  sleeping = false;
}

/**
 * Switches the mocked chrome.smartCardProviderPrivate function back
 * to record mode.
 * Verifies that there were no unmet expectations before switching.
 * @param {string} functionName
 * @returns {Function} Mocked function object.
 */
function getFreshMock(functionName) {
  const mock = chrome.smartCardProviderPrivate[functionName];
  assertEquals(
      `No mocked function ${functionName} found`, 'function', typeof mock);
  mock.$verify();
  mock.$reset();
  return mock;
}

/**
 * Dispatches `onEstablishContextRequested` and checks that result was reported
 * correctly.
 * @param {number} requestId
 * @returns {!Promise<number>} Established context.
 */
async function establishContext(requestId) {
  let sCardContext = 0;
  getFreshMock('reportEstablishContextResult')(
      requestId, goog.testing.mockmatchers.isNumber, 'SUCCESS')
      .$once()
      .$does((requestId, context, resultCode) => {
        sCardContext = context;
      })
      .$replay();
  await mockChromeApi.dispatchEvent('onEstablishContextRequested', requestId)
      .$waitAndVerify();
  return sCardContext;
}

/**
 * Dispatches `onConnectRequested` and checks that result was reported
 * correctly.
 * @param {number} requestId
 * @param {number} sCardContext
 * @param {string} reader
 * @param {!chrome.smartCardProviderPrivate.ShareMode} shareMode
 * @param {!chrome.smartCardProviderPrivate.Protocols} preferredProtocols
 * @param {!chrome.smartCardProviderPrivate.Protocol} expectedProtocol
 * @returns {!Promise<!number>} Established connection handle.
 */
async function connect(
    requestId, sCardContext, reader, shareMode, preferredProtocols,
    expectedProtocol) {
  let sCardHandle = 0;
  getFreshMock('reportConnectResult')(
      requestId, goog.testing.mockmatchers.isNumber, expectedProtocol,
      'SUCCESS')
      .$once()
      .$does((_0, handle, _1, _2) => {
        sCardHandle = handle;
      })
      .$replay();
  await mockChromeApi
      .dispatchEvent(
          'onConnectRequested', requestId, sCardContext, reader, shareMode,
          preferredProtocols)
      .$waitAndVerify();
  return sCardHandle;
}

/**
 * Sets expectation that reportEstablishContextResult will be called for given
 * `requestId` with result code equal to `resultCode`.
 * @param {number} requestId
 * @param {string} resultCode
 */
function expectReportReleaseContext(requestId, resultCode) {
  getFreshMock('reportReleaseContextResult')(requestId, resultCode)
      .$once()
      .$replay();
}

/**
 * Sets expectation that reportListReadersResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {!Array.<string>} readers
 * @param {string} resultCode
 */
function expectReportListReaders(requestId, readers, resultCode) {
  getFreshMock('reportListReadersResult')(requestId, readers, resultCode)
      .$once()
      .$replay();
}

/**
 * Sets expectation that reportGetStatusChangeResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {!Array<!chrome.smartCardProviderPrivate.ReaderStateOut>} readerStates
 * @param {string} resultCode
 */
function expectReportGetStatusChange(requestId, readerStates, resultCode) {
  getFreshMock('reportGetStatusChangeResult')(
      requestId, readerStates, resultCode)
      .$once()
      .$replay();
}

/**
 * Sets expectation that reportPlaintResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {string} resultCode
 */
function expectReportPlainResult(requestId, resultCode) {
  getFreshMock('reportPlainResult')(requestId, resultCode).$once().$replay();
}

/**
 * Sets expectation that reportConnectResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {!chrome.smartCardProviderPrivate.Protocol} activeProtocol
 * @param {string} resultCode
 */
function expectReportConnectResult(requestId, activeProtocol, resultCode) {
  getFreshMock('reportConnectResult')(
      requestId,
      /*scardHandle=*/ goog.testing.mockmatchers.isNumber, activeProtocol,
      resultCode)
      .$once()
      .$replay();
}

/**
 * Sets expectation that reportStatusResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {string|!ArgMatcher} readerName
 * @param {!chrome.smartCardProviderPrivate.ConnectionState|!ArgMatcher} state
 * @param {!chrome.smartCardProviderPrivate.Protocol|!ArgMatcher} protocol
 * @param {!ArrayBuffer|!ArgMatcher} atr
 * @param {string|!ArgMatcher} resultCode
 */
function expectReportStatusResult(
    requestId, readerName, state, protocol, atr, resultCode) {
  getFreshMock('reportStatusResult')(
      requestId, readerName, state, protocol, atr, resultCode)
      .$once()
      .$replay();
}

goog.exportSymbol('testChromeApiProviderToCpp', {
  'setUp': async function() {
    // Set up the controller and load the C/C++ executable module.
    testController = new GSC.IntegrationTestController();
    await testController.initAsync();
    // Stub out libusb receiver.
    libusbProxyReceiver = new GSC.LibusbProxyReceiver(
        testController.executableModule.getMessageChannel());
    libusbProxyReceiver.addHook(new GSC.TestingLibusbSmartCardSimulationHook(
        testController.executableModule.getMessageChannel()));
    // Set up observers.
    pcscReadinessTracker = new ReadinessTracker(
        testController.executableModule.getMessageChannel());
    // Mock chrome.smartCardProviderPrivate API.
    mockControl = new goog.testing.MockControl();
    mockChromeApi =
        new MockChromeApi(mockControl, testController.propertyReplacer);
    mockControl.$replayAll();

    createChromeApiProvider();
  },

  'tearDown': async function() {
    try {
      await testController.disposeAsync();
      pcscReadinessTracker.dispose();
      assertTrue(chromeApiProvider.isDisposed());
      // Check all mock expectations are satisfied.
      mockControl.$verifyAll();
    } finally {
      chromeApiProvider = null;
      pcscReadinessTracker = null;
      testController = null;
    }
  },

  'testSmoke': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    await pcscReadinessTracker.promise;
  },

  // Test a single `onEstablishContextRequested` event.
  'testEstablishContext': async function() {
    await launchPcscServer(/*initialDevices=*/[]);
    await establishContext(/*requestId=*/ 123);
  },

  // Test releasing the established context.
  'testReleaseContext': async function() {
    expectReportReleaseContext(/*requestId=*/ 124, 'SUCCESS');

    await launchPcscServer(/*initialDevices=*/[]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onReleaseContextRequested', /*requestId=*/ 124, sCardContext)
        .$waitAndVerify();
  },

  // Test that releasing bogus context returns INVALID_HANDLE error.
  'testReleaseContext_none': async function() {
    const BAD_CONTEXT = 123;
    expectReportReleaseContext(/*requestId=*/ 42, 'INVALID_HANDLE');

    await launchPcscServer(/*initialDevices=*/[]);
    await mockChromeApi
        .dispatchEvent(
            'onReleaseContextRequested', /*requestId=*/ 42, BAD_CONTEXT)
        .$waitAndVerify();
  },

  // Test ListReaders with no readers attached.
  'testListReaders_none': async function() {
    expectReportListReaders(/*requestId=*/ 124, [], 'NO_READERS_AVAILABLE');

    await launchPcscServer(/*initialDevices=*/[]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 124, sCardContext)
        .$waitAndVerify();
  },

  // Test that ListReaders requested with already released context returns
  // INVALID_HANDLE error.
  'testListReaders_releasedContext': async function() {
    expectReportReleaseContext(/*requestId=*/ 124, 'SUCCESS');
    expectReportListReaders(/*requestId=*/ 125, [], 'INVALID_HANDLE');

    await launchPcscServer(/*initialDevices=*/[]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onReleaseContextRequested', /*requestId=*/ 124, sCardContext)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 125, sCardContext)
        .$waitAndVerify();
  },

  // Test ListReaders returns a one-item list when there's a single attached
  // device.
  'testListReaders_singleDevice': async function() {
    expectReportListReaders(
        /*requestId=*/ 124, ['Gemalto PC Twin Reader 00 00'], 'SUCCESS');

    await launchPcscServer(
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
        ]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 124, sCardContext)
        .$waitAndVerify();
  },


  // Test ListReaders returns a two-item list when there're two attached device.
  'testListReaders_twoDevices': async function() {
    expectReportListReaders(
        /*requestId=*/ 124,
        [
          'Gemalto PC Twin Reader 00 00',
          'Dell Dell Smart Card Reader Keyboard 01 00'
        ],
        'SUCCESS');

    await launchPcscServer(
        /*initialDevices=*/[
          {'id': 101, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE},
          {'id': 102, 'type': SimulationConstants.DELL_DEVICE_TYPE}
        ]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 124, sCardContext)
        .$waitAndVerify();
  },

  // Test GetStatusChange with `\\?PnP?\Notification` to get the reader added
  // event.
  'testGetStatusChange_deviceAppearing': async function() {
    expectReportGetStatusChange(
        /*requestId=*/ 124, [{
          'reader': PNP_NOTIFICATION,
          'eventState': {'changed': true},
          'eventCount': 0,
          'atr': new ArrayBuffer(0)
        }],
        'SUCCESS');

    await launchPcscServer(/*initialDevices=*/[]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    const verifyResult =
        mockChromeApi
            .dispatchEvent(
                'onGetStatusChangeRequested', /*requestId=*/ 124, sCardContext,
                /*timeout=*/ {}, [{
                  'reader': PNP_NOTIFICATION,
                  'currentState': {'unaware': true},
                  'currentCount': 0
                }])
            .$waitAndVerify();
    await setSimulatedDevices(
        [{'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}]);
    await verifyResult;
  },

  // Test GetStatusChange returns the reader and card information.
  'testGetStatusChange_withCardInitially': async function() {
    expectReportGetStatusChange(
        /*requestId=*/ 124, [{
          'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
          'eventState': {'changed': true, 'present': true},
          'eventCount': 0,
          'atr': SimulationConstants.COSMO_ID_70_ATR
        }],
        'SUCCESS');

    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onGetStatusChangeRequested', /*requestId=*/ 124, sCardContext,
            /*timeout=*/ {}, [{
              'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
              'currentState': {'unknown': true},
              'currentCount': 0
            }])
        .$waitAndVerify();
  },

  // Test GetStatusChange detects when a card is removed.
  'testGetStatusChange_cardRemoving': async function() {
    expectReportGetStatusChange(
        /*requestId=*/ 124, [{
          'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
          'eventState': {'changed': true, 'empty': true},
          /*corresponds to the number of card insertions/removals*/
          'eventCount': 1,
          'atr': new ArrayBuffer(0)
        }],
        'SUCCESS');

    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    const verifyResult =
        mockChromeApi
            .dispatchEvent(
                'onGetStatusChangeRequested', /*requestId=*/ 124, sCardContext,
                /*timeout=*/ {}, [{
                  'reader':
                      SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
                  'currentState': {'present': true},
                  'currentCount': 0
                }])
            .$waitAndVerify();
    // Simulate the card removal.
    await setSimulatedDevices(
        [{'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}]);
    await verifyResult;
  },

  // Test GetStatusChange with timeout parameter will return even if status
  // is not changing.
  'testGetStatusChange_withTimeout': async function() {
    expectReportGetStatusChange(
        /*requestId=*/ 124, [], 'TIMEOUT');

    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onGetStatusChangeRequested', /*requestId=*/ 124, sCardContext,
            /*timeout=*/ {'milliseconds': 10}, [{
              'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
              'currentState': {'present': true},
              'currentCount': 0
            }])
        .$waitAndVerify();
  },

  // Test Cancel terminates a running GetStatusChange call.
  'testCancel': async function() {
    expectReportGetStatusChange(/*requestId=*/ 124, [], 'CANCELLED');
    expectReportPlainResult(/*requestId=*/ 125, 'SUCCESS');

    await launchPcscServer(/*initialDevices=*/[
      {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
    ]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    const statusPromise =
        mockChromeApi
            .dispatchEvent(
                'onGetStatusChangeRequested', /*requestId=*/ 124, sCardContext,
                /*timeout=*/ {}, [{
                  'reader':
                      SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
                  'currentState': {'empty': true},
                  'currentCount': 0
                }])
            .$waitAndVerify();
    // Check that the call is actually blocked (either until a reader event or
    // cancellation happen). The exact interval isn't important here - we just
    // want some reasonably big probability of catching a bug if it's
    // introduced.
    await assertRemainsPending(statusPromise, /*timeoutMillis=*/ 1000);
    // Trigger Cancel to abort the long-running call.
    await mockChromeApi
        .dispatchEvent('onCancelRequested', /*requestId=*/ 125, sCardContext)
        .$waitAndVerify();
    // Wait until GetStatusChange returns and result is verified.
    await statusPromise;
  },

  // Test Connect fails when there's no card inserted.
  'testConnect_errorNoCard': async function() {
    expectReportConnectResult(
        /*requestId=*/ 124, chrome.smartCardProviderPrivate.Protocol.UNDEFINED,
        'NO_SMARTCARD');

    await launchPcscServer(/*initialDevices=*/[
      {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
    ]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    await mockChromeApi
        .dispatchEvent(
            'onConnectRequested', /*requestId=*/ 124, sCardContext,
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            chrome.smartCardProviderPrivate.ShareMode.SHARED,
            /*preferredProtocols=*/ {'t0': true, 't1': true})
        .$waitAndVerify();
  },

  // Test Connect succeeds with `ShareMode.DIRECT` even when there's no card
  // inserted.
  'testConnect_direct': async function() {
    await launchPcscServer(/*initialDevices=*/[
      {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
    ]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    expectReportConnectResult(
        /*requestId=*/ 124, chrome.smartCardProviderPrivate.Protocol.UNDEFINED,
        'SUCCESS');
    await mockChromeApi
        .dispatchEvent(
            'onConnectRequested', /*requestId=*/ 124, sCardContext,
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            chrome.smartCardProviderPrivate.ShareMode.DIRECT,
            /*preferredProtocols=*/ {})
        .$waitAndVerify();
  },

  // Test Connect successfully connects to a card using the "T1" protocol.
  'testConnect_t1': async function() {
    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const sCardContext = await establishContext(/*requestId=*/ 123);
    expectReportConnectResult(
        /*requestId=*/ 124, chrome.smartCardProviderPrivate.Protocol.T1,
        'SUCCESS');
    await mockChromeApi
        .dispatchEvent(
            'onConnectRequested', /*requestId=*/ 124, sCardContext,
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            chrome.smartCardProviderPrivate.ShareMode.SHARED,
            /*preferredProtocols=*/ {'t1': true})
        .$waitAndVerify();
  },

  // Test Disconnect succeeds with correct handle.
  'testDisconnect_simple': async function() {
    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const sCardContext = await establishContext(/*requestId=*/ 111);
    const sCardHandle = await connect(
        /*requestId=*/ 112, sCardContext,
        SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        chrome.smartCardProviderPrivate.ShareMode.SHARED,
        /*preferredProtocols=*/ {'t1': true},
        /*expectedProtocol=*/ chrome.smartCardProviderPrivate.Protocol.T1);
    expectReportPlainResult(/*requestId=*/ 113, 'SUCCESS');
    await mockChromeApi
        .dispatchEvent(
            'onDisconnectRequested', /*requestId=*/ 113, sCardHandle,
            chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
        .$waitAndVerify();
  },

  // Test Disconnect fails with invalid handle.
  'testDisconnect_invalid': async function() {
    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    const BAD_HANDLE = 12228;
    expectReportPlainResult(/*requestId=*/ 113, 'INVALID_HANDLE');
    await mockChromeApi
        .dispatchEvent(
            'onDisconnectRequested', /*requestId=*/ 113, BAD_HANDLE,
            chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
        .$waitAndVerify();
  },

  'testConnectedReader': {
    'setUp': async function() {
      await launchPcscServer(/*initialDevices=*/[{
        'id': 123,
        'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
        'cardType': SimulationConstants.COSMO_CARD_TYPE
      }]);
      const sCardContext = await establishContext(/*requestId=*/ 66);
      readerHandle = await connect(
          /*requestId=*/ 67, sCardContext,
          SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
          chrome.smartCardProviderPrivate.ShareMode.SHARED,
          /*preferredProtocols=*/ {'t1': true},
          /*expectedProtocol=*/ chrome.smartCardProviderPrivate.Protocol.T1);
    },

    'tearDown': async function() {
      expectReportPlainResult(/*requestId=*/ 68, 'SUCCESS');
      await mockChromeApi
          .dispatchEvent(
              'onDisconnectRequested', /*requestId=*/ 68, readerHandle,
              chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
          .$waitAndVerify();
    },

    // Test that Status returns information about the card.
    'testStatus': async function() {
      expectReportStatusResult(
          /*requestId=*/ 111,
          SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
          chrome.smartCardProviderPrivate.ConnectionState.NEGOTIABLE,
          chrome.smartCardProviderPrivate.Protocol.T1,
          SimulationConstants.COSMO_ID_70_ATR, 'SUCCESS');
      await mockChromeApi
          .dispatchEvent('onStatusRequested', /*requestId=*/ 111, readerHandle)
          .$waitAndVerify();
    },

    // Test that Status returns READER_UNAVAILABLE after reader disappears.
    'testStatus_readerUnavailable': async function() {
      await setSimulatedDevices(/*devices=*/[]);

      // Status should eventually return a READER_UNAVAILABLE error.
      for (let requestId = 111; ; requestId += 1) {
        const ignore = goog.testing.mockmatchers.ignoreArgument;
        let savedArgs;
        getFreshMock('reportStatusResult')(
            requestId, ignore, ignore, ignore, ignore, ignore)
            .$once()
            .$does((requestId, ...args) => {
              savedArgs = args;
            })
            .$replay();
        await mockChromeApi
            .dispatchEvent('onStatusRequested', requestId, readerHandle)
            .$waitAndVerify();

        const resultCode = savedArgs[savedArgs.length - 1];
        // If result code is still SUCCESS, continue.
        if (resultCode == 'SUCCESS') {
          assertArrayEquals(
              [
                SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
                chrome.smartCardProviderPrivate.ConnectionState.NEGOTIABLE,
                chrome.smartCardProviderPrivate.Protocol.T1,
                SimulationConstants.COSMO_ID_70_ATR, 'SUCCESS'
              ],
              savedArgs);
          continue;
        }

        if (resultCode != 'READER_UNAVAILABLE') {
          fail(`Unexpected result code: ${resultCode}`);
        }

        // Got the expected error code.
        // Since result code is error, default values should be reported.
        assertArrayEquals(
            [
              '', chrome.smartCardProviderPrivate.ConnectionState.ABSENT,
              chrome.smartCardProviderPrivate.Protocol.UNDEFINED,
              new ArrayBuffer(0), 'READER_UNAVAILABLE'
            ],
            savedArgs);
        break;
      }
    },

    // Test that Status returns REMOVED_CARD after removing the card.
    'testStatus_removedCard': async function() {
      // Simulate the card removal.
      await setSimulatedDevices(/*devices=*/[
        {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
      ]);

      // Status should eventually return a REMOVED_CARD error.
      for (let requestId = 111; ; requestId += 1) {
        const ignore = goog.testing.mockmatchers.ignoreArgument;
        let savedArgs;
        getFreshMock('reportStatusResult')(
            requestId, ignore, ignore, ignore, ignore, ignore)
            .$once()
            .$does((requestId, ...args) => {
              savedArgs = args;
            })
            .$replay();
        await mockChromeApi
            .dispatchEvent('onStatusRequested', requestId, readerHandle)
            .$waitAndVerify();

        const resultCode = savedArgs[savedArgs.length - 1];
        // If result code is still SUCCESS, continue.
        if (resultCode == 'SUCCESS') {
          assertArrayEquals(
              [
                SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
                chrome.smartCardProviderPrivate.ConnectionState.NEGOTIABLE,
                chrome.smartCardProviderPrivate.Protocol.T1,
                SimulationConstants.COSMO_ID_70_ATR, 'SUCCESS'
              ],
              savedArgs);
          continue;
        }

        if (resultCode != 'REMOVED_CARD') {
          goog.testing.asserts.raiseException(
              `Unexpected result code: ${resultCode}`);
        }

        // Got the expected error code.
        // Since result code is error, default values should be reported.
        assertArrayEquals(
            [
              '', chrome.smartCardProviderPrivate.ConnectionState.ABSENT,
              chrome.smartCardProviderPrivate.Protocol.UNDEFINED,
              new ArrayBuffer(0), 'REMOVED_CARD'
            ],
            savedArgs);
        break;
      }
    }
  }
});
});  // goog.scope
