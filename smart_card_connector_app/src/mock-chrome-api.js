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
 * @fileoverview This file contains a mock of chrome.smartCardProviderPrivate
 * API.
 */

goog.provide('GoogleSmartCard.ConnectorApp.MockChromeApi');

goog.require('goog.Disposable');
goog.require('goog.asserts');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.StrictMock');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

/**
 * Contains the names of all events exposed by chrome.smartCardProviderPrivate
 * API.
 */
const EXTENSION_API_EVENTS = [
  'onEstablishContextRequested',
  'onReleaseContextRequested',
  'onListReadersRequested',
  'onGetStatusChangeRequested',
  'onCancelRequested',
  'onConnectRequested',
  'onDisconnectRequested',
  'onTransmitRequested',
  'onControlRequested',
  'onGetAttribRequested',
  'onSetAttribRequested',
  'onStatusRequested',
  'onBeginTransactionRequested',
  'onEndTransactionRequested',
];

/**
 * Contains the names of all functions exposed by
 * chrome.smartCardProviderPrivate API.
 */
const EXTENSION_FUNCTIONS = [
  'reportEstablishContextResult',
  'reportReleaseContextResult',
  'reportListReadersResult',
  'reportGetStatusChangeResult',
  'reportPlainResult',
  'reportConnectResult',
  'reportDataResult',
  'reportStatusResult',
];

/**
 * Contains the ResultCode enum values exposed by
 * chrome.smartCardProviderPrivate API.
 */
const RESULT_CODES = [
  'SUCCESS',
  'REMOVED_CARD',
  'RESET_CARD',
  'UNPOWERED_CARD',
  'UNRESPONSIVE_CARD',
  'UNSUPPORTED_CARD',
  'READER_UNAVAILABLE',
  'SHARING_VIOLATION',
  'NOT_TRANSACTED',
  'NO_SMARTCARD',
  'PROTO_MISMATCH',
  'SYSTEM_CANCELLED',
  'NOT_READY',
  'CANCELLED',
  'INSUFFICIENT_BUFFER',
  'INVALID_HANDLE',
  'INVALID_PARAMETER',
  'INVALID_VALUE',
  'NO_MEMORY',
  'TIMEOUT',
  'UNKNOWN_READER',
  'UNSUPPORTED_FEATURE',
  'NO_READERS_AVAILABLE',
  'SERVICE_STOPPED',
  'NO_SERVICE',
  'COMM_ERROR',
  'INTERNAL_ERROR',
  'UNKNOWN_ERROR',
  'SERVER_TOO_BUSY',
  'UNEXPECTED',
  'SHUTDOWN',
  'UNKNOWN'
];

/**
 * Sets up mocks for
 * chrome.smartCardProviderPrivate.*.addListener()/removeListener(),
 * report*Result functions. Checks that all events have a subscriber and that
 * this subscriber is removed properly. Checks that all the expectations are
 * satisfied on disposal.
 */
GoogleSmartCard.ConnectorApp.MockChromeApi = class extends goog.Disposable {
  /**
   * @param {!goog.testing.PropertyReplacer} propertyReplacer
   */
  constructor(propertyReplacer) {
    super();
    /** @type {Object.<string, Function>} */
    this.listeners_ = {};
    this.listenersMockControl_ = new goog.testing.MockControl();
    for (const event of EXTENSION_API_EVENTS) {
      // Mock chrome.smartCardProviderPrivate.*.addListener()/removeListener().
      // Cast is to work around the issue that the return type of
      // createFunctionMock() is too generic (as it depends on the value of the
      // optional second argument).
      const mockedAddListener = /** @type {!goog.testing.StrictMock} */ (
          this.listenersMockControl_.createFunctionMock(
              `${event}.addListener`));
      propertyReplacer.setPath(
          `chrome.smartCardProviderPrivate.${event}.addListener`,
          mockedAddListener);
      const mockedRemoveListener = /** @type {!goog.testing.StrictMock} */ (
          this.listenersMockControl_.createFunctionMock(
              `${event}.removeListener`));
      propertyReplacer.setPath(
          `chrome.smartCardProviderPrivate.${event}.removeListener`,
          mockedRemoveListener);

      // Check that a listener will be added to the event and the same listener
      // will be removed.
      chrome.smartCardProviderPrivate[event].addListener(
          goog.testing.mockmatchers.isFunction);
      let listener;
      mockedAddListener.$once().$does((actual_listener) => {
        listener = actual_listener;
        this.listeners_[event] = listener;
      });

      const argumentMatcher =
          new goog.testing.mockmatchers.ArgumentMatcher(function(value) {
            return value === listener;
          }, 'verifySameListener');
      chrome.smartCardProviderPrivate[event]
          .removeListener(argumentMatcher)
          .$once();

      this.listenersMockControl_.$replayAll();
    }

    this.functionsMockControl_ = new goog.testing.MockControl();
    // Mock chrome.smartCardProviderPrivate.report*Result functions.
    for (const reportFunction of EXTENSION_FUNCTIONS) {
      // Cast is to work around the issue that the return type of
      // createFunctionMock() is too generic (as it depends on the value of the
      // optional second argument).
      const mockedReport = /** @type {!goog.testing.StrictMock} */ (
          this.functionsMockControl_.createFunctionMock(`${reportFunction}`));
      propertyReplacer.setPath(
          `chrome.smartCardProviderPrivate.${reportFunction}`, mockedReport);
    }

    // Mock ResultCode enum.
    for (const resultCode of RESULT_CODES) {
      propertyReplacer.setPath(
          `chrome.smartCardProviderPrivate.ResultCode.${resultCode}`,
          resultCode);
    }
  }

  /** @override */
  disposeInternal() {
    this.listenersMockControl_.$verifyAll();
    this.functionsMockControl_.$verifyAll();
    super.disposeInternal();
  }

  /**
   * Dispatches the specified chrome.smartCardProviderPrivate API event with
   * `args`.
   * @param {string} eventName
   * @param  {...*} args
   */
  async dispatchEvent(eventName, ...args) {
    goog.asserts.assertFunction(this.listeners_[eventName]);
    await this.listeners_[eventName](...args);
  }
};
});
