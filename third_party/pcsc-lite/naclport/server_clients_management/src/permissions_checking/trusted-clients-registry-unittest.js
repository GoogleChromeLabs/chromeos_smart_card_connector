/**
 * @license
 * Copyright 2019 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientInfo');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistryImpl');
goog.require('goog.Promise');
goog.require('goog.json');
goog.require('goog.net.HttpStatus');
goog.require('goog.testing');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.net.XhrIo');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const TrustedClientsRegistryImpl =
    GSC.PcscLiteServer.TrustedClientsRegistryImpl;

const FAKE_APP_1_ORIGIN = 'chrome-extension://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
const FAKE_APP_1_NAME = 'App Name 1';
const FAKE_APP_2_ORIGIN = `chrome-extension://bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb`;
const FAKE_TRUSTED_CLIENTS = {
  [FAKE_APP_1_ORIGIN]: {'name': FAKE_APP_1_NAME},
};

const propertyReplacer = new goog.testing.PropertyReplacer();
/** @type {TrustedClientsRegistryImpl?} */
let registry;

/**
 * Set up the mock for goog.net.XhrIo.
 *
 * This allows mocking out network requests.
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 */
function setUpXhrioMock(propertyReplacer) {
  propertyReplacer.setPath('goog.net.XhrIo.send', goog.testing.net.XhrIo.send);
}

/**
 * Simulates the response delivery for a previously created mock Xhrio request.
 *
 * The response contains a fake trusted clients JSON.
 */
function simulateXhrioResponse() {
  const sentXhrios = goog.testing.net.XhrIo.getSendInstances();
  assertEquals(1, sentXhrios.length);
  const xhrio = sentXhrios[0];
  assertEquals(
      xhrio.getLastUri(),
      'pcsc_lite_server_clients_management/known_client_apps.json');
  const response = goog.json.serialize(FAKE_TRUSTED_CLIENTS);
  xhrio.simulateResponse(goog.net.HttpStatus.OK, response);
}

goog.exportSymbol('testTrustedClientsRegistry', {
  'setUp': function() {
    const propertyReplacer = new goog.testing.PropertyReplacer();
    setUpXhrioMock(propertyReplacer);

    registry = new TrustedClientsRegistryImpl();

    simulateXhrioResponse();
  },

  'tearDown': function() {
    registry = null;
    propertyReplacer.reset();
  },

  'testGetByOrigin_success': async function() {
    const trustedClient = await registry.getByOrigin(FAKE_APP_1_ORIGIN);
    assertEquals(trustedClient.origin, FAKE_APP_1_ORIGIN);
    assertEquals(trustedClient.name, FAKE_APP_1_NAME);
  },

  'testGetByOrigin_failure': async function() {
    try {
      await registry.getByOrigin(FAKE_APP_2_ORIGIN);
    } catch (e) {
      // Expected branch - complete the test.
      return;
    }
    // Unexpected branch - abort the test.
    fail('Unexpected successful response');
  },

  'testTryGetByOrigins': async function() {
    const trustedClients =
        await registry.tryGetByOrigins([FAKE_APP_1_ORIGIN, FAKE_APP_2_ORIGIN]);
    assertEquals(trustedClients.length, 2);
    assertEquals(trustedClients[0].origin, FAKE_APP_1_ORIGIN);
    assertEquals(trustedClients[0].name, FAKE_APP_1_NAME);
    assertNull(trustedClients[1]);
  },
});
});  // goog.scope
