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

goog.require('SmartCardClientApp.CertificateProviderBridge.Backend');
goog.require('GoogleSmartCard.IntegrationTestController');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.Promise');
goog.require('goog.promise.Resolver');
goog.require('goog.testing');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const CertificateProviderBridge = SmartCardClientApp.CertificateProviderBridge;

// Note: These constants have to be kept in sync with
// api_bridge_integration_test_helper.cc.
const FAKE_CERT_1_DER = new Uint8Array([1, 2, 3]);
const FAKE_CERT_1_ALGORITHMS = ['RSASSA_PKCS1_v1_5_SHA256'];
const FAKE_CERT_2_DER = new Uint8Array([4]);
const FAKE_CERT_2_ALGORITHMS =
    ['RSASSA_PKCS1_v1_5_SHA512', 'RSASSA_PKCS1_v1_5_SHA1'];

/** @type {GSC.IntegrationTestController?} */
let testController;
/** @type {CertificateProviderBridge.Backend?} */
let bridgeBackend;
/**
 * @type {!goog.promise.Resolver<!chrome.certificateProvider.SetCertificatesDetails>}
 */
let setCertificatesApiExpectation;

/**
 * Sets up stub implementation of the chrome.certificateProvider API.
 */
function setUpApiStubs() {
  setCertificatesApiExpectation = goog.Promise.withResolver();
  testController.propertyReplacer.set(chrome, 'certificateProvider', {
    'setCertificates': stubSetCertificatesApi,
  });
}

/**
 * Sets up stub implementation of the legacy chrome.certificateProvider API (the
 * one that existed before the setCertificates() method was introduced).
 */
function setUpLegacyApiStubs() {
  testController.propertyReplacer.set(chrome, 'certificateProvider', {});
}

/**
 * Stub implementation of the chrome.certificateProvider.setCertificates() API.
 * @param {!chrome.certificateProvider.SetCertificatesDetails} details
 * @param {function()=} callback
 */
function stubSetCertificatesApi(details, callback) {
  setCertificatesApiExpectation.resolve(details);
  if (callback)
    callback();
}

goog.exportSymbol('testChromeCertificateProviderApiBridge', {
  'setUp': async function() {
    testController = new GSC.IntegrationTestController();
    await testController.initAsync();
    bridgeBackend =
        new CertificateProviderBridge.Backend(testController.executableModule);
    await testController.setUpCppHelper(
        'ChromeCertificateProviderApiBridge', /*helperArgument=*/ {});
  },

  'tearDown': async function() {
    try {
      if (bridgeBackend) {
        bridgeBackend.dispose();
      }
      if (testController) {
        await testController.disposeAsync();
      }
    } finally {
      bridgeBackend = null;
      testController = null;
    }
  },

  'testSetCertificates_empty': async function() {
    setUpApiStubs();
    const request = testController.sendMessageToCppHelper(
        'ChromeCertificateProviderApiBridge',
        /*messageForHelper=*/ 'setCertificates_empty');
    const details = await setCertificatesApiExpectation.promise;
    assertObjectEquals(details, {'clientCertificates': []});
    // Wait until the whole request-response roundtrip finishes, to check
    // there's no error occurred there.
    await request;
  },

  'testSetCertificates_empty_legacyApi': async function() {
    setUpLegacyApiStubs();
    // Just verify that no crash happens.
    await testController.sendMessageToCppHelper(
        'ChromeCertificateProviderApiBridge',
        /*messageForHelper=*/ 'setCertificates_empty');
  },

  'testSetCertificates_fakeCerts': async function() {
    setUpApiStubs();
    const request = testController.sendMessageToCppHelper(
        'ChromeCertificateProviderApiBridge',
        /*messageForHelper=*/ 'setCertificates_fakeCerts');
    const details = await setCertificatesApiExpectation.promise;
    assertObjectEquals(details, {
      'clientCertificates': [
        {
          'certificateChain': [FAKE_CERT_1_DER.buffer],
          'supportedAlgorithms': FAKE_CERT_1_ALGORITHMS
        },
        {
          'certificateChain': [FAKE_CERT_2_DER.buffer],
          'supportedAlgorithms': FAKE_CERT_2_ALGORITHMS
        }
      ]
    });
    // Wait until the whole request-response roundtrip finishes, to check
    // there's no error occurred there.
    await request;
  },

  'testSetCertificates_noApi': async function() {
    // Note the missing setUpApiStubs() call.
    // Just verify that no crash happens.
    await testController.sendMessageToCppHelper(
        'ChromeCertificateProviderApiBridge',
        /*messageForHelper=*/ 'setCertificates_empty');
  },
});
});  // goog.scope
