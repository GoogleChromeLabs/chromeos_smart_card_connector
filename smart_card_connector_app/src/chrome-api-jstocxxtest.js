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
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
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

const PNP_NOTIFICATION = GSC.PcscLiteCommon.Constants.PNP_NOTIFICATION;

/**
 * @typedef {{sCardContext: number}}
 */
let EstablishContextResult;

const EMPTY_CONTEXT_RESULT = {
  'sCardContext': 0
};

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

/**
 * @param {!Array} initialDevices
 * @return {!Promise}
 */
async function launchPcscServer(initialDevices) {
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
 * Sets expectation that reportEstablishContextResult will be called for given
 * `requestId` with result code equal to `resultCode`. Sets the received result
 * to `outResult`.
 * @param {number} requestId
 * @param {string} resultCode
 * @param {!EstablishContextResult} outResult
 */
function expectReportEstablishContext(requestId, resultCode, outResult) {
  chrome
      .smartCardProviderPrivate['reportEstablishContextResult'](
          requestId, goog.testing.mockmatchers.isNumber, resultCode)
      .$once()
      .$does((requestId, context, resultCode) => {
        outResult.sCardContext = context;
      });
}

/**
 * Sets expectation that reportEstablishContextResult will be called for given
 * `requestId` with result code equal to `resultCode`.
 * @param {number} requestId
 * @param {string} resultCode
 */
function expectReportReleaseContext(requestId, resultCode) {
  chrome
      .smartCardProviderPrivate['reportReleaseContextResult'](
          requestId, resultCode)
      .$once();
}

/**
 * Sets expectation that reportListReadersResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {!Array.<string>} readers
 * @param {string} resultCode
 */
function expectReportListReaders(requestId, readers, resultCode) {
  chrome
      .smartCardProviderPrivate['reportListReadersResult'](
          requestId, readers, resultCode)
      .$once();
}

/**
 * Sets expectation that reportGetStatusChangeResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {!Array<!chrome.smartCardProviderPrivate.ReaderStateOut>} readerStates
 * @param {string} resultCode
 */
function expectReportGetStatusChange(requestId, readerStates, resultCode) {
  chrome
      .smartCardProviderPrivate['reportGetStatusChangeResult'](
          requestId, readerStates, resultCode)
      .$once();
}

/**
 * Sets expectation that reportPlaintResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {string} resultCode
 */
function expectReportPlainResult(requestId, resultCode) {
  chrome.smartCardProviderPrivate['reportPlainResult'](requestId, resultCode)
      .$once();
}

/**
 * Sets expectation that reportConnectResult will be called with given
 * parameters.
 * @param {number} requestId
 * @param {!chrome.smartCardProviderPrivate.Protocol} activeProtocol
 * @param {string} resultCode
 */
function expectReportConnectResult(requestId, activeProtocol, resultCode) {
  chrome
      .smartCardProviderPrivate['reportConnectResult'](
          requestId,
          /*scardHandle=*/ goog.testing.mockmatchers.isNumber, activeProtocol,
          resultCode)
      .$once();
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
    mockControl.$replayAll();
    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    await pcscReadinessTracker.promise;
  },

  // Test a single `onEstablishContextRequested` event.
  'testEstablishContext': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
  },

  // Test releasing the established context.
  'testReleaseContext': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportReleaseContext(/*requestId=*/ 124, 'SUCCESS');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onReleaseContextRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext)
        .$waitAndVerify();
  },

  // Test that releasing bogus context returns INVALID_HANDLE error.
  'testReleaseContext_none': async function() {
    const BAD_CONTEXT = 123;
    expectReportReleaseContext(/*requestId=*/ 42, 'INVALID_HANDLE');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent(
            'onReleaseContextRequested', /*requestId=*/ 42, BAD_CONTEXT)
        .$waitAndVerify();
  },

  // Test ListReaders with no readers attached.
  'testListReaders_none': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportListReaders(/*requestId=*/ 124, [], 'NO_READERS_AVAILABLE');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext)
        .$waitAndVerify();
  },

  // Test that ListReaders requested with already released context returns
  // INVALID_HANDLE error.
  'testListReaders_releasedContext': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportReleaseContext(/*requestId=*/ 124, 'SUCCESS');
    expectReportListReaders(/*requestId=*/ 125, [], 'INVALID_HANDLE');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onReleaseContextRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 125,
            establishContextResult.sCardContext)
        .$waitAndVerify();
  },

  // Test ListReaders returns a one-item list when there's a single attached
  // device.
  'testListReaders_singleDevice': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportListReaders(
        /*requestId=*/ 124, ['Gemalto PC Twin Reader 00 00'], 'SUCCESS');
    mockControl.$replayAll();

    launchPcscServer(
        /*initialDevices=*/[
          {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
        ]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext)
        .$waitAndVerify();
  },


  // Test ListReaders returns a two-item list when there're two attached device.
  'testListReaders_twoDevices': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportListReaders(
        /*requestId=*/ 124,
        [
          'Gemalto PC Twin Reader 00 00',
          'Dell Dell Smart Card Reader Keyboard 01 00'
        ],
        'SUCCESS');
    mockControl.$replayAll();

    launchPcscServer(
        /*initialDevices=*/[
          {'id': 101, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE},
          {'id': 102, 'type': SimulationConstants.DELL_DEVICE_TYPE}
        ]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onListReadersRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext)
        .$waitAndVerify();
  },

  // Test GetStatusChange with `\\?PnP?\Notification` to get the reader added
  // event.
  'testGetStatusChange_deviceAppearing': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportGetStatusChange(
        /*requestId=*/ 124, [{
          'reader': PNP_NOTIFICATION,
          'eventState': {'changed': true},
          'eventCount': 0,
          'atr': new ArrayBuffer(0)
        }],
        'SUCCESS');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    const verifyResult =
        mockChromeApi
            .dispatchEvent(
                'onGetStatusChangeRequested', /*requestId=*/ 124,
                establishContextResult.sCardContext, /*timeout*/ {}, [{
                  'reader': PNP_NOTIFICATION,
                  'currentState': {'unaware': true},
                  'currentCount': 0
                }])
            .$waitAndVerify();
    setSimulatedDevices(
        [{'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}]);
    await verifyResult;
  },

  // Test GetStatusChange returns the reader and card information.
  'testGetStatusChange_withCardInitially': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportGetStatusChange(
        /*requestId=*/ 124, [{
          'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
          'eventState': {'changed': true, 'present': true},
          'eventCount': 0,
          'atr': SimulationConstants.COSMO_ID_70_ATR
        }],
        'SUCCESS');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onGetStatusChangeRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext, /*timeout*/ {}, [{
              'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
              'currentState': {'unknown': true},
              'currentCount': 0
            }])
        .$waitAndVerify();
  },

  // Test GetStatusChange detects when a card is removed.
  'testGetStatusChange_cardRemoving': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportGetStatusChange(
        /*requestId=*/ 124, [{
          'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
          'eventState': {'changed': true, 'empty': true},
          /*corresponds to the number of card insertions/removals*/
          'eventCount': 1,
          'atr': new ArrayBuffer(0)
        }],
        'SUCCESS');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    const verifyResult =
        mockChromeApi
            .dispatchEvent(
                'onGetStatusChangeRequested', /*requestId=*/ 124,
                establishContextResult.sCardContext, /*timeout*/ {}, [{
                  'reader':
                      SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
                  'currentState': {'present': true},
                  'currentCount': 0
                }])
            .$waitAndVerify();
    // Simulate the card removal.
    setSimulatedDevices(
        [{'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}]);
    await verifyResult;
  },

  // Test GetStatusChange with timeout parameter will return even if status
  // is not changing.
  'testGetStatusChange_withTimeout': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportGetStatusChange(
        /*requestId=*/ 124, [], 'TIMEOUT');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[{
      'id': 123,
      'type': SimulationConstants.GEMALTO_DEVICE_TYPE,
      'cardType': SimulationConstants.COSMO_CARD_TYPE
    }]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onGetStatusChangeRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext,
            /*timeout*/ {'milliseconds': 10}, [{
              'reader': SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
              'currentState': {'present': true},
              'currentCount': 0
            }])
        .$waitAndVerify();
  },

  // Test Cancel terminates a running GetStatusChange call.
  'testCancel': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportGetStatusChange(/*requestId=*/ 124, [], 'CANCELLED');
    expectReportPlainResult(/*requestId=*/ 125, 'SUCCESS');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[
      {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
    ]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    const statusPromise =
        mockChromeApi
            .dispatchEvent(
                'onGetStatusChangeRequested', /*requestId=*/ 124,
                establishContextResult.sCardContext,
                /*timeout*/ {}, [{
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
        .dispatchEvent(
            'onCancelRequested', /*requestId=*/ 125,
            establishContextResult.sCardContext)
        .$waitAndVerify();
    // Wait until GetStatusChange returns and result is verified.
    await statusPromise;
  },

  // Test Connect fails when there's no card inserted.
  'testConnect_errorNoCard': async function() {
    const establishContextResult = EMPTY_CONTEXT_RESULT;
    expectReportEstablishContext(
        /*requestId=*/ 123, 'SUCCESS', establishContextResult);
    expectReportConnectResult(
        /*requestId=*/ 124, chrome.smartCardProviderPrivate.Protocol.UNDEFINED,
        'NO_SMARTCARD');
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[
      {'id': 123, 'type': SimulationConstants.GEMALTO_DEVICE_TYPE}
    ]);
    createChromeApiProvider();
    await mockChromeApi
        .dispatchEvent('onEstablishContextRequested', /*requestId=*/ 123)
        .$waitAndVerify();
    await mockChromeApi
        .dispatchEvent(
            'onConnectRequested', /*requestId=*/ 124,
            establishContextResult.sCardContext,
            SimulationConstants.GEMALTO_PC_TWIN_READER_PCSC_NAME0,
            chrome.smartCardProviderPrivate.ShareMode.SHARED,
            /*preferredProtocols*/ {'t0': true, 't1': true})
        .$waitAndVerify();
  }
});
});  // goog.scope
