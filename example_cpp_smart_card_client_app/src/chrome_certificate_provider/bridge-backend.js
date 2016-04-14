/** @license
 * Copyright 2016 Google Inc. All Rights Reserved.
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
 * @fileoverview JavaScript-side backend for the bridge between JavaScript
 * chrome.certificateProvider API (see
 * <https://developer.chrome.com/extensions/certificateProvider>) and the NaCl
 * module handlers.
 */

goog.provide('SmartCardClientApp.CertificateProviderBridge.Backend');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Disposable');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var REQUESTER_NAME = 'certificate_provider_bridge';

/** @const */
var HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME = 'HandleCertificatesRequest';

/** @const */
var HANDLE_SIGN_DIGEST_REQUEST_FUNCTION_NAME = 'HandleSignDigestRequest';

/** @const */
var GSC = GoogleSmartCard;

/**
 * JavaScript-side backend for the bridge between JavaScript
 * chrome.certificateProvider API (see
 * <https://developer.chrome.com/extensions/certificateProvider>) and the NaCl
 * module handlers.
 *
 * Once the NaCl module is loaded, this object subscribes itself for listening
 * the chrome.certificateProvider API events and translates them into requests
 * sent to the NaCl module as messages. The response messages from the NaCl
 * module, once they are received, trigger the corresponding callbacks provided
 * by the chrome.certificateProvider API.
 * @param {!GSC.NaclModule} naclModule
 * @extends goog.Disposable
 * @constructor
 */
SmartCardClientApp.CertificateProviderBridge.Backend = function(naclModule) {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getLogger(
      'SmartCardClientApp.CertificateProviderBridge.Backend<"' +
      naclModule.naclModulePath + '">');

  /** @private */
  this.requester_ = new GSC.Requester(
      REQUESTER_NAME, naclModule.messageChannel);

  /** @private */
  this.boundCertificatesRequestListener_ = null;

  /** @private */
  this.boundSignDigestRequestListener_ = null;

  naclModule.addLoadEventListener(this.naclModuleLoadedListener_.bind(this));
  naclModule.addOnDisposeCallback(this.naclModuleDisposedListener_.bind(this));

  this.logger.fine('Constructed');
};

/** @const */
var Backend = SmartCardClientApp.CertificateProviderBridge.Backend;

goog.inherits(Backend, goog.Disposable);

if (goog.DEBUG) {
  /**
   * Debug-only function that allows to inject a certificates request (that is
   * otherwise generated as a result of the chrome.certificateProvider event -
   * see
   * <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>).
   * @param {function(!Array.<chrome.certificateProvider.CertificateInfo>=)}
   * reportCallback
   */
  Backend.prototype.dispatchCertificatesRequest = function(reportCallback) {
    this.certificatesRequestListener_(reportCallback);
  };

  /**
   * Debug-only function that allows to inject a data signing request (that is
   * otherwise generated as a result of the chrome.certificateProvider event -
   * see
   * <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>).
   * @param {!chrome.certificateProvider.SignRequest} request
   * @param {function(!ArrayBuffer=)} reportCallback
   */
  Backend.prototype.dispatchSignRequest = function(request, reportCallback) {
    this.signDigestRequestListener_(request, reportCallback);
  };
}

/** @override */
Backend.prototype.disposeInternal = function() {
  if (this.boundSignDigestRequestListener_) {
    chrome.certificateProvider.onSignDigestRequested.removeListener(
        this.boundSignDigestRequestListener_);
    this.boundSignDigestRequestListener_ = null;
  }

  if (this.boundCertificatesRequestListener_) {
    chrome.certificateProvider.onCertificatesRequested.removeListener(
        this.boundCertificatesRequestListener_);
    this.boundCertificatesRequestListener_ = null;
  }

  this.requester_.dispose();
  this.requester_ = null;

  this.logger.fine('Disposed');

  Backend.base(this, 'disposeInternal');
};

/** @private */
Backend.prototype.naclModuleLoadedListener_ = function() {
  this.logger.fine('NaCl module has loaded, setting up ' +
                   'chrome.certificateProvider API listeners...');
  this.setupApiListeners_();
};

/** @private */
Backend.prototype.naclModuleDisposedListener_ = function() {
  this.logger.fine('NaCl module was disposed, disposing...');
  this.dispose();
};

