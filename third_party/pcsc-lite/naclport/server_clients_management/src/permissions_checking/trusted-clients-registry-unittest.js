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

goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientInfo');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistryImpl');
goog.require('goog.testing');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');

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

/** @type {TrustedClientsRegistryImpl?} */
let registry;

/**
 * Test-only override of the `fetch()` API.
 *
 * The response contains a fake trusted clients JSON.
 * @param {string} url
 * @return {!Promise<!Response>}
 */
async function fakeFetch(url) {
  assertEquals(url, 'trusted_clients.json');
  const body = JSON.stringify(FAKE_TRUSTED_CLIENTS);
  return new Response(body);
}

goog.exportSymbol('testTrustedClientsRegistry', {
  'setUp': function() {
    TrustedClientsRegistryImpl.overrideFetchForTesting(fakeFetch);
    registry = new TrustedClientsRegistryImpl();
  },

  'tearDown': function() {
    registry = null;
    TrustedClientsRegistryImpl.overrideFetchForTesting(null);
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
