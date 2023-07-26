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

goog.require('GoogleSmartCard.ConnectorApp.ChromeApiProvider');
goog.require('goog.Promise');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.StrictMock');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');
goog.require('goog.testing.messaging.MockMessageChannel');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ChromeApiProvider = GSC.ConnectorApp.ChromeApiProvider;

const propertyReplacer = new goog.testing.PropertyReplacer();
/** @type {!goog.testing.MockControl|undefined} */
let mockControl;
let mockServerMessageChannel;

goog.exportSymbol('testChromeApiProvider', {
  'setUp': function() {
    mockControl = new goog.testing.MockControl();
    mockServerMessageChannel =
        new goog.testing.messaging.MockMessageChannel(mockControl);
    for (const event
             of ['onEstablishContextRequested',
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
    ]) {
      // Mock chrome.smartCardProviderPrivate.*.addListener()/removeListener().
      // Cast is to work around the issue that the return type of
      // createFunctionMock() is too generic (as it depends on the value of the
      // optional second argument).
      const mockedAddListener = /** @type {!goog.testing.StrictMock} */ (
          mockControl.createFunctionMock(`${event}.addListener`));
      propertyReplacer.setPath(
          `chrome.smartCardProviderPrivate.${event}.addListener`,
          mockedAddListener);
      const mockedRemoveListener = /** @type {!goog.testing.StrictMock} */ (
          mockControl.createFunctionMock(`${event}.removeListener`));
      propertyReplacer.setPath(
          `chrome.smartCardProviderPrivate.${event}.removeListener`,
          mockedRemoveListener);

      // Check that a listener will be added to the event and the same listener
      // will be removed.
      chrome.smartCardProviderPrivate[event].addListener(
          goog.testing.mockmatchers.isFunction);
      let listener;
      mockedAddListener.$once().$does(
          (actual_listener) => {listener = actual_listener});
      const argumentMatcher =
          new goog.testing.mockmatchers.ArgumentMatcher(function(value) {
            return value === listener;
          }, 'verifySameListener');
      chrome.smartCardProviderPrivate[event].removeListener(argumentMatcher);
      mockedRemoveListener.$once();
    }
  },

  'tearDown': function() {
    // Check all mock expectations are satisfied.
    mockControl.$verifyAll();
  },

  'testSmoke': function() {
    mockControl.$replayAll();
    const chromeApiProvider =
        new ChromeApiProvider(mockServerMessageChannel, goog.Promise.resolve());
    chromeApiProvider.dispose();
  },
});
});  // goog.scope