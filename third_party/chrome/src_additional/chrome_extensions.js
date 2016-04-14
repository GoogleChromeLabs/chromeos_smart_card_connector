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
 * reasons missing in the officially bundled chrome_extensions.js file (see the
 * ../src directory).
 *
 * As soon as the officially bundled chrome_extensions.js file is fixed and gets
 * all these definitions, this file could be removed.
 *
 * @externs
 */


/**
 * @param {!chrome.usb.UsbGetUserSelectedDevicesOptions} options
 * @param {function(!Array.<!chrome.usb.Device>)} callback
 */
chrome.usb.getUserSelectedDevices = function(options, callback) {};


/**
 * @typedef {?{
 *   multiple: (boolean|undefined),
 *   filters: (!Array.<!chrome.usb.DeviceFilter>|undefined)
 * }}
 */
chrome.usb.UsbGetUserSelectedDevicesOptions;


/**
 * @constructor
 */
chrome.usb.DeviceFilter = function() {};


/**
 * @type {number|undefined}
 */
chrome.usb.DeviceFilter.prototype.vendorId;


/**
 * @type {number|undefined}
 */
chrome.usb.DeviceFilter.prototype.productId;


/**
 * @type {number|undefined}
 */
chrome.usb.DeviceFilter.prototype.interfaceClass;


/**
 * @type {number|undefined}
 */
chrome.usb.DeviceFilter.prototype.interfaceSubclass;


/**
 * @type {number|undefined}
 */
chrome.usb.DeviceFilter.prototype.interfaceProtocol;


/**
 * @type {string|undefined}
 */
chrome.usb.Device.prototype.productName;


/**
 * @type {string|undefined}
 */
chrome.usb.Device.prototype.manufacturerName;


/**
 * @type {string|undefined}
 */
chrome.usb.Device.prototype.serialNumber;


/**
 * @constructor
 */
chrome.usb.DeviceEvent = function() {};


/**
 * @param {function(!chrome.usb.Device)} callback
 */
chrome.usb.DeviceEvent.prototype.addListener = function(callback) {};


/**
 * @type {!chrome.usb.DeviceEvent}
 */
chrome.usb.onDeviceAdded;


/**
 * @type {!chrome.usb.DeviceEvent}
 */
chrome.usb.onDeviceRemoved;

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
