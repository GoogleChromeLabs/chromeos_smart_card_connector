/** @license
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

/** @type {GSC.IntegrationTestController?} */
let testController;
/** @type {CertificateProviderBridge.Backend?} */
let bridgeBackend;
/** @type {!goog.promise.Resolver<!chrome.certificateProvider.SetCertificatesDetails>} */
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
  setUp: function() {
    testController = new GSC.IntegrationTestController();
    return testController.initAsync().then(() => {
      bridgeBackend = new CertificateProviderBridge.Backend(
          testController.naclModule);
      return testController.setUpCppHelper(
        'ChromeCertificateProviderApiBridge', /*helperArgument=*/{});
    });
  },

  tearDown: function() {
    return testController.disposeAsync().thenAlways(() => {
      testController = null;
    });
  },

  testSetCertificates_empty: function() {
    setUpApiStubs();
    testController.sendMessageToCppHelper(
        'ChromeCertificateProviderApiBridge',
        /*messageForHelper=*/'setCertificates_empty');
    return setCertificatesApiExpectation.promise.then((details) => {
      assertObjectEquals(details, {'clientCertificates': []});
    });
  },

  testSetCertificates_empty_legacyApi: function() {
    setUpLegacyApiStubs();
    // Just verify that no crash happens.
    return testController.sendMessageToCppHelper(
        'ChromeCertificateProviderApiBridge',
        /*messageForHelper=*/'setCertificates_empty');
  },
});

});  // goog.scope
