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
goog.require('GoogleSmartCard.ConnectorApp.MockChromeApi');
goog.require('goog.Promise');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.messaging.MockMessageChannel');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ChromeApiProvider = GSC.ConnectorApp.ChromeApiProvider;
const MockChromeApi = GSC.ConnectorApp.MockChromeApi;

const propertyReplacer = new goog.testing.PropertyReplacer();
/** @type {!goog.testing.MockControl|undefined} */
let mockControl;
let mockServerMessageChannel;
/** @type {MockChromeApi?} */
let mockChromeApi;

goog.exportSymbol('testChromeApiProvider', {
  'setUp': function() {
    mockControl = new goog.testing.MockControl();
    mockServerMessageChannel =
        new goog.testing.messaging.MockMessageChannel(mockControl);
    // Set up chrome API mocks and set expectations that each event will be
    // subscribed to by some listener.
    mockChromeApi = new MockChromeApi(mockControl, propertyReplacer);
  },

  'tearDown': function() {
    // Check all mock expectations are satisfied.
    mockControl.$verifyAll();

    propertyReplacer.reset();
  },

  'testSmoke': function() {
    mockControl.$replayAll();
    const chromeApiProvider =
        new ChromeApiProvider(mockServerMessageChannel, goog.Promise.resolve());
    chromeApiProvider.dispose();
  },
});
});  // goog.scope