/** @private */
Backend.prototype.setupApiListeners_ = function() {
  if (goog.DEBUG && !chrome.certificateProvider) {
    this.logger.warning(
        'chrome.certificateProvider API is not available. Providing ' +
        'certificates to the Chrome browser will be impossible. This is just ' +
        'a warning in the Debug build (in order to make some testing on ' +
        'non-Chrome OS systems possible), but this will be a fatal error in ' +
        'the Release build');
    return;
  }
  GSC.Logging.checkWithLogger(
      this.logger,
      chrome.certificateProvider,
      'chrome.certificateProvider API is not available');

  this.boundCertificatesRequestListener_ =
      this.certificatesRequestListener_.bind(this);
  chrome.certificateProvider.onCertificatesRequested.addListener(
      this.boundCertificatesRequestListener_);

  this.boundSignDigestRequestListener_ =
      this.signDigestRequestListener_.bind(this);
  chrome.certificateProvider.onSignDigestRequested.addListener(
      this.boundSignDigestRequestListener_);
};

/**
 * @param {function((!Array.<!chrome.certificateProvider.CertificateInfo>),function(!Array.<!chrome.certificateProvider.CertificateInfo>))}
 * reportCallback
 * @private
 */
Backend.prototype.certificatesRequestListener_ = function(reportCallback) {
  this.logger.info('Started handling certificates request');
  var remoteCallMessage = new GSC.RemoteCallMessage(
      HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME, []);
  var promise = this.requester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  promise.then(function(results) {
    GSC.Logging.checkWithLogger(this.logger, results.length == 1);
    var certificates = goog.array.map(results[0], normalizeCertificateInfo);
    this.logger.info(
        'Responding to the certificates request with ' + certificates.length +
        ' certificates: ' + GSC.DebugDump.debugDump(certificates));
    reportCallback(certificates, this.rejectedCertificatesCallback_.bind(this));
  }, function() {
    this.logger.info('Responding to the certificates request with an error');
    reportCallback([], this.rejectedCertificatesCallback_.bind(this));
  }, this);
};

/**
 * @param {!chrome.certificateProvider.SignRequest} request
 * @param {function(!ArrayBuffer=)} reportCallback
 * @private
 */
Backend.prototype.signDigestRequestListener_ = function(
    request, reportCallback) {
  this.logger.info(
      'Started handling digest signing request. The request contents are: ' +
      'hash is "' + request.hash + '", digest is ' +
      GSC.DebugDump.debugDump(request.digest) + ', certificate is ' +
      GSC.DebugDump.debugDump(request.certificate));
  var remoteCallMessage = new GSC.RemoteCallMessage(
      HANDLE_SIGN_DIGEST_REQUEST_FUNCTION_NAME, [request]);
  var promise = this.requester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  promise.then(function(results) {
    GSC.Logging.checkWithLogger(this.logger, results.length == 1);
    var signature = normalizeSignature(results[0]);
    this.logger.info(
        'Responding to the digest sign request with the created signature: ' +
        GSC.DebugDump.debugDump(signature));
    reportCallback(signature);
  }, function() {
    this.logger.info('Responding to the digest sign request with an error');
    reportCallback(undefined);
  }, this);
};

/**
 * @param {!Array.<!chrome.certificateProvider.CertificateInfo>}
 * rejectedCertificates
 * @private
 */
Backend.prototype.rejectedCertificatesCallback_ = function(
    rejectedCertificates) {
  if (!rejectedCertificates.length)
    return;
  this.logger.warning(
      'chrome.certificateProvider API rejected ' + rejectedCertificates.length +
      ' certificates: ' + GSC.DebugDump.debugDump(rejectedCertificates));
};

/**
 * @param {!chrome.certificateProvider.CertificateInfo} certificateInfo
 * @return {!chrome.certificateProvider.CertificateInfo}
 */
function normalizeCertificateInfo(certificateInfo) {
  certificateInfo.certificate = (new Uint8Array(
      certificateInfo.certificate)).buffer;
  return certificateInfo;
}

/**
 * @param {!Array.<number>} signature
 * @return !ArrayBuffer
 */
function normalizeSignature(signature) {
  return (new Uint8Array(signature)).buffer;
}

});  // goog.scope
