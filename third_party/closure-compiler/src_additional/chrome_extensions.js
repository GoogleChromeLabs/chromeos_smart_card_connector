/** @license
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
 * @typedef {string}
 */
chrome.certificateProvider.Hash;


/**
 * @constructor
 */
chrome.certificateProvider.CertificateInfo = function() {};


/**
 * @type {!ArrayBuffer}
 */
chrome.certificateProvider.CertificateInfo.prototype.certificate;


/**
 * @type {!Array.<chrome.certificateProvider.Hash>}
 */
chrome.certificateProvider.CertificateInfo.prototype.supportedHashes;


/**
 * @constructor
 */
chrome.certificateProvider.SignRequest = function() {};


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
chrome.certificateProvider.CertificatesRequestEvent = function() {};


/**
 * @param {function(function((!Array.<!chrome.certificateProvider.CertificateInfo>),function(!Array.<!chrome.certificateProvider.CertificateInfo>)))} callback
 */
chrome.certificateProvider.CertificatesRequestEvent.prototype.addListener = function(callback) {};


/**
 * @param {function(function((!Array.<!chrome.certificateProvider.CertificateInfo>|undefined),function(!Array.<!chrome.certificateProvider.CertificateInfo>)=))} callback
 */
chrome.certificateProvider.CertificatesRequestEvent.prototype.removeListener = function(callback) {};


/**
 * @type {!chrome.certificateProvider.CertificatesRequestEvent}
 */
chrome.certificateProvider.onCertificatesRequested;


/**
 * @constructor
 */
chrome.certificateProvider.SignDigestRequestEvent = function() {};


/**
 * @param {function(!chrome.certificateProvider.SignRequest, function(!ArrayBuffer=))} callback
 */
chrome.certificateProvider.SignDigestRequestEvent.prototype.addListener = function(callback) {};


/**
 * @param {function(!chrome.certificateProvider.SignRequest, function(!ArrayBuffer=))} callback
 */
chrome.certificateProvider.SignDigestRequestEvent.prototype.removeListener = function(callback) {};


/**
 * @type {!chrome.certificateProvider.SignDigestRequestEvent}
 */
chrome.certificateProvider.onSignDigestRequested;

/**
 * @const
 */
chrome.loginState = {};

/**
 * @enum {string}
 */
chrome.loginState.ProfileType = {
  SIGNIN_PROFILE: '',
  USER_PROFILE: '',
};

/**
 * @enum {string}
 */
chrome.loginState.SessionState = {
  UNKNOWN: '',
  IN_OOBE_SCREEN: '',
  IN_LOGIN_SCREEN: '',
  IN_SESSION: '',
  IN_LOCK_SCREEN: '',
};

/**
 * @param {function(!chrome.loginState.ProfileType)} callback Callback function.
 * @return {undefined}
 */
chrome.loginState.getProfileType = function(callback) {};

/**
 * @param {function(!chrome.loginState.SessionState)} callback Callback function.
 * @return {undefined}
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
