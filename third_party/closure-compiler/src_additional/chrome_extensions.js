/**
 * @license
 * Copyright 2016 Google Inc.
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
 * This file contains the Chrome Extensions API definitions, that are for some
 * reasons missing in the chrome_extensions.js file that is shipped with the
 * Closure Compiler (../src/contrib/externs/chrome_extensions.js).
 *
 * As soon as the officially bundled chrome_extensions.js file is fixed and gets
 * all these definitions, this file could be removed.
 *
 * @externs
 */


/**
 * @const
 */
chrome.certificateProvider = {};


/**
 * @enum {string}
 */
chrome.certificateProvider.Algorithm = {
  RSASSA_PKCS1_v1_5_MD5_SHA1: 'RSASSA_PKCS1_v1_5_MD5_SHA1',
  RSASSA_PKCS1_v1_5_SHA1: 'RSASSA_PKCS1_v1_5_SHA1',
  RSASSA_PKCS1_v1_5_SHA256: 'RSASSA_PKCS1_v1_5_SHA256',
  RSASSA_PKCS1_v1_5_SHA384: 'RSASSA_PKCS1_v1_5_SHA384',
  RSASSA_PKCS1_v1_5_SHA512: 'RSASSA_PKCS1_v1_5_SHA512',
};


/**
 * @enum {string}
 */
chrome.certificateProvider.Error = {
  GENERAL_ERROR: 'GENERAL_ERROR',
};


/**
 * @enum {string}
 */
chrome.certificateProvider.Hash = {
  MD5_SHA1: 'MD5_SHA1',
  SHA1: 'SHA1',
  SHA256: 'SHA256',
  SHA384: 'SHA384',
  SHA512: 'SHA512',
};


/**
 * @typedef {{
 *   certificateChain: !Array<!ArrayBuffer>,
 *   supportedAlgorithms: !Array<!chrome.certificateProvider.Algorithm>
 * }}
 */
chrome.certificateProvider.ClientCertificateInfo;


/**
 * @typedef {{
 *   certificatesRequestId: (number|undefined),
 *   error: (!chrome.certificateProvider.Error|undefined),
 *   clientCertificates:
 *       !Array<!chrome.certificateProvider.ClientCertificateInfo>
 * }}
 */
chrome.certificateProvider.SetCertificatesDetails;


/**
 * @constructor
 */
chrome.certificateProvider.CertificatesUpdateRequest = function() {};

/**
 * @type {number}
 */
chrome.certificateProvider.CertificatesUpdateRequest.prototype
    .certificatesRequestId;


/**
 * @constructor
 */
chrome.certificateProvider.SignatureRequest = function() {};

/**
 * @type {number}
 */
chrome.certificateProvider.SignatureRequest.prototype.signRequestId;

/**
 * @type {!ArrayBuffer}
 */
chrome.certificateProvider.SignatureRequest.prototype.input;

/**
 * @type {!chrome.certificateProvider.Algorithm}
 */
chrome.certificateProvider.SignatureRequest.prototype.algorithm;

/**
 * @type {!ArrayBuffer}
 */
chrome.certificateProvider.SignatureRequest.prototype.certificate;


/**
 * @typedef {{
 *   signRequestId: number,
 *   error: (!chrome.certificateProvider.Error|undefined),
 *   signature: (!ArrayBuffer|undefined)
 * }}
 */
chrome.certificateProvider.ReportSignatureDetails;


/**
 * @typedef {{
 *   certificate: !ArrayBuffer,
 *   supportedHashes: !Array.<chrome.certificateProvider.Hash>
 * }}
 */
chrome.certificateProvider.CertificateInfo;


/**
 * @constructor
 */
chrome.certificateProvider.SignRequest = function() {};


/**
 * @type {number}
 */
chrome.certificateProvider.SignRequest.prototype.signRequestId;


/**
 * @type {!ArrayBuffer}
 */
chrome.certificateProvider.SignRequest.prototype.digest;


/**
 * @type {chrome.certificateProvider.Hash}
 */
chrome.certificateProvider.SignRequest.prototype.hash;


/**
 * @type {!ArrayBuffer}
 */
chrome.certificateProvider.SignRequest.prototype.certificate;


/**
 * @constructor
 */
chrome.certificateProvider.CertificatesUpdateRequestEvent = function() {};

/**
 * @param {function(!chrome.certificateProvider.CertificatesUpdateRequest)}
 *     callback
 */
chrome.certificateProvider.CertificatesUpdateRequestEvent.prototype
    .addListener = function(callback) {};

/**
 * @param {function(!chrome.certificateProvider.CertificatesUpdateRequest)}
 *     callback
 */
chrome.certificateProvider.CertificatesUpdateRequestEvent.prototype
    .removeListener = function(callback) {};

