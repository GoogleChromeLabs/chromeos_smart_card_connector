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

goog.require('GoogleSmartCard.DeferredProcessor');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Disposable');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var NACL_INCOMING_REQUESTER_NAME = 'certificate_provider_nacl_incoming';

/** @const */
var NACL_OUTGOING_REQUESTER_NAME = 'certificate_provider_nacl_outgoing';

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
 * This object subscribes itself for listening the chrome.certificateProvider
 * API events and translates them into requests sent to the NaCl module as
 * messages. The response messages from the NaCl module, once they are received,
 * trigger the corresponding callbacks provided by the
 * chrome.certificateProvider API.
 *
 * Note that the requests to the NaCl module are delayed until it loads, but the
 * API listeners are set up straight away. This allows to handle the
 * chrome.certificateProvider API events correctly even if they were received
 * too early.
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
      NACL_INCOMING_REQUESTER_NAME, naclModule.messageChannel);

  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      NACL_OUTGOING_REQUESTER_NAME,
      naclModule.messageChannel,
      this.handleRequest_.bind(this));

  /** @private */
  this.boundCertificatesRequestListener_ =
      this.certificatesRequestListener_.bind(this);
  /** @private */
  this.boundSignDigestRequestListener_ =
      this.signDigestRequestListener_.bind(this);
  // Start listening for the chrome.certificateProvider API events straight
  // away.
  this.setupApiListeners_();

  /** @private */
  this.deferredProcessor_ = new GSC.DeferredProcessor(
      naclModule.getLoadPromise());
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
   * @param {function(!Array.<!chrome.certificateProvider.CertificateInfo>=)}
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
  chrome.certificateProvider.onSignDigestRequested.removeListener(
      this.boundSignDigestRequestListener_);

  chrome.certificateProvider.onCertificatesRequested.removeListener(
      this.boundCertificatesRequestListener_);

  this.requester_.dispose();
  this.requester_ = null;

  this.requestReceiver_ = null;

  this.logger.fine('Disposed');

  Backend.base(this, 'disposeInternal');
};

/**
 * Handles a remote call request received from the NaCl module.
 * @param {!Object} payload
 * @return {!goog.Promise}
 * @private
 */
Backend.prototype.handleRequest_ = function(payload) {
  var remoteCallMessage = GSC.RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the remote call message: ' +
        GSC.DebugDump.debugDump(payload));
  }

  var debugRepresentation = 'chrome.certificateProvider.' +
                            remoteCallMessage.getDebugRepresentation();
  this.logger.fine('Received a remote call request: ' + debugRepresentation);

  var promiseResolver = goog.Promise.withResolver();

  var apiFunction = chrome.certificateProvider[remoteCallMessage.functionName];
  if (apiFunction) {
    /** @preserveTry */
    try {
      remoteCallMessage.functionArguments.push(function() {
        if (chrome.runtime.lastError) {
          promiseResolver.reject(new Error(goog.object.get(
              chrome.runtime.lastError, 'message', 'Unknown error')));
          return;
        }
        promiseResolver.resolve(Array.prototype.slice.call(arguments));
      });
      apiFunction.apply(null, remoteCallMessage.functionArguments);
    } catch (exc) {
      promiseResolver.reject(exc);
    }
  } else {
    promiseResolver.reject(
        'No such function in the API: ' + remoteCallMessage.functionName +
        '. Please check that the extension is running on Chrome OS and that ' +
        'the Chrome version is sufficiently new.');
  }

  return promiseResolver.promise;
};

/** @private */
Backend.prototype.naclModuleDisposedListener_ = function() {
  this.logger.fine('NaCl module was disposed, disposing...');
  this.dispose();
};

/** @private */
Backend.prototype.setupApiListeners_ = function() {
  GSC.Logging.checkWithLogger(this.logger, !this.isDisposed());

  if (!chrome.certificateProvider) {
    if (goog.DEBUG) {
      this.logger.warning(
          'chrome.certificateProvider API is not available. Providing ' +
          'certificates to the Chrome browser will be impossible. This is ' +
          'just a warning in the Debug build (in order to make some testing ' +
          'on non-Chrome OS systems possible), but this will be a fatal ' +
          'error in the Release build');
    } else {
      this.logger.severe(
          'chrome.certificateProvider API is not available. Providing ' +
          'certificates to the Chrome browser will be impossible');
    }
    return;
  }

  chrome.certificateProvider.onCertificatesRequested.addListener(
      this.boundCertificatesRequestListener_);
  chrome.certificateProvider.onSignDigestRequested.addListener(
      this.boundSignDigestRequestListener_);
  this.logger.fine(
      'Started listening for chrome.certificateProvider API events');
};

/**
 * @param {function((!Array.<!chrome.certificateProvider.CertificateInfo>),function(!Array.<!chrome.certificateProvider.CertificateInfo>))} reportCallback
 * @private
 */
Backend.prototype.certificatesRequestListener_ = function(reportCallback) {
  this.deferredProcessor_.addJob(this.startCertificatesRequest_.bind(
      this, reportCallback));
};

/**
 * @param {!chrome.certificateProvider.SignRequest} request
 * @param {function(!ArrayBuffer=)} reportCallback
 * @private
 */
Backend.prototype.signDigestRequestListener_ = function(
    request, reportCallback) {
  this.deferredProcessor_.addJob(this.startSignDigestRequest_.bind(
      this, request, reportCallback));
};

/**
 * @param {function((!Array.<!chrome.certificateProvider.CertificateInfo>),function(!Array.<!chrome.certificateProvider.CertificateInfo>))} reportCallback
 * @private
 */
Backend.prototype.startCertificatesRequest_ = function(reportCallback) {
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
Backend.prototype.startSignDigestRequest_ = function(request, reportCallback) {
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
