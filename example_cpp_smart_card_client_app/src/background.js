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
 * @fileoverview Entry point of the Smart Card Client App background script (see
 * <https://developer.chrome.com/apps/event_pages>).
 *
 * The main pieces performed here are the following:
 * * NaCl module containing the actual client implementation is created and
 *   loaded (being attached as an <embed> to the App background page);
 * * A GoogleSmartCard.PcscLiteClient.NaclClientBackend object is created, that
 *   translates all PC/SC-Lite client API requests received from the NaCl module
 *   into the JavaScript PC/SC-Lite client API method calls (see the
 *   GoogleSmartCard.PcscLiteClient.API class).
 * * A SmartCardClientApp.CertificateProviderBridge.Backend object is created,
 *   that subscribes itself for chrome.certificateProvider API events (see
 *   <https://developer.chrome.com/extensions/certificateProvider#events>) and
 *   translates them into requests to the NaCl module.
 * * A SmartCardClientApp.PinDialog.Backend object is created, that performs the
 *   PIN dialog requests received from the NaCl module.
 * * Subscribing to a special Chrome Extensions API event that makes the App
 *   auto-loading.
 *
 * Note that the App's background page will be never unloaded by Chrome due to
 * inactivity (in contrast to how it usually manages the background pages - see
 * <https://developer.chrome.com/apps/event_pages#lifetime>), because having an
 * attached NaCl module disables this unloading mechanism (see
 * <http://crbug.com/472532>).
 */

goog.provide('SmartCardClientApp.BackgroundMain');

goog.require('SmartCardClientApp.CertificateProviderBridge.Backend');
goog.require('SmartCardClientApp.PinDialog.Backend');
goog.require('GoogleSmartCard.AppUtils');
goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.PcscLiteClient.NaclClientBackend');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('goog.log.Level');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var Constants = GSC.PcscLiteCommon.Constants;

/**
 * Client title for the connection to the server App.
 *
 * Currently this is only used for the debug logs produced by the server App.
 * @const
 */
var CLIENT_TITLE = 'example_cpp_client_app';

/**
 * Identifier of the server App.
 * @const
 */
var SERVER_APP_ID = Constants.SERVER_OFFICIAL_APP_ID;

/**
 * Logger that should be used for logging the App log messages.
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getLogger(
    'SmartCardClientApp',
    goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO);

/**
 * Holder of the NaCl module containing the actual smart card client code.
 * @const
 */
var naclModule = new GSC.NaclModule(
    'nacl_module.nmf', GSC.NaclModule.Type.PNACL);

/**
 * Translator of all PC/SC-Lite client API requests received from the NaCl
 * module into the JavaScript PC/SC-Lite client API method calls (see the
 * GoogleSmartCard.PcscLiteClient.API class).
 * @const
 */
var pcscLiteNaclClientBackend = new GSC.PcscLiteClient.NaclClientBackend(
    naclModule.messageChannel, CLIENT_TITLE, SERVER_APP_ID);

/**
 * Translator of the events from the chrome.certificateProvider API (see
 * <https://developer.chrome.com/extensions/certificateProvider#events>) into
 * requests sent to the NaCl module.
 * @const
 */
var certificateProviderBridgeBackend =
    new SmartCardClientApp.CertificateProviderBridge.Backend(naclModule);

/**
 * Object that performs the PIN dialog requests received from the NaCl module.
 * @const
 */
var pinDialogBackend = new SmartCardClientApp.PinDialog.Backend(
    naclModule.messageChannel);

// Starts the NaCl module loading. Up to this point, the module was not actually
// loading yet, which allowed to add all the necessary event listeners in
// advance.
naclModule.load();

GSC.AppUtils.enableSelfAutoLoading();

});  // goog.scope
