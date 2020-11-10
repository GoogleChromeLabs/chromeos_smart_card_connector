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
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Disposable');
goog.require('goog.array');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

const NACL_INCOMING_REQUESTER_NAME = 'certificate_provider_nacl_incoming';

const NACL_OUTGOING_REQUESTER_NAME = 'certificate_provider_nacl_outgoing';

/**
 * Function called in the NaCl module when handling
 * onCertificatesUpdateRequested/onCertificatesRequested events.
 */
const HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME = 'HandleCertificatesRequest';

/**
 * Function called in the NaCl module when handling
 * onSignatureRequested/onSignDigestRequested events.
 */
const HANDLE_SIGNATURE_REQUEST_FUNCTION_NAME = 'HandleSignatureRequest';

const GSC = GoogleSmartCard;

/**
 * Mapping from the |chrome.certificateProvider.Hash| enum values to the
 * |chrome.certificateProvider.Algorithm| ones.
 *
 * Note: The actual string values are hardcoded below, instead of references to
 * |chrome.certificateProvider|, since the latter will break when run with the
 * API version that doesn't have these (on Chrome OS <=85 or on some future
 * Chrome OS that has the legacy Hash enum deleted from the API).
 * @type {!Map<string,string>}
 */
const HASH_TO_ALGORITHM_MAPPING = new Map([
  ['MD5_SHA1', 'RSASSA_PKCS1_v1_5_MD5_SHA1'],
  ['SHA1', 'RSASSA_PKCS1_v1_5_SHA1'],
  ['SHA256', 'RSASSA_PKCS1_v1_5_SHA256'],
  ['SHA384', 'RSASSA_PKCS1_v1_5_SHA384'],
  ['SHA512', 'RSASSA_PKCS1_v1_5_SHA512'],
]);

/**
 * Certificate information returned by the NaCl module in response to the
 * |HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME| call.
 * @typedef {{
 *   certificate: !Array<number>,
 *   supportedAlgorithms: !Array<!chrome.certificateProvider.Algorithm>
 * }}
 */
let NaclCertificateInfo;

/**
 * Information about the onSignatureRequested/onSignDigestRequested event sent
 * to the NaCl module.
 *
 * Note that only one of the fields |input| and |digest| is used; the unused
 * field is empty (note: |undefined| is not used in order to simplify the
 * handling on the NaCl side).
 * @typedef {{
 *   signRequestId: number,
 *   input: !ArrayBuffer,
 *   digest: !ArrayBuffer,
 *   algorithm: !chrome.certificateProvider.Algorithm,
 *   certificate: !ArrayBuffer
 * }}
 */
let NaclSignatureRequest;

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

  /** @private {?function(!chrome.certificateProvider.CertificatesUpdateRequest)} */
  this.certificatesUpdateRequestListener_ = null;
  /** @private {?function(!chrome.certificateProvider.SignatureRequest)} */
  this.signatureRequestListener_ = null;
  /** @private {?function(function((!Array.<!chrome.certificateProvider.CertificateInfo>), function(!Array.<!chrome.certificateProvider.CertificateInfo>)))} */
  this.certificatesRequestListener_ = null;
  /** @private {?function(!chrome.certificateProvider.SignRequest, function(!ArrayBuffer=))} */
  this.signDigestRequestListener_ = null;

  /** @private */
  this.requester_ = new GSC.Requester(
      NACL_INCOMING_REQUESTER_NAME, naclModule.messageChannel);

  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      NACL_OUTGOING_REQUESTER_NAME,
      naclModule.messageChannel,
      this.handleRequest_.bind(this));

  // Start listening for the chrome.certificateProvider API events straight
  // away.
  this.setupApiListeners_();

  /** @private */
  this.deferredProcessor_ = new GSC.DeferredProcessor(
      naclModule.getLoadPromise());
  naclModule.addOnDisposeCallback(this.naclModuleDisposedListener_.bind(this));

  this.logger.fine('Constructed');
};

