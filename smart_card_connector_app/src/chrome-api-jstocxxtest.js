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
/** @type {ChromeApiProvider?} */
let chromeApiProvider;
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
      chromeApiProvider.dispose();
      await testController.disposeAsync();
      pcscReadinessTracker.dispose();
    } finally {
      // Check all mock expectations are satisfied.
      mockControl.$verifyAll();
      chromeApiProvider = null;
      pcscReadinessTracker = null;
      testController = null;
    }
  },

  'testSmoke': async function() {
    mockControl.$replayAll();
    launchPcscServer(/*initialDevices=*/[]);
    chromeApiProvider = new ChromeApiProvider(
        testController.executableModule.getMessageChannel(),
        pcscReadinessTracker.promise);
    await pcscReadinessTracker.promise;
  }
});
});  // goog.scope
