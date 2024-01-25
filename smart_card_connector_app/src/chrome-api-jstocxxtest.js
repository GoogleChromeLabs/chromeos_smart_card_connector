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
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PcscLiteServer.ReaderTrackerThroughPcscServerHook');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.TestingConstants');
goog.require('GoogleSmartCard.TestingLibusbSmartCardSimulationConstants');
goog.require('goog.Thenable');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const API = GSC.PcscLiteClient.API;
const ChromeApiProvider = GSC.ConnectorApp.ChromeApiProvider;
const MockChromeApi = GSC.ConnectorApp.MockChromeApi;
const ReadinessTracker = GSC.PcscLiteServerClientsManagement.ReadinessTracker;
const SimulationConstants = GSC.TestingLibusbSmartCardSimulationConstants;
const ArgMatcher = goog.testing.mockmatchers.ArgumentMatcher;

const PNP_NOTIFICATION = GSC.PcscLiteCommon.Constants.PNP_NOTIFICATION;
const IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE =
    GSC.TestingConstants.IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE;
const IOCTL_FEATURE_IFD_PIN_PROPERTIES =
    GSC.TestingConstants.IOCTL_FEATURE_IFD_PIN_PROPERTIES;
const TAG_IFD_DEVICE_REMOVED =
    GSC.PcscLiteCommon.Constants.TAG_IFD_DEVICE_REMOVED;

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
/** @type {number?} */
let readerHandle;
/** @type {number?} */
let releasedContext;

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

/**
 * Sets expectation that reportDataResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {!ArrayBuffer} data
 * @param {string} resultCode
 */
function expectReportDataResult(requestId, data, resultCode) {
  getFreshMock('reportDataResult')(requestId, data, resultCode)
      .$once()
      .$replay();
}

const SIMPLE_TEST_CASES = {
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

  // Test ListReaders returns a two-item list when there're two attached
  // devices.
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

  // Test Transmit success.
  'testTransmit_success': async function() {
    // Set up a reader and a card with a PIV profile.
    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE,
      'cardProfile': SimulationConstants.CHARISMATHICS_PIV_TYPE
    }]);

    const sCardContext = await establishContext(/*requestId=*/ 66);
    const sCardHandle = await connect(
        /*requestId=*/ 67, sCardContext,
        SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        chrome.smartCardProviderPrivate.ShareMode.SHARED,
        /*preferredProtocols=*/ {'t1': true},
        /*expectedProtocol=*/ chrome.smartCardProviderPrivate.Protocol.T1);

    // Reply should contain the application identifier, followed by the status
    // bytes that denote success: SW1=0x90, SW2=0x00. This corresponds to the
    // fake reply built by `testing_smart_card_simulation.cc`.
    expectReportDataResult(
        /*requestId=*/ 111,
        new Uint8Array([
          ...SimulationConstants.APPLICATION_IDENTIFIER_CHARISMATHICS_PIV, 0x90,
          0x00
        ]).buffer,
        'SUCCESS');

    await mockChromeApi
        .dispatchEvent(
            'onTransmitRequested', /*requestId=*/ 111, sCardHandle,
            chrome.smartCardProviderPrivate.Protocol.T1,
            /*data=*/ SimulationConstants.SELECT_COMMAND)
        .$waitAndVerify();
  },

  // Test that Control requested on an already terminated connection
  // returns INVALID_HANDLE error.
  'testTransmit_terminatedConnection': async function() {
    // Set up a reader and a card with a PIV profile.
    await launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE,
      'cardProfile': SimulationConstants.CHARISMATHICS_PIV_TYPE
    }]);

    // Connect to reader and terminate connection.
    const sCardContext = await establishContext(/*requestId=*/ 66);
    const sCardHandle = await connect(
        /*requestId=*/ 67, sCardContext,
        SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
        chrome.smartCardProviderPrivate.ShareMode.SHARED,
        /*preferredProtocols=*/ {'t1': true},
        /*expectedProtocol=*/ chrome.smartCardProviderPrivate.Protocol.T1);
    expectReportPlainResult(/*requestId=*/ 68, 'SUCCESS');
    await mockChromeApi
        .dispatchEvent(
            'onDisconnectRequested', /*requestId=*/ 68, sCardHandle,
            chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
        .$waitAndVerify();

    // Check that Transmit will return INVALID_HANDLE.
    expectReportDataResult(
        /*requestId=*/ 69, new ArrayBuffer(0), 'INVALID_HANDLE');
    await mockChromeApi
        .dispatchEvent(
            'onTransmitRequested', /*requestId=*/ 69, sCardHandle,
            chrome.smartCardProviderPrivate.Protocol.T1,
            /*data=*/ SimulationConstants.SELECT_COMMAND)
        .$waitAndVerify();
  },
};

