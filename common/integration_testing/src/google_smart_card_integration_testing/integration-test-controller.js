/**
 * @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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

goog.provide('GoogleSmartCard.IntegrationTestController');

goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Promise');
goog.require('goog.testing.PropertyReplacer');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const NACL_MODULE_PATH = 'integration_tests.nmf';
const INTEGRATION_TEST_NACL_MODULE_REQUESTER_NAME = 'integration_test';

/**
 * Class that encapsulates setup/teardown/communication steps of a
 * JavaScript-and-C++ integration test.
 */
GSC.IntegrationTestController = class {
  constructor() {
    /** @type {!goog.testing.PropertyReplacer} @const */
    this.propertyReplacer = new goog.testing.PropertyReplacer;
    /** @type {!GSC.NaclModule} @const */
    this.naclModule =
        new GSC.NaclModule(NACL_MODULE_PATH, GSC.NaclModule.Type.PNACL);
    /** @type {!GSC.Requester} @private @const */
    this.naclModuleRequester_ = new GSC.Requester(
        INTEGRATION_TEST_NACL_MODULE_REQUESTER_NAME,
        this.naclModule.getMessageChannel());
  }

  /**
   * @return {!goog.Promise<void>}
   */
  initAsync() {
    this.naclModule.startLoading();
    return this.naclModule.getLoadPromise();
  }

  async disposeAsync() {
    try {
      if (!this.naclModule.isDisposed()) {
        await this.callCpp_('TearDownAll', /*functionArguments=*/[]);
      }
    }
    finally {
      this.naclModuleRequester_.dispose();
      this.naclModule.dispose();
      this.propertyReplacer.reset();
    }
  }

  /**
   * Tells the C++ side to initialize the given helper; reports the result of
   * the setup via the returned promise.
   * @param {string} helperName
   * @param {!Object} helperArgument
   * @return {!goog.Promise<void>}
   */
  setUpCppHelper(helperName, helperArgument) {
    return this.callCpp_(
        'SetUp', /*functionArguments=*/[helperName, helperArgument]);
  }

  /**
   * Sends a message to the given C++ helper; reports the result via a promise.
   * @param {string} helperName
   * @param {*} messageForHelper
   * @return {!goog.Promise<void>}
   */
  sendMessageToCppHelper(helperName, messageForHelper) {
    return this.callCpp_(
        'HandleMessage', /*functionArguments=*/[helperName, messageForHelper]);
  }

  /**
   * Sends a remote call request to the NaCl module and returns its response via
   * a promise.
   * @param {string} functionName
   * @param {!Array.<*>} functionArguments
   * @return {!goog.Promise}
   */
  callCpp_(functionName, functionArguments) {
    const remoteCallMessage =
        new GSC.RemoteCallMessage(functionName, functionArguments);
    return this.naclModuleRequester_.postRequest(
        remoteCallMessage.makeRequestPayload());
  }
};
});
