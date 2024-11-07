/**
 * @license
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
 * <https://developer.chrome.com/extensions/certificateProvider>) and the
 * executable module handlers.
 */

goog.provide('SmartCardClientApp.CertificateProviderBridge.Backend');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.DeferredProcessor');
goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('GoogleSmartCard.Requester');
goog.require('goog.Disposable');
goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.array');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

// Note: The following string constants must match the ones in the
// api_bridge.cc.

const TO_EXECUTABLE_REQUESTER_NAME =
    'certificate_provider_request_to_executable';

const FROM_EXECUTABLE_REQUESTER_NAME =
    'certificate_provider_request_from_executable';

/**
 * Function called in the executable module when handling
 * onCertificatesUpdateRequested/onCertificatesRequested events.
 */
const HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME = 'HandleCertificatesRequest';

/**
 * Function called in the executable module when handling
 * onSignatureRequested events.
 */
const HANDLE_SIGNATURE_REQUEST_FUNCTION_NAME = 'HandleSignatureRequest';

const GSC = GoogleSmartCard;

/**
 * Certificate information returned by the executable module in response to the
 * |HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME| call.
 * @typedef {{
 *   certificate: !Array<number>,
 *   supportedAlgorithms: !Array<!chrome.certificateProvider.Algorithm>
 * }}
 */
let ExecutableModuleCertificateInfo;

/**
 * Information about the onSignatureRequested event sent to the C++ executable
 * module.
 * @typedef {{
 *   signRequestId: number,
 *   input: !ArrayBuffer,
 *   algorithm: !chrome.certificateProvider.Algorithm,
 *   certificate: !ArrayBuffer
 * }}
 */
let ExecutableModuleSignatureRequest;

/**
 * JavaScript-side backend for the bridge between JavaScript
 * chrome.certificateProvider API (see
 * <https://developer.chrome.com/extensions/certificateProvider>) and the
 * executable module handlers.
 *
 * This object subscribes itself for listening the chrome.certificateProvider
 * API events and translates them into requests sent to the executable module as
 * messages. The response messages from the executable module, once they are
 * received, trigger the corresponding callbacks provided by the
 * chrome.certificateProvider API.
 *
 * Note that the requests to the executable module are delayed until it loads,
 * but the API listeners are set up straight away. This allows to handle the
 * chrome.certificateProvider API events correctly even if they were received
 * too early.
 * @param {!GSC.ExecutableModule} executableModule
 * @extends goog.Disposable
 * @constructor
 */
SmartCardClientApp.CertificateProviderBridge.Backend = function(
    executableModule) {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getLogger(
      'SmartCardClientApp.CertificateProviderBridge.Backend');

  /**
   * @private {?function(!chrome.certificateProvider.CertificatesUpdateRequest)}
   */
  this.certificatesUpdateRequestListener_ = null;
  /** @private {?function(!chrome.certificateProvider.SignatureRequest)} */
  this.signatureRequestListener_ = null;

  /** @private */
  this.requester_ = new GSC.Requester(
      TO_EXECUTABLE_REQUESTER_NAME, executableModule.getMessageChannel());

  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      FROM_EXECUTABLE_REQUESTER_NAME, executableModule.getMessageChannel(),
      this.handleRequest_.bind(this));

  // Start listening for the chrome.certificateProvider API events straight
  // away.
  this.setupApiListeners_();

  /** @private @const */
  this.deferredProcessor_ =
      new GSC.DeferredProcessor(executableModule.getLoadPromise());
  executableModule.addOnDisposeCallback(
      this.executableModuleDisposedListener_.bind(this));

  goog.log.fine(this.logger, 'Constructed');
};

const Backend = SmartCardClientApp.CertificateProviderBridge.Backend;

goog.inherits(Backend, goog.Disposable);

/** @override */
Backend.prototype.disposeInternal = function() {
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

  goog.log.fine(this.logger, 'Disposed');

  Backend.base(this, 'disposeInternal');
};

/**
 * Handles a remote call request received from the executable module.
 * @param {!Object} payload
 * @return {!goog.Thenable}
 * @private
 */
Backend.prototype.handleRequest_ = function(payload) {
  const remoteCallMessage = GSC.RemoteCallMessage.parseRequestPayload(payload);
  if (!remoteCallMessage) {
    GSC.Logging.failWithLogger(
        this.logger,
        'Failed to parse the remote call message: ' +
            GSC.DebugDump.debugDumpSanitized(payload));
  }

  const debugRepresentation = 'chrome.certificateProvider.' +
      remoteCallMessage.getDebugRepresentation();
  goog.log.fine(
      this.logger, 'Received a remote call request: ' + debugRepresentation);

  const promiseResolver = goog.Promise.withResolver();

  // Check whether the API and the method are available.
  const apiFunction = chrome.certificateProvider ?
      chrome.certificateProvider[remoteCallMessage.functionName] :
      undefined;
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
        '. Please check that the extension is running on ChromeOS and that ' +
        'the Chrome version is sufficiently new.');
  }

  return promiseResolver.promise;
};

/** @private */
Backend.prototype.executableModuleDisposedListener_ = function() {
  goog.log.fine(this.logger, 'Executable module was disposed, disposing...');
  this.dispose();
};

