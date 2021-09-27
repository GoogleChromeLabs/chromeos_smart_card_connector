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
 * @fileoverview Entry point of the Smart Card Client App background script (see
 * <https://developer.chrome.com/apps/event_pages>).
 *
 * The main pieces performed here are the following:
 * * The executable module containing the actual client implementation is
 *   created and loaded (using a method that is needed for either
 *   Emscripten/WebAssembly or Native Client, depending on the build
 *   configuration);
 * * A GoogleSmartCard.PcscLiteClient.NaclClientBackend object is created, that
 *   translates all PC/SC-Lite client API requests received from the executable
 *   module into the JavaScript PC/SC-Lite client API method calls (see the
 *   GoogleSmartCard.PcscLiteClient.API class).
 * * A SmartCardClientApp.CertificateProviderBridge.Backend object is created,
 *   that translates between requests to/from the chrome.certificateProvider API
 *   (see <https://developer.chrome.com/extensions/certificateProvider>) into
 *   requests from/to the executable module.
 * * A SmartCardClientApp.BuiltInPinDialog.Backend object is created, that
 *   handles the built-in PIN dialog requests received from the executable
 *   module.
 * * Subscribing to a special Chrome Extensions API event that makes the App
 *   auto-loading.
 *
 * Note that the App's background page will be never unloaded by Chrome due to
 * inactivity (in contrast to how it usually manages the background pages - see
 * <https://developer.chrome.com/apps/event_pages#lifetime>), because having an
 * attached NaCl module disables this unloading mechanism (see
 * <http://crbug.com/472532>).
 * TODO(#220): Figure out the story with the Emscripten/WebAssembly builds.
 */

goog.provide('SmartCardClientApp.BackgroundMain');

goog.require('GoogleSmartCard.AppUtils');
goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.EmscriptenModule');
goog.require('GoogleSmartCard.ExecutableModule');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.NaclModule');
goog.require('GoogleSmartCard.PcscLiteClient.NaclClientBackend');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('GoogleSmartCard.PopupOpener');
goog.require('SmartCardClientApp.BuiltInPinDialog.Backend');
goog.require('SmartCardClientApp.CertificateProviderBridge.Backend');
goog.require('goog.asserts');
goog.require('goog.log');
goog.require('goog.log.Level');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

const Constants = GSC.PcscLiteCommon.Constants;

/**
 * Client title for the connection to the server App.
 *
 * Currently this is only used for the debug logs produced by the server App.
 */
const CLIENT_TITLE = 'example_cpp_client_app';

/**
 * Identifier of the server App.
 */
const SERVER_APP_ID = Constants.SERVER_OFFICIAL_APP_ID;

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
  'width': 500,
  'resizable': false,
};

/**
 * Logger that should be used for logging the App log messages.
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.getLogger(
    'SmartCardClientApp',
    goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO);

const extensionManifest = chrome.runtime.getManifest();
const formattedStartupTime = (new Date()).toLocaleString();
goog.log.info(
    logger,
    `The extension (id "${chrome.runtime.id}", version ` +
        `${extensionManifest.version}) background script started. Browser ` +
        `version: "${window.navigator.appVersion}". System time: ` +
        `"${formattedStartupTime}"`);

/**
 * Loads the binary executable module depending on the toolchain configuration.
 * @return {!GSC.ExecutableModule}
 */
function createExecutableModule() {
  switch (GSC.ExecutableModule.TOOLCHAIN) {
    case GSC.ExecutableModule.Toolchain.PNACL:
      return new GSC.NaclModule(
          'executable_module.nmf', GSC.NaclModule.Type.PNACL);
    case GSC.ExecutableModule.Toolchain.EMSCRIPTEN:
      return new GSC.EmscriptenModule('executable_module');
  }
  GSC.Logging.fail(
      `Cannot load executable module: unknown toolchain ` +
      `${GSC.ExecutableModule.TOOLCHAIN}`);
  goog.asserts.fail();
}

/**
 * The binary executable module that contains the actual smart card client code.
 * @type {!GSC.ExecutableModule}
 */
const executableModule = createExecutableModule();

/**
 * Called when the executable module is disposed of.
 */
function executableModuleDisposedListener() {
  if (!goog.DEBUG) {
    // Trigger the fatal error in the Release mode, which will emit an error
    // message and cause the app reload (see the GSC.Logging.fail() function
    // implementation in //common/js/src/logging/logging.js).
    GSC.Logging.failWithLogger(logger, 'Executable module was disposed of');
  }
}

// Reload our app when the executable module is disposed of (e.g., due to a
// crash) and we're in the Release mode.
executableModule.addOnDisposeCallback(executableModuleDisposedListener);

/**
 * Translator of all PC/SC-Lite client API requests received from the executable
 * module into the JavaScript PC/SC-Lite client API method calls (see the
 * GoogleSmartCard.PcscLiteClient.API class).
 */
const pcscLiteNaclClientBackend = new GSC.PcscLiteClient.NaclClientBackend(
    executableModule.getMessageChannel(), CLIENT_TITLE, SERVER_APP_ID);

/**
 * Translator of the events from the chrome.certificateProvider API (see
 * <https://developer.chrome.com/extensions/certificateProvider#events>) into
 * requests sent to the executable module.
 */
const certificateProviderBridgeBackend =
    new SmartCardClientApp.CertificateProviderBridge.Backend(executableModule);

// Ignore messages sent from the executable module to the main window when the
// latter is not opened.
executableModule.getMessageChannel().registerService('ui', () => {});

/**
 * Object that handles the built-in PIN dialog requests received from the
 * executable module.
 *
 * NOTE: This should only be used for the PIN requests that aren't associated
 * with signature requests made by Chrome, since for those the
 * chrome.certificateProvider.requestPin() method should be used.
 */
const builtInPinDialogBackend = new SmartCardClientApp.BuiltInPinDialog.Backend(
    executableModule.getMessageChannel());

// Starts the executable module loading. Up to this point, the module was not
// actually loading yet, which allowed to add all the necessary event listeners
// in advance.
executableModule.startLoading();

// Open the UI window when the user launches the app.
chrome.app.runtime.onLaunched.addListener(() => {
  GSC.PopupOpener.createWindow(MAIN_WINDOW_URL, MAIN_WINDOW_OPTIONS, {
    executableModuleMessageChannel: executableModule.getMessageChannel(),
  });
});

// Automatically load the App (in the background) with Chrome startup.
GSC.AppUtils.enableSelfAutoLoading();
});  // goog.scope
