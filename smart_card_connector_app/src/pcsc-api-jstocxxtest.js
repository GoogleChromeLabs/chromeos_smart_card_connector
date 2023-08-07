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
goog.require('GoogleSmartCard.MessageChannelPair');
goog.require('GoogleSmartCard.Pcsc.PermissionsChecker');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ReadinessTracker');
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

/** @type {GSC.IntegrationTestController?} */
let testController;
/** @type {ReadinessTracker?} */
let pcscReadinessTracker;
const stubPermissionsChecker = new StubPermissionsChecker();
/** @type {ClientHandler?} */
let clientHandler;
/** @type {API?} */
let api;

/**
 * Shorthand for obtaining the PC/SC context.
 * @return {!Promise<!API.SCARDCONTEXT>}
 */
async function establishContextOrThrow() {
  const result =
      await api.SCardEstablishContext(API.SCARD_SCOPE_SYSTEM, null, null);
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

goog.exportSymbol('testPcscApi', {
  'setUp': async function() {
    testController = new GSC.IntegrationTestController();
    await testController.initAsync();
    ClientHandler.overridePermissionsCheckerForTesting(stubPermissionsChecker);
    pcscReadinessTracker = new ReadinessTracker(
        testController.executableModule.getMessageChannel());
    await testController.setUpCppHelper(
        'SmartCardConnectorApplicationTestHelper', {});
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
    await pcscReadinessTracker.promise;
  },

  'testWithSingleClient': {
    'setUp': function() {
      const apiMessageChannelPair = new GSC.MessageChannelPair();
      clientHandler = new ClientHandler(
          testController.executableModule.getMessageChannel(),
          pcscReadinessTracker.promise, apiMessageChannelPair.getFirst(),
          /*clientOrigin=*/ undefined);
      api = new API(apiMessageChannelPair.getSecond());
    },

    'tearDown': function() {
      api.dispose();
      clientHandler.dispose();
    },

    // Test `SCardEstablishContext()`.
    'testSCardEstablishContext': async function() {
      const result =
          await api.SCardEstablishContext(API.SCARD_SCOPE_SYSTEM, null, null);
      let sCardContext;
      result.get(
          (context) => {
            sCardContext = context;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assert(Number.isInteger(sCardContext));
    },

    // Test `SCardEstablishContext()` when it's called without providing
    // optional arguments.
    'testSCardEstablishContext_omittedOptionalArgs': async function() {
      const result = await api.SCardEstablishContext(API.SCARD_SCOPE_SYSTEM);
      let sCardContext;
      result.get(
          (context) => {
            sCardContext = context;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assert(Number.isInteger(sCardContext));
    },

    // Test `testSCardReleaseContext()` with the correct handle.
    'testSCardReleaseContext_correct': async function() {
      const context = await establishContextOrThrow();

      const result = await api.SCardReleaseContext(context);
      let called = false;
      result.get(
          () => {
            called = true;
          },
          (errorCode) => {
            fail(`Unexpected error ${errorCode}`);
          });
      assert(called);
    },

    // Test `testSCardReleaseContext()` fails on a non-existing handle.
    'testSCardReleaseContext_nonExisting': async function() {
      const BAD_CONTEXT = 123;

      const result = await api.SCardReleaseContext(BAD_CONTEXT);
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
    },

    // Test `testSCardReleaseContext()` fails on a handle different from
    // currently obtained one.
    'testSCardReleaseContext_different': async function() {
      const context = await establishContextOrThrow();
      const badContext = context ^ 1;

      const result = await api.SCardReleaseContext(badContext);
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
    },
  },
});
});  // goog.scope
