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
 *   that translates between requests to/from the chrome.certificateProvider API
 *   (see <https://developer.chrome.com/extensions/certificateProvider>) into
 *   from/to the NaCl module.
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

goog.require('GoogleSmartCard.AppUtils');
goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.PcscLiteClient.NaclClientBackend');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PopupWindow.Server');
goog.require('SmartCardClientApp.CertificateProviderBridge.Backend');
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
 * URL of the main window.
 *
 * It's opened when the user launches the App.
 */
const MAIN_WINDOW_URL = 'window.html';

/**
 * Identifier of the main window.
 *
 * It's used in order to prevent opening multiple instances of the main window.
 * Additionally, this enables remembering of window's position across restarts.
 */
const MAIN_WINDOW_ID = 'main-window';

/**
 * Parameters of the main window.
 *
 * Note that the window is intentionally created as "hidden", because the window
 * should only be shown after some additional initialization is done (see
 * window.js).
 */
const MAIN_WINDOW_OPTIONS = {
  'frame': 'none',
  'hidden': true,
  'id': MAIN_WINDOW_ID,
  'innerBounds': {
    'width': 500,
    'height': 500,
  },
  'resizable': false,
};

/**
 * Logger that should be used for logging the App log messages.
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getLogger(
    'SmartCardClientApp',
    goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO);

const extensionManifest = chrome.runtime.getManifest();
const formattedStartupTime = (new Date()).toLocaleString();
logger.info(
    `The extension (id "${chrome.runtime.id}", version ` +
    `${extensionManifest.version}) background script started. Browser ` +
    `version: "${window.navigator.appVersion}". System time: ` +
    `"${formattedStartupTime}"`);

/**
 * Holder of the NaCl module containing the actual smart card client code.
 * @const
 */
var naclModule = new GSC.NaclModule(
    'nacl_module.nmf', GSC.NaclModule.Type.PNACL);

/**
 * Called when the NaCl module is disposed of.
 */
function naclModuleDisposedListener() {
  if (!goog.DEBUG) {
    // Trigger the fatal error in the Release mode, which will emit an error
    // message and cause the app reload (see the GSC.Logging.fail() function
    // implementation in //common/js/src/logging/logging.js).
    GSC.Logging.failWithLogger(logger, 'NaCl module was disposed of');
  }
}

// Reload our app when the NaCl module is disposed of (e.g., due to a crash) and
// we're in the Release mode.
naclModule.addOnDisposeCallback(naclModuleDisposedListener);

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

// Starts the NaCl module loading. Up to this point, the module was not actually
// loading yet, which allowed to add all the necessary event listeners in
// advance.
naclModule.startLoading();

// Open the UI window when the user launches the app.
chrome.app.runtime.onLaunched.addListener(() => {
  GSC.PopupWindow.Server.createWindow(MAIN_WINDOW_URL, MAIN_WINDOW_OPTIONS);
});

// Automatically load the App (in the background) with Chrome startup.
GSC.AppUtils.enableSelfAutoLoading();

});  // goog.scope
