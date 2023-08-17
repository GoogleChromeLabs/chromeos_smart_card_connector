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
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
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

/**
 * @param {!Array} initialDevices
 * @return {!Promise}
 */
async function launchPcscServer(initialDevices) {
  await testController.setUpCppHelper(
      'SmartCardConnectorApplicationTestHelper', initialDevices);
}

function createChromeApiProvider() {
  // Note: the reference to the created provider is not stored anywhere,
  // because it manages its lifetime itself, based on the lifetime of the
  // passed message channel.
  new ChromeApiProvider(
      testController.executableModule.getMessageChannel(),
      pcscReadinessTracker.promise);
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
    } finally {
      // Check all mock expectations are satisfied.
      mockControl.$verifyAll();
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
    chrome
        .smartCardProviderPrivate['reportEstablishContextResult'](
            /*requestId=*/ 123,
            /*scardContext=*/ goog.testing.mockmatchers.isNumber, 'SUCCESS')
        .$once();
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    mockChromeApi.dispatchEvent(
        'onEstablishContextRequested', /*requestId=*/ 123);
    await chrome.smartCardProviderPrivate['reportEstablishContextResult']
        .$waitAndVerify();
  },

  // Test releasing the established context.
  'testReleaseContext': async function() {
    let sCardContext = 0;
    chrome
        .smartCardProviderPrivate['reportEstablishContextResult'](
            /*requestId=*/ 123,
            /*scardContext=*/ goog.testing.mockmatchers.isNumber, 'SUCCESS')
        .$once()
        .$does((requestId, context, resultCode) => {sCardContext = context});
    chrome
        .smartCardProviderPrivate['reportReleaseContextResult'](
            /*requestId=*/ 124, 'SUCCESS')
        .$once();
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    mockChromeApi.dispatchEvent(
        'onEstablishContextRequested', /*requestId=*/ 123);
    await chrome.smartCardProviderPrivate['reportEstablishContextResult']
        .$waitAndVerify();
    mockChromeApi.dispatchEvent(
        'onReleaseContextRequested', /*requestId=*/ 124, sCardContext);
    await chrome.smartCardProviderPrivate['reportReleaseContextResult']
        .$waitAndVerify();
  },

  // Test that releasing bogus context returns INVALID_HANDLE error.
  'testReleaseContext_none': async function() {
    const BAD_CONTEXT = 123;
    chrome
        .smartCardProviderPrivate['reportReleaseContextResult'](
            /*requestId=*/ 42, 'INVALID_HANDLE')
        .$once();
    mockControl.$replayAll();

    launchPcscServer(/*initialDevices=*/[]);
    createChromeApiProvider();
    mockChromeApi.dispatchEvent(
        'onReleaseContextRequested', /*requestId=*/ 42, BAD_CONTEXT);
    await chrome.smartCardProviderPrivate['reportReleaseContextResult']
        .$waitAndVerify();
  }
});
});  // goog.scope