const Backend = SmartCardClientApp.CertificateProviderBridge.Backend;

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
    this.onCertificatesRequested_(reportCallback);
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
    this.onSignDigestRequested_(request, reportCallback);
  };
}

/** @override */
Backend.prototype.disposeInternal = function() {
  if (this.signDigestRequestListener_) {
    chrome.certificateProvider.onSignDigestRequested.removeListener(
        this.signDigestRequestListener_);
    this.signDigestRequestListener_ = null;
  }
  if (this.certificatesRequestListener_) {
    chrome.certificateProvider.onCertificatesRequested.removeListener(
        this.certificatesRequestListener_);
    this.certificatesRequestListener_ = null;
  }
  if (this.signatureRequestListener_) {
    chrome.certificateProvider.onSignatureRequested.removeListener(
        this.signatureRequestListener_);
    this.signatureRequestListener_ = null;
  }
  if (this.certificatesUpdateRequestListener_) {
    chrome.certificateProvider.onCertificatesUpdateRequested.removeListener(
        this.certificatesUpdateRequestListener_);
    this.certificatesUpdateRequestListener_ = null;
  }

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
  const remoteCallMessage = GSC.RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the remote call message: ' +
        GSC.DebugDump.debugDump(payload));
  }

  const debugRepresentation = 'chrome.certificateProvider.' +
                              remoteCallMessage.getDebugRepresentation();
  this.logger.fine('Received a remote call request: ' + debugRepresentation);

  const promiseResolver = goog.Promise.withResolver();

  // Check whether the API and the method are available.
  const apiFunction = chrome.certificateProvider ?
      chrome.certificateProvider[remoteCallMessage.functionName] : undefined;
  if (apiFunction) {
    const transformedFunctionArguments = transformFunctionArguments(
        remoteCallMessage.functionName, remoteCallMessage.functionArguments);
    /** @preserveTry */
    try {
      transformedFunctionArguments.push(function() {
        if (chrome.runtime.lastError) {
          promiseResolver.reject(new Error(goog.object.get(
              chrome.runtime.lastError, 'message', 'Unknown error')));
          return;
        }
        promiseResolver.resolve(Array.prototype.slice.call(arguments));
      });
      apiFunction.apply(null, transformedFunctionArguments);
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

  if (chrome.certificateProvider.onCertificatesUpdateRequested &&
      chrome.certificateProvider.onSignatureRequested &&
      chrome.certificateProvider.setCertificates) {
    this.certificatesUpdateRequestListener_ =
        this.onCertificatesUpdateRequested_.bind(this);
    chrome.certificateProvider.onCertificatesUpdateRequested.addListener(
        this.certificatesUpdateRequestListener_);
    this.signatureRequestListener_ = this.onSignatureRequested_.bind(this);
    chrome.certificateProvider.onSignatureRequested.addListener(
        this.signatureRequestListener_);
  } else {
    // TODO: Remove this branch after support of Chrome OS <=85 is dropped.
    // (There's no specific timeline - a conservative approach would be to wait
    // until all devices released with Chrome OS <=85 reach the auto-update
    // expiration date, which will be roughly in year 2028.)
    this.logger.info(
        'Proactively notifying Chrome about certificate changes will not be ' +
        'possible: chrome.certificateProvider API version is too old.');
    this.certificatesRequestListener_ =
        this.onCertificatesRequested_.bind(this);
    chrome.certificateProvider.onCertificatesRequested.addListener(
        this.certificatesRequestListener_);
    this.signDigestRequestListener_ = this.onSignDigestRequested_.bind(this);
    chrome.certificateProvider.onSignDigestRequested.addListener(
        this.signDigestRequestListener_);
  }

  this.logger.fine(
      'Started listening for chrome.certificateProvider API events');
};

/**
 * @param {!chrome.certificateProvider.CertificatesUpdateRequest} request
 * @private
 */
Backend.prototype.onCertificatesUpdateRequested_ = function(request) {
  this.deferredProcessor_.addJob(this.processCertificatesUpdateRequest_.bind(
      this, request));
};

/**
 * @param {!chrome.certificateProvider.SignatureRequest} request
 * @private
 */
Backend.prototype.onSignatureRequested_ = function(request) {
  this.deferredProcessor_.addJob(this.processSignatureRequest_.bind(
      this, request));
};

/**
 * @param {function((!Array.<!chrome.certificateProvider.CertificateInfo>), function(!Array.<!chrome.certificateProvider.CertificateInfo>))} reportCallback
 * @private
 */
Backend.prototype.onCertificatesRequested_ = function(reportCallback) {
  this.deferredProcessor_.addJob(this.processCertificatesRequest_.bind(
      this, reportCallback));
};

/**
 * @param {!chrome.certificateProvider.SignRequest} request
 * @param {function(!ArrayBuffer=)} reportCallback
 * @private
 */
Backend.prototype.onSignDigestRequested_ = function(
    request, reportCallback) {
  this.deferredProcessor_.addJob(this.processSignDigestRequest_.bind(
      this, request, reportCallback));
};

/**
 * @param {!chrome.certificateProvider.CertificatesUpdateRequest} request
 * @private
 */
Backend.prototype.processCertificatesUpdateRequest_ = function(request) {
  this.logger.info('Started handling certificates update request');
  const remoteCallMessage = new GSC.RemoteCallMessage(
      HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME, []);
  const promise = this.requester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  promise.then(function(results) {
    GSC.Logging.checkWithLogger(this.logger, results.length == 1);
    const certificates = goog.array.map(results[0], createClientCertificateInfo);
    this.logger.info(
        'Setting the certificate list with ' + certificates.length +
        ' certificates: ' + GSC.DebugDump.debugDump(certificates));
    chrome.certificateProvider.setCertificates({
      certificatesRequestId: request.certificatesRequestId,
      clientCertificates: certificates
    });
  }, function() {
    this.logger.info('Setting empty certificates list and an error');
    chrome.certificateProvider.setCertificates({
      certificatesRequestId: request.certificatesRequestId,
      clientCertificates: [],
      error: chrome.certificateProvider.Error.GENERAL_ERROR
    });
  }, this);
};

/**
 * @param {!chrome.certificateProvider.SignatureRequest} request
 * @private
 */
Backend.prototype.processSignatureRequest_ = function(request) {
  this.logger.info(
      'Started handling signature request. The request contents are: ' +
      'algorithm is "' + request.algorithm + '", input is ' +
      GSC.DebugDump.debugDump(request.input) + ', certificate is ' +
      GSC.DebugDump.debugDump(request.certificate));
  /** @type {!NaclSignatureRequest} */
  const naclRequest = {
    signRequestId: request.signRequestId,
    input: request.input,
    digest: new ArrayBuffer(/*length=*/0),
    algorithm: request.algorithm,
    certificate: request.certificate,
  };
  const remoteCallMessage = new GSC.RemoteCallMessage(
      HANDLE_SIGNATURE_REQUEST_FUNCTION_NAME, [naclRequest]);
  const promise = this.requester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  promise.then(function(results) {
    GSC.Logging.checkWithLogger(this.logger, results.length == 1);
    const signature = results[0];
    this.logger.info(
        'Responding to the signature request with the created signature: ' +
        GSC.DebugDump.debugDump(signature));
    chrome.certificateProvider.reportSignature({
      signRequestId: request.signRequestId,
      signature: signature
    });
  }, function() {
    this.logger.info('Responding to the signature request with an error');
    chrome.certificateProvider.reportSignature({
      signRequestId: request.signRequestId,
      error: chrome.certificateProvider.Error.GENERAL_ERROR
    });
  }, this);
};

/**
 * @param {function((!Array.<!chrome.certificateProvider.CertificateInfo>), function(!Array.<!chrome.certificateProvider.CertificateInfo>))} reportCallback
 * @private
 */
Backend.prototype.processCertificatesRequest_ = function(reportCallback) {
  this.logger.info('Started handling certificates request');
  const remoteCallMessage = new GSC.RemoteCallMessage(
      HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME, []);
  const promise = this.requester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  promise.then(function(results) {
    GSC.Logging.checkWithLogger(this.logger, results.length == 1);
    const certificates = goog.array.map(results[0], createCertificateInfo);
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
Backend.prototype.processSignDigestRequest_ = function(
    request, reportCallback) {
  this.logger.info(
      'Started handling digest signing request. The request contents are: ' +
      'hash is "' + request.hash + '", digest is ' +
      GSC.DebugDump.debugDump(request.digest) + ', certificate is ' +
      GSC.DebugDump.debugDump(request.certificate));
  /** @type {!NaclSignatureRequest} */
  const naclRequest = {
    signRequestId: request.signRequestId,
    input: new ArrayBuffer(/*length=*/0),
    digest: request.digest,
    algorithm: getAlgorithmFromHash(request.hash),
    certificate: request.certificate,
  };
  const remoteCallMessage = new GSC.RemoteCallMessage(
      HANDLE_SIGNATURE_REQUEST_FUNCTION_NAME, [naclRequest]);
  const promise = this.requester_.postRequest(
      remoteCallMessage.makeRequestPayload());
  promise.then(function(results) {
    GSC.Logging.checkWithLogger(this.logger, results.length == 1);
    const signature = results[0];
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
 * @param {string} functionName
 * @param {!Array.<*>} functionArguments
 * @return {!Array.<*>}
 */
function transformFunctionArguments(functionName, functionArguments) {
  const transformedArguments = functionArguments.slice();
  if (functionName === 'setCertificates') {
    // The certificates need to be transformed in order to be recognized by the
    // API.
    transformedArguments[0] =
        goog.object.clone(/** @type {!Object<?,?>} */(functionArguments[0]));
    const certificates = goog.array.map(
        functionArguments[0]["clientCertificates"],
        createClientCertificateInfo);
    transformedArguments[0]["clientCertificates"] = certificates;
  }
  return transformedArguments;
}

/**
 * @param {!NaclCertificateInfo} naclCertificateInfo
 * @return {!chrome.certificateProvider.ClientCertificateInfo}
 */
function createClientCertificateInfo(naclCertificateInfo) {
  return {
    certificateChain: [naclCertificateInfo['certificate']],
    supportedAlgorithms: naclCertificateInfo['supportedAlgorithms']
  };
}

/**
 * @param {!NaclCertificateInfo} naclCertificateInfo
 * @return {!chrome.certificateProvider.CertificateInfo}
 */
function createCertificateInfo(naclCertificateInfo) {
  return {
    certificate: naclCertificateInfo['certificate'],
    supportedHashes: goog.array.map(
      naclCertificateInfo['supportedAlgorithms'], getHashFromAlgorithm)
  };
}

/**
 * @param {!chrome.certificateProvider.Hash} hash
 * @return {!chrome.certificateProvider.Algorithm}
 */
function getAlgorithmFromHash(hash) {
  if (HASH_TO_ALGORITHM_MAPPING.has(hash)) {
    return /** @type {!chrome.certificateProvider.Algorithm} */ (
        HASH_TO_ALGORITHM_MAPPING.get(hash));
  }
  GSC.Logging.fail(`Unknown hash ${hash}`);
  return chrome.certificateProvider.Algorithm.RSASSA_PKCS1_v1_5_SHA1;
}

/**
 * @param {!chrome.certificateProvider.Algorithm} algorithm
 * @return {!chrome.certificateProvider.Hash}
 */
function getHashFromAlgorithm(algorithm) {
  for (const hash of HASH_TO_ALGORITHM_MAPPING.keys()) {
    if (HASH_TO_ALGORITHM_MAPPING.get(hash) === algorithm)
      return /** @type {!chrome.certificateProvider.Hash} */ (hash);
  }
  GSC.Logging.fail(`Unknown algorithm ${algorithm}`);
  return chrome.certificateProvider.Hash.SHA1;
}

});  // goog.scope
