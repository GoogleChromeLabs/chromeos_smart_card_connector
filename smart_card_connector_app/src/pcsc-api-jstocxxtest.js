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
 *
 * TODO(emaxx): This is currently just a boilerplate. Real tests will be added
 * after needed helpers are implemented.
 */

goog.require('GoogleSmartCard.IntegrationTestController');
goog.require('GoogleSmartCard.MessageChannelPair');
goog.require('GoogleSmartCard.Pcsc.PermissionsChecker');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('goog.Promise');
goog.require('goog.testing');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;
const ReadinessTracker = GSC.PcscLiteServerClientsManagement.ReadinessTracker;

class StubPermissionsChecker extends GSC.Pcsc.PermissionsChecker {
  /** @override */
  check(clientOrigin) {
    return goog.Promise.resolve();
  }
}

/** @type {GSC.IntegrationTestController?} */
let testController;
/** @type {ReadinessTracker?} */
let pcscReadinessTracker;
const stubPermissionsChecker = new StubPermissionsChecker();

goog.exportSymbol('testPcscApi', {
  'setUp': async function() {
    testController = new GSC.IntegrationTestController();
    await testController.initAsync();
    ClientHandler.overridePermissionsCheckerForTesting(stubPermissionsChecker);
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
    await testController.setUpCppHelper(
        'SmartCardConnectorApplicationTestHelper', {});
    await pcscReadinessTracker.promise;
  },

  // Test `SCardEstablishContext()`.
  'testScardEstablishContext': async function() {
    await testController.setUpCppHelper(
        'SmartCardConnectorApplicationTestHelper', {});

    const apiMessageChannelPair = new GSC.MessageChannelPair();
    const clientHandler = new ClientHandler(
        testController.executableModule.getMessageChannel(),
        pcscReadinessTracker.promise, apiMessageChannelPair.getFirst(),
        /*clientOrigin=*/ undefined);
    const api = new GSC.PcscLiteClient.API(apiMessageChannelPair.getSecond());

    const result = await api.SCardEstablishContext(
        GSC.PcscLiteClient.API.SCARD_SCOPE_SYSTEM);
    let sCardContext;
    result.get(
        (context) => {
          sCardContext = context;
        },
        (errorCode) => {
          fail(`Unexpected error ${errorCode}`);
        });
    assert(Number.isInteger(sCardContext));

    api.dispose();
    clientHandler.dispose();
  },
});
});  // goog.scope
