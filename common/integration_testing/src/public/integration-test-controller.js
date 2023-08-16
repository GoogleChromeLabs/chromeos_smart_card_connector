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

goog.require('GoogleSmartCard.EmscriptenModule');
goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.TestCase');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const INTEGRATION_TEST_REQUESTER_NAME = 'integration_test';

/**
 * Custom timeout for the tests that use the controller.
 *
 * The default timeout in Closure Library, 1 second as of now, is much too small
 * for tests involving loading nontrivial executable modules.
 */
const TIMEOUT_MILLISECONDS = 60000;

/**
 * Class that encapsulates setup/teardown/communication steps of a
 * JavaScript-and-C++ integration test.
 */
GSC.IntegrationTestController = class {
  constructor() {
    // Customize the current test timeout. Note that the framework will
    // automatically reset the timeout to the default value when the subsequent
    // test starts.
    goog.testing.TestCase.getActiveTestCase().promiseTimeout =
        TIMEOUT_MILLISECONDS;

    /** @type {!goog.testing.PropertyReplacer} @const */
    this.propertyReplacer = new goog.testing.PropertyReplacer;
    /** @type {!GSC.ExecutableModule} @const */
    this.executableModule = createExecutableModule();
    /** @type {!GSC.Requester} @private @const */
    this.executableModuleRequester_ = new GSC.Requester(
        INTEGRATION_TEST_REQUESTER_NAME,
        this.executableModule.getMessageChannel());
  }

  /**
   * @return {!goog.Promise<void>}
   */
  initAsync() {
    this.executableModule.startLoading();
    return this.executableModule.getLoadPromise();
  }

  async disposeAsync() {
    try {
      if (!this.executableModule.isDisposed()) {
        await this.callCpp_('TearDownAll', /*functionArguments=*/[]);
      }
    } finally {
      this.executableModuleRequester_.dispose();
      this.executableModule.dispose();
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
   * Sends a remote call request to the executable module and returns its
   * response via a promise.
   * @param {string} functionName
   * @param {!Array.<*>} functionArguments
   * @return {!goog.Promise}
   */
  callCpp_(functionName, functionArguments) {
    const remoteCallMessage =
        new GSC.RemoteCallMessage(functionName, functionArguments);
    return this.executableModuleRequester_.postRequest(
        remoteCallMessage.makeRequestPayload());
  }
};

/**
 * Loads the binary executable module depending on the toolchain configuration.
 * @return {!GSC.ExecutableModule}
 */
function createExecutableModule() {
  switch (GSC.ExecutableModule.TOOLCHAIN) {
    case GSC.ExecutableModule.Toolchain.PNACL:
      return new GSC.NaclModule(
          'integration_tests.nmf', GSC.NaclModule.Type.PNACL);
    case GSC.ExecutableModule.Toolchain.EMSCRIPTEN:
      return new GSC.EmscriptenModule('integration_tests');
  }
  GSC.Logging.fail(
      `Cannot load executable module: unknown toolchain ` +
      `${GSC.ExecutableModule.TOOLCHAIN}`);
  goog.asserts.fail();
}
});