const RELEASED_CONTEXT_TEST_CASES = {
  // Test that ReleaseContext requested on an already released context
  // returns INVALID_HANDLE error.
  'testReleaseContext': async function() {
    expectReportReleaseContext(/*requestId=*/ 1001, 'INVALID_HANDLE');
    await mockChromeApi
        .dispatchEvent(
            'onReleaseContextRequested', /*requestId=*/ 1001, releasedContext)
        .$waitAndVerify();
  },

  // Test that ListReaders requested on an already released context returns
  // INVALID_HANDLE error.
  'testListReaders': async function() {
    expectReportListReaders(/*requestId=*/ 1001, [], 'INVALID_HANDLE');
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 1001, releasedContext)
        .$waitAndVerify();
  },
};

const RELEASED_CONTEXT_WITH_READER_TEST_CASES = {
  // Test that GetStatusChange requested on an already released context
  // returns INVALID_HANDLE error.
  'testGetStatusChange': async function() {
    expectReportGetStatusChange(/*requestId=*/ 1001, [], 'INVALID_HANDLE');
    await mockChromeApi
        .dispatchEvent(
            'onGetStatusChangeRequested',
            /*requestId=*/ 1001, releasedContext,
            /*timeout=*/ {}, [{
              'reader': PNP_NOTIFICATION,
              'currentState': {'unaware': true},
              'currentCount': 0
            }])
        .$waitAndVerify();
  },

  // Test that Cancel requested on an already released context returns
  // INVALID_HANDLE error.
  'testCancel': async function() {
    expectReportPlainResult(/*requestId=*/ 1001, 'INVALID_HANDLE');
    await mockChromeApi
        .dispatchEvent(
            'onCancelRequested', /*requestId=*/ 1001, releasedContext)
        .$waitAndVerify();
  },

  // Test that Connect requested on an already released context returns
  // INVALID_HANDLE error.
  'testConnect': async function() {
    expectReportConnectResult(
        /*requestId=*/ 1001, chrome.smartCardProviderPrivate.Protocol.UNDEFINED,
        'INVALID_HANDLE');
    await mockChromeApi
        .dispatchEvent(
            'onConnectRequested', /*requestId=*/ 1001, releasedContext,
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            chrome.smartCardProviderPrivate.ShareMode.DIRECT,
            /*preferredProtocols=*/ {})
        .$waitAndVerify();
  },
};

