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

goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.ClientHandler');
goog.require('goog.Promise');
goog.require('goog.reflect');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.messaging.MockMessageChannel');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const ClientHandler = GSC.PcscLiteServerClientsManagement.ClientHandler;

const EXTENSION_ID_A = 'chrome-extension://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';

class StubPermissionsChecker extends GSC.Pcsc.PermissionsChecker {
  /** @override */
  check(clientOrigin) {
    return goog.Promise.resolve();
  }
}

/** @type {!goog.testing.MockControl|undefined} */
let mockControl;
let mockServerMessageChannel;
let mockClientMessageChannel;
const stubPermissionsChecker = new StubPermissionsChecker();

/**
 * @param {!goog.testing.messaging.MockMessageChannel} mockMessageChannel
 */
function disposeOfMockMessageChannel(mockMessageChannel) {
  mockMessageChannel.dispose();
  // Hack: call the protected disposeInternal() method in order to trigger the
  // disposal notifications; the MockMessageChannel.dispose() doesn't call
  // them.
  mockMessageChannel[goog.reflect.objectProperty(
      'disposeInternal', mockMessageChannel)]();
}

goog.exportSymbol('testClientHandler', {
  'setUp': function() {
    mockControl = new goog.testing.MockControl();
    mockServerMessageChannel =
        new goog.testing.messaging.MockMessageChannel(mockControl);
    mockClientMessageChannel =
        new goog.testing.messaging.MockMessageChannel(mockControl);

    ClientHandler.overridePermissionsCheckerForTesting(stubPermissionsChecker);
  },

  'tearDown': function() {
    // Check all mock expectations are satisfied.
    mockControl.$verifyAll();

    ClientHandler.overridePermissionsCheckerForTesting(null);
  },

  'testSmoke': function() {
    const handler = new ClientHandler(
        mockServerMessageChannel, goog.Promise.resolve(),
        mockClientMessageChannel, EXTENSION_ID_A);

    handler.dispose();
  },

  'testClientChannelDisposed': function() {
    const handler = new ClientHandler(
        mockServerMessageChannel, goog.Promise.resolve(),
        mockClientMessageChannel, EXTENSION_ID_A);
    disposeOfMockMessageChannel(mockClientMessageChannel);
    assert(
        'ClientHandler should dispose on client channel disposal',
        handler.isDisposed());
  },

  'testServerChannelDisposed': function() {
    const handler = new ClientHandler(
        mockServerMessageChannel, goog.Promise.resolve(),
        mockClientMessageChannel, EXTENSION_ID_A);
    disposeOfMockMessageChannel(mockServerMessageChannel);
    assert(
        'ClientHandler should dispose on server channel disposal',
        handler.isDisposed());
  },
});
});  // goog.scope