/** @private */
Backend.prototype.setupApiListeners_ = function() {
  GSC.Logging.checkWithLogger(this.logger, !this.isDisposed());

  if (!chrome.certificateProvider) {
    if (goog.DEBUG) {
      goog.log.warning(
          this.logger,
          'chrome.certificateProvider API is not available. Providing ' +
              'certificates to the Chrome browser will be impossible. This ' +
              'is just a warning in the Debug build (in order to make some ' +
              'testing on non-ChromeOS systems possible), but this will be a ' +
              'fatal error in the Release build');
    } else {
      goog.log.error(
          this.logger,
          'chrome.certificateProvider API is not available. Providing ' +
              'certificates to the Chrome browser will be impossible');
    }
    return;
  }

  if (!chrome.certificateProvider.onCertificatesUpdateRequested ||
      !chrome.certificateProvider.onSignatureRequested ||
      !chrome.certificateProvider.setCertificates) {
    goog.log.error(
        this.logger, 'chrome.certificateProvider API version is incompatible');
    return;
  }

  this.certificatesUpdateRequestListener_ =
      this.onCertificatesUpdateRequested_.bind(this);
  chrome.certificateProvider.onCertificatesUpdateRequested.addListener(
      this.certificatesUpdateRequestListener_);
  this.signatureRequestListener_ = this.onSignatureRequested_.bind(this);
  chrome.certificateProvider.onSignatureRequested.addListener(
      this.signatureRequestListener_);

  goog.log.fine(
      this.logger,
      'Started listening for chrome.certificateProvider API events');
};

/**
 * @param {!chrome.certificateProvider.CertificatesUpdateRequest} request
 * @private
 */
Backend.prototype.onCertificatesUpdateRequested_ = function(request) {
  this.deferredProcessor_.addJob(
      this.processCertificatesUpdateRequest_.bind(this, request));
};

/**
 * @param {!chrome.certificateProvider.SignatureRequest} request
 * @private
 */
Backend.prototype.onSignatureRequested_ = function(request) {
  this.deferredProcessor_.addJob(
      this.processSignatureRequest_.bind(this, request));
};

/**
 * @param {!chrome.certificateProvider.CertificatesUpdateRequest} request
 * @private
 */
Backend.prototype.processCertificatesUpdateRequest_ = function(request) {
  goog.log.info(this.logger, 'Started handling certificates update request');
  const remoteCallMessage =
      new GSC.RemoteCallMessage(HANDLE_CERTIFICATES_REQUEST_FUNCTION_NAME, []);
  const promise =
      this.requester_.postRequest(remoteCallMessage.makeRequestPayload());
  promise.then(
      function(results) {
        GSC.Logging.checkWithLogger(this.logger, results.length == 1);
        const certificates =
            goog.array.map(results[0], createClientCertificateInfo);
        goog.log.info(
            this.logger,
            'Setting the certificate list with ' + certificates.length +
                ' certificates: ' +
                GSC.DebugDump.debugDumpSanitized(certificates));
        chrome.certificateProvider.setCertificates({
          certificatesRequestId: request.certificatesRequestId,
          clientCertificates: certificates
        });
      },
      function() {
        goog.log.info(
            this.logger, 'Setting empty certificates list and an error');
        chrome.certificateProvider.setCertificates({
          certificatesRequestId: request.certificatesRequestId,
          clientCertificates: [],
          error: chrome.certificateProvider.Error.GENERAL_ERROR
        });
      },
      this);
};

/**
 * @param {!chrome.certificateProvider.SignatureRequest} request
 * @private
 */
Backend.prototype.processSignatureRequest_ = function(request) {
  goog.log.info(
      this.logger,
      'Started handling signature request. The request contents are: ' +
          'algorithm is "' + request.algorithm + '", input is ' +
          GSC.DebugDump.debugDumpSanitized(request.input) +
          ', certificate is ' +
          GSC.DebugDump.debugDumpSanitized(request.certificate));
  /** @type {!ExecutableModuleSignatureRequest} */
  const executableRequest = {
    signRequestId: request.signRequestId,
    input: request.input,
    algorithm: request.algorithm,
    certificate: request.certificate,
  };
  const remoteCallMessage = new GSC.RemoteCallMessage(
      HANDLE_SIGNATURE_REQUEST_FUNCTION_NAME, [executableRequest]);
  const promise =
      this.requester_.postRequest(remoteCallMessage.makeRequestPayload());
  promise.then(
      function(results) {
        GSC.Logging.checkWithLogger(this.logger, results.length == 1);
        const signature = results[0];
        goog.log.info(
            this.logger,
            'Responding to the signature request with the created signature: ' +
                GSC.DebugDump.debugDumpSanitized(signature));
        chrome.certificateProvider.reportSignature({
          signRequestId: request.signRequestId,
          signature: toArrayBuffer(signature)
        });
      },
      function() {
        goog.log.info(
            this.logger, 'Responding to the signature request with an error');
        chrome.certificateProvider.reportSignature({
          signRequestId: request.signRequestId,
          error: chrome.certificateProvider.Error.GENERAL_ERROR
        });
      },
      this);
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
        goog.object.clone(/** @type {!Object<?,?>} */ (functionArguments[0]));
    const certificates = goog.array.map(
        functionArguments[0]['clientCertificates'],
        createClientCertificateInfo);
    transformedArguments[0]['clientCertificates'] = certificates;
  }
  return transformedArguments;
}

/**
 * @param {!ExecutableModuleCertificateInfo} executableModuleCertificateInfo
 * @return {!chrome.certificateProvider.ClientCertificateInfo}
 */
function createClientCertificateInfo(executableModuleCertificateInfo) {
  return {
    certificateChain:
        [toArrayBuffer(executableModuleCertificateInfo['certificate'])],
    supportedAlgorithms: executableModuleCertificateInfo['supportedAlgorithms']
  };
}

/**
 * Converts the array of bytes to an array buffer, if needed.
 * @param {!ArrayBuffer|!Array<number>} value
 * @return {!ArrayBuffer}
 */
function toArrayBuffer(value) {
  if (value instanceof ArrayBuffer)
    return value;
  return (new Uint8Array(value)).buffer;
}
});  // goog.scope
