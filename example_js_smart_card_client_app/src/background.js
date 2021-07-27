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
 * * A GoogleSmartCard.PcscLiteClient.Context object is created and its
 *   initialization is started, which attempts to establish a connection to the
 *   server App. Upon successful connection, a
 *   GoogleSmartCard.PcscLiteClient.API object is received, that can be used to
 *   perform PC/SC-Lite client API requests.
 * * Subscribing to a special Chrome Extensions API event that makes the App
 *   auto-loading.
 */

goog.provide('SmartCardClientApp.BackgroundMain');

goog.require('GoogleSmartCard.AppUtils');
goog.require('GoogleSmartCard.BackgroundPageUnloadPreventing');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteClient.API');
goog.require('GoogleSmartCard.PcscLiteClient.Context');
goog.require('GoogleSmartCard.PcscLiteClient.Demo');
goog.require('GoogleSmartCard.PcscLiteCommon.Constants');
goog.require('SmartCardClientApp.PinDialog.Server');
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
const CLIENT_TITLE = 'example_js_client_app';

/** Identifier of the server App. */
const SERVER_APP_ID = Constants.SERVER_OFFICIAL_APP_ID;

/**
 * Logger that should be used for logging the App log messages.
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.getLogger(
    'SmartCardClientApp',
    goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO);

/**
 * Context for using the PC/SC-Lite client API.
 *
 * This object establishes and manages a connection to the server App. Upon
 * successful connection, a GoogleSmartCard.PcscLiteClient.API object is
 * returned through the callback, that allows to perform PC/SC-Lite client API
 * requests.
 * @type {GSC.PcscLiteClient.Context?}
 */
let context = null;

/**
 * Initiates the PC/SC-Lite client API context initialization.
 */
function initializeContext() {
  GSC.Logging.checkWithLogger(logger, context === null);
  context = new GSC.PcscLiteClient.Context(CLIENT_TITLE, SERVER_APP_ID);
  context.addOnInitializedCallback(contextInitializedListener);
  context.addOnDisposeCallback(contextDisposedListener);
  context.initialize();
}

/**
 * This callback is called when the PC/SC-Lite client API context is
 * successfully initialized.
 * @param {!GSC.PcscLiteClient.API} api Object that allows to perform PC/SC-Lite
 * client API requests.
 */
function contextInitializedListener(api) {
  goog.log.info(logger, 'Successfully connected to the server app');
  work(api);
}

/**
 * This callback is called when the PC/SC-Lite client API context is disposed
 * (either it failed to initialize or was disposed later due to some error).
 *
 * The GoogleSmartCard.PcscLiteClient.API that was supplied previously also
 * becomes disposed at this point (if not disposed yet).
 */
function contextDisposedListener() {
  goog.log.warning(logger, 'Connection to the server app was shut down');
  context = null;
  stopWork();
}

/**
 * This function is executed when the context for using PC/SC-Lite client API is
 * initialized successfully.
 * @param {!GSC.PcscLiteClient.API} api Object that allows to perform PC/SC-Lite
 * client API requests.
 */
function work(api) {
  //
  // CHANGE HERE:
  // Place your custom code working with PC/SC-Lite client API here:
  //

  function runPcscLiteDemo(callback) {
    goog.log.info(logger, 'Starting PC/SC-Lite demo...');
    goog.log.setLevel(logger, goog.log.Level.FINE);
    GSC.PcscLiteClient.Demo.run(
        api,
        function() {
          goog.log.info(logger, 'PC/SC-Lite demo successfully finished');
          callback();
        },
        function() {
          goog.log.warning(logger, 'PC/SC-Lite demo failed');
          callback();
        });
  }

  function runPinDialogDemo() {
    goog.log.info(logger, 'Starting PIN dialog demo...');
    const pinPromise = SmartCardClientApp.PinDialog.Server.requestPin();
    pinPromise.then(
        function(pin) {
          goog.log.info(
              logger, 'PIN dialog demo finished: received PIN "' + pin + '"');
        },
        function(error) {
          goog.log.info(logger, 'PIN dialog demo finished: ' + error);
        });
  }

  runPcscLiteDemo(runPinDialogDemo);
}

/**
 * This function is executed when the PC/SC-Lite client API context is disposed
 * (either because it failed to initialize or because it was disposed later due
 * to some error).
 *
 * The GoogleSmartCard.PcscLiteClient.API that was supplied to the work function
 * also becomes disposed at this point (if not disposed yet).
 */
function stopWork() {
  //
  // CHANGE HERE:
  // Place your custom deinitialization code here:
  //
}

initializeContext();

GSC.AppUtils.enableSelfAutoLoading();

GSC.BackgroundPageUnloadPreventing.enable();
});  // goog.scope