const CONNECTED_READER_DISCONNECT_ON_TEARDOWN_TEST_CASES = {
  // Test Disconnect succeeds with correct handle.
  'testSmoke': function() {},

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
    for (let requestId = 111;; requestId += 1) {
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
    for (let requestId = 111;; requestId += 1) {
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
  },

  // Test that GetAttrib succeeds for the `SCARD_ATTR_ATR_STRING` argument.
  'testGetAttrib': async function() {
    expectReportDataResult(
        /*requestId=*/ 111, SimulationConstants.COSMO_ID_70_ATR,
        /*resultCode=*/ 'SUCCESS');

    await mockChromeApi
        .dispatchEvent(
            'onGetAttribRequested', /*requestId=*/ 111, readerHandle,
            API.SCARD_ATTR_ATR_STRING)
        .$waitAndVerify();
  },

  // Test that SetAttrib fails for unsupported attributes.
  'testSetAttrib_error': async function() {
    expectReportPlainResult(/*requestId=*/ 111, 'INVALID_PARAMETER');

    await mockChromeApi
        .dispatchEvent(
            'onSetAttribRequested', /*requestId=*/ 111, readerHandle,
            API.SCARD_ATTR_ATR_STRING, /*data=*/ new ArrayBuffer(0))
        .$waitAndVerify();
  },

  // Test that SetAttrib works for supported attribute.
  'testSetAttrib_success': async function() {
    expectReportPlainResult(/*requestId=*/ 111, 'SUCCESS');

    // TAG_IFD_DEVICE_REMOVED is used with a value of 0 here. This is a
    // no-op. Nothing will happen to the reader. The goal is just to test
    // `SetAttrib` returning the `SUCCESS` value.
    await mockChromeApi
        .dispatchEvent(
            'onSetAttribRequested', /*requestId=*/ 111, readerHandle,
            TAG_IFD_DEVICE_REMOVED, /*data=*/ new Uint8Array(['0x0']))
        .$waitAndVerify();
  },

  // Test that BeginTransaction succeeds.
  'testBeginTransaction': async function() {
    expectReportPlainResult(/*requestId=*/ 111, 'SUCCESS');

    await mockChromeApi
        .dispatchEvent(
            'onBeginTransactionRequested', /*requestId=*/ 111, readerHandle)
        .$waitAndVerify();

    // The test `tearDown()` will verify that the disconnection works
    // despite the unended transaction.
  },

  // Test that EndTransaction succeeds after transaction has begun.
  'testEndTransaction_success': async function() {
    // Start transaction
    expectReportPlainResult(/*requestId=*/ 111, 'SUCCESS');
    await mockChromeApi
        .dispatchEvent(
            'onBeginTransactionRequested', /*requestId=*/ 111, readerHandle)
        .$waitAndVerify();

    expectReportPlainResult(/*requestId=*/ 112, 'SUCCESS');
    await mockChromeApi
        .dispatchEvent(
            'onEndTransactionRequested', /*requestId=*/ 112, readerHandle,
            chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
        .$waitAndVerify();
  },

  // Test that EndTransaction fails without BeginTransaction.
  'testEndTransaction_fail': async function() {
    expectReportPlainResult(/*requestId=*/ 111, 'NOT_TRANSACTED');
    await mockChromeApi
        .dispatchEvent(
            'onEndTransactionRequested', /*requestId=*/ 111, readerHandle,
            chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
        .$waitAndVerify();
  },

  // Test that Control succeeds for the `IOCTL_FEATURE_IFD_PIN_PROPERTIES`
  // command and returns the properties.
  'testControl_success': async function() {
    expectReportDataResult(
        /*requestId=*/ 111, SimulationConstants.PIN_PROPERTIES_STRUCTURE,
        'SUCCESS');
    await mockChromeApi
        .dispatchEvent(
            'onControlRequested', /*requestId=*/ 111, readerHandle,
            IOCTL_FEATURE_IFD_PIN_PROPERTIES, new ArrayBuffer(0))
        .$waitAndVerify();
  },

  // Test that Control fails for the `IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE`
  // command.
  'testControl_fail': async function() {
    expectReportDataResult(
        /*requestId=*/ 111, new ArrayBuffer(0), 'NOT_TRANSACTED');
    await mockChromeApi
        .dispatchEvent(
            'onControlRequested', /*requestId=*/ 111, readerHandle,
            IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE, new ArrayBuffer(0))
        .$waitAndVerify();
  },
};

const TERMINATED_READER_CONNECTION_TEST_CASES = {
  // Test that Disconnect requested on an already terminated connection
  // returns INVALID_HANDLE error.
  'testDisconnect': async function() {
    expectReportPlainResult(/*requestId=*/ 201, 'INVALID_HANDLE');

    await mockChromeApi
        .dispatchEvent(
            'onDisconnectRequested', /*requestId=*/ 201, readerHandle,
            chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
        .$waitAndVerify();
  },

  // Test that Status requested on an already terminated connection
  // returns INVALID_HANDLE error.
  'testStatus': async function() {
    expectReportStatusResult(
        /*requestId=*/ 201, /*readerName=*/ '',
        chrome.smartCardProviderPrivate.ConnectionState.ABSENT,
        chrome.smartCardProviderPrivate.Protocol.UNDEFINED, new ArrayBuffer(0),
        'INVALID_HANDLE');

    await mockChromeApi
        .dispatchEvent('onStatusRequested', /*requestId=*/ 201, readerHandle)
        .$waitAndVerify();
  },

  // Test that GetAttrib requested on an already terminated connection
  // returns INVALID_HANDLE error.
  'testGetAttrib': async function() {
    expectReportDataResult(
        /*requestId=*/ 201, new ArrayBuffer(0),
        /*resultCode=*/ 'INVALID_HANDLE');

    await mockChromeApi
        .dispatchEvent(
            'onGetAttribRequested', /*requestId=*/ 201, readerHandle,
            API.SCARD_ATTR_ATR_STRING)
        .$waitAndVerify();
  },

  // Test that SetAttrib requested on an already terminated connection
  // returns INVALID_HANDLE error.
  'testSetAttrib': async function() {
    expectReportPlainResult(/*requestId=*/ 201, 'INVALID_HANDLE');

    await mockChromeApi
        .dispatchEvent(
            'onSetAttribRequested', /*requestId=*/ 201, readerHandle,
            TAG_IFD_DEVICE_REMOVED, /*data=*/ new Uint8Array(['0x0']))
        .$waitAndVerify();
  },

  // Test that BeginTransaction requested on an already terminated
  // connection returns INVALID_HANDLE error.
  'testBeginTransaction': async function() {
    expectReportPlainResult(/*requestId=*/ 201, 'INVALID_HANDLE');

    await mockChromeApi
        .dispatchEvent(
            'onBeginTransactionRequested', /*requestId=*/ 201, readerHandle)
        .$waitAndVerify();
  },

  // Test that EndTransaction requested on an already terminated connection
  // returns INVALID_HANDLE error.
  'testEndTransaction': async function() {
    expectReportPlainResult(/*requestId=*/ 201, 'INVALID_HANDLE');

    await mockChromeApi
        .dispatchEvent(
            'onEndTransactionRequested', /*requestId=*/ 201, readerHandle,
            chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
        .$waitAndVerify();
  },

  // Test that Control requested on an already terminated connection
  // returns INVALID_HANDLE error.
  'testControl': async function() {
    expectReportDataResult(
        /*requestId=*/ 201, new ArrayBuffer(0), 'INVALID_HANDLE');

    await mockChromeApi
        .dispatchEvent(
            'onControlRequested', /*requestId=*/ 201, readerHandle,
            IOCTL_FEATURE_IFD_PIN_PROPERTIES, new ArrayBuffer(0))
        .$waitAndVerify();
  },
};

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
      if (testController) {
        await testController.disposeAsync();
      }
      if (pcscReadinessTracker) {
        pcscReadinessTracker.dispose();
      }
      if (chromeApiProvider) {
        assertTrue(chromeApiProvider.isDisposed());
      }
      if (mockControl) {
        // Check all mock expectations are satisfied.
        mockControl.$verifyAll();
      }
    } finally {
      chromeApiProvider = null;
      mockControl = null;
      pcscReadinessTracker = null;
      testController = null;
    }
  },

  ...SIMPLE_TEST_CASES,

  // Tests that check that requests on an already released context return
  // INVALID_HANDLE.
  'testReleasedContext': {
    'setUp': async function() {
      await launchPcscServer(/*initialDevices=*/[]);
      // Establish and release context.
      releasedContext = await establishContext(/*requestId=*/ 123);
      expectReportReleaseContext(/*requestId=*/ 124, 'SUCCESS');
      await mockChromeApi
          .dispatchEvent(
              'onReleaseContextRequested', /*requestId=*/ 124, releasedContext)
          .$waitAndVerify();
    },

    'tearDown': function() {
      releasedContext = null;
    },

    ...RELEASED_CONTEXT_TEST_CASES,
  },

  // Tests that check that requests on an already released context return
  // INVALID_HANDLE. Tests with a reader connected.
  'testReleasedContextWithReader': {
    'setUp': async function() {
      await launchPcscServer(/*initialDevices=*/[]);

      const sCardContext = await establishContext(/*requestId=*/ 123);

      // Simulate reader connection and wait until PCSC finishes reader
      // initialization.
      expectReportGetStatusChange(
          /*requestId=*/ 124, [{
            'reader': PNP_NOTIFICATION,
            'eventState': {'changed': true},
            'eventCount': 0,
            'atr': new ArrayBuffer(0)
          }],
          'SUCCESS');
      const verifyResult = mockChromeApi
                               .dispatchEvent(
                                   'onGetStatusChangeRequested',
                                   /*requestId=*/ 124, sCardContext,
                                   /*timeout=*/ {}, [{
                                     'reader': PNP_NOTIFICATION,
                                     'currentState': {'unaware': true},
                                     'currentCount': 0
                                   }])
                               .$waitAndVerify();
      await setSimulatedDevices(
          [{'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}]);
      await verifyResult;

      expectReportReleaseContext(/*requestId=*/ 125, 'SUCCESS');
      await mockChromeApi
          .dispatchEvent(
              'onReleaseContextRequested', /*requestId=*/ 125, sCardContext)
          .$waitAndVerify();
      releasedContext = sCardContext;
    },

    'tearDown': function() {
      releasedContext = null;
    },

    ...RELEASED_CONTEXT_WITH_READER_TEST_CASES,
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

    'tearDown': function() {
      readerHandle = null;
    },

    'testDisconnectOnTearDown': {
      'tearDown': async function() {
        expectReportPlainResult(/*requestId=*/ 68, 'SUCCESS');
        await mockChromeApi
            .dispatchEvent(
                'onDisconnectRequested', /*requestId=*/ 68, readerHandle,
                chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
            .$waitAndVerify();
      },

      ...CONNECTED_READER_DISCONNECT_ON_TEARDOWN_TEST_CASES,
    },

    // Tests that check that requests on an already terminated reader connection
    // return INVALID_HANDLE.
    'testTerminatedConnection': {
      'setUp': async function() {
        expectReportPlainResult(/*requestId=*/ 101, 'SUCCESS');
        await mockChromeApi
            .dispatchEvent(
                'onDisconnectRequested', /*requestId=*/ 101, readerHandle,
                chrome.smartCardProviderPrivate.Disposition.LEAVE_CARD)
            .$waitAndVerify();
      },

      ...TERMINATED_READER_CONNECTION_TEST_CASES,
    },
  },
});
});  // goog.scope