/**
 * @type {!chrome.certificateProvider.CertificatesUpdateRequestEvent}
 */
chrome.certificateProvider.onCertificatesUpdateRequested;


/**
 * @constructor
 */
chrome.certificateProvider.SignatureRequestEvent = function() {};

/**
 * @param {function(!chrome.certificateProvider.SignatureRequest)} callback
 */
chrome.certificateProvider.SignatureRequestEvent.prototype.addListener =
    function(callback) {};

/**
 * @param {function(!chrome.certificateProvider.SignatureRequest)} callback
 */
chrome.certificateProvider.SignatureRequestEvent.prototype.removeListener =
    function(callback) {};

/**
 * @type {!chrome.certificateProvider.SignatureRequestEvent}
 */
chrome.certificateProvider.onSignatureRequested;


/**
 * @constructor
 */
chrome.certificateProvider.CertificatesRequestEvent = function() {};


/**
 * @param {function(function((!Array.<!chrome.certificateProvider.CertificateInfo>),function(!Array.<!chrome.certificateProvider.CertificateInfo>)))}
 *     callback
 */
chrome.certificateProvider.CertificatesRequestEvent.prototype.addListener =
    function(callback) {};


/**
 * @param {function(function((!Array.<!chrome.certificateProvider.CertificateInfo>|undefined),function(!Array.<!chrome.certificateProvider.CertificateInfo>)=))}
 *     callback
 */
chrome.certificateProvider.CertificatesRequestEvent.prototype.removeListener =
    function(callback) {};


/**
 * @type {!chrome.certificateProvider.CertificatesRequestEvent}
 */
chrome.certificateProvider.onCertificatesRequested;


/**
 * @constructor
 */
chrome.certificateProvider.SignDigestRequestEvent = function() {};


/**
 * @param {function(!chrome.certificateProvider.SignRequest,
 *     function(!ArrayBuffer=))} callback
 */
chrome.certificateProvider.SignDigestRequestEvent.prototype.addListener =
    function(callback) {};


/**
 * @param {function(!chrome.certificateProvider.SignRequest,
 *     function(!ArrayBuffer=))} callback
 */
chrome.certificateProvider.SignDigestRequestEvent.prototype.removeListener =
    function(callback) {};


/**
 * @type {!chrome.certificateProvider.SignDigestRequestEvent}
 */
chrome.certificateProvider.onSignDigestRequested;

/**
 * @param {!chrome.certificateProvider.SetCertificatesDetails} details
 * @param {function()=} callback
 */
chrome.certificateProvider.setCertificates = function(details, callback) {};

/**
 * @param {!chrome.certificateProvider.ReportSignatureDetails} details
 * @param {function()=} callback
 */
chrome.certificateProvider.reportSignature = function(details, callback) {};


/**
 * @const
 */
chrome.loginState = {};

/**
 * @enum {string}
 */
chrome.loginState.ProfileType = {
  SIGNIN_PROFILE: 'SIGNIN_PROFILE',
  USER_PROFILE: 'USER_PROFILE',
};

/**
 * @enum {string}
 */
chrome.loginState.SessionState = {
  UNKNOWN: 'UNKNOWN',
  IN_OOBE_SCREEN: 'IN_OOBE_SCREEN',
  IN_LOGIN_SCREEN: 'IN_LOGIN_SCREEN',
  IN_SESSION: 'IN_SESSION',
  IN_LOCK_SCREEN: 'IN_LOCK_SCREEN',
};

/**
 * @param {function(!chrome.loginState.ProfileType)} callback
 */
chrome.loginState.getProfileType = function(callback) {};

/**
 * @param {function(!chrome.loginState.SessionState)} callback
 */
chrome.loginState.getSessionState = function(callback) {};

/**
 * Event that triggers when the session state changes.
 * @interface
 * @extends {ChromeBaseEvent<function(!chrome.loginState.SessionState)>}
 */
chrome.loginState.SessionStateEvent = function() {};

/**
 * @type {!chrome.loginState.SessionStateEvent}
 */
chrome.loginState.onSessionStateChanged;

/**
 * @const
 */
chrome.smartCardProviderPrivate = {};

/** @type {!ChromeBaseEvent<function(!number)>} */
chrome.smartCardProviderPrivate.onEstablishContextRequested;

/**
 * @enum {string}
 */
chrome.smartCardProviderPrivate.ResultCode = {
  SUCCESS: 'SUCCESS',
  INTERNAL_ERROR: 'INTERNAL_ERROR',
};

/**
 * @param {number} requestId
 * @param {number} sCardContext
 * @param {chrome.smartCardProviderPrivate.ResultCode} resultCode
 */
chrome.smartCardProviderPrivate.reportEstablishContextResult = function(
    requestId, sCardContext, resultCode) {};
