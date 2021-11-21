/**
 * @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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
 * @fileoverview Backend that handles built-in PIN dialog requests received from
 * the executable module.
 *
 * NOTE: This should only be used for the PIN requests that aren't associated
 * with signature requests made by Chrome, since for those the
 * chrome.certificateProvider.requestPin() method should be used.
 */

goog.provide('SmartCardClientApp.BuiltInPinDialog.Backend');

goog.require('GoogleSmartCard.PopupOpener');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Promise');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

// Note: these parameters should stay in sync with the C++ side
// (pin_dialog_server.cc).
const REQUESTER_NAME = 'built_in_pin_dialog';
const PIN_MESSAGE_KEY = 'pin';

const PIN_DIALOG_URL = 'built-in-pin-dialog.html';

const PIN_DIALOG_WINDOW_OPTIONS_OVERRIDES = {
  'alwaysOnTop': false,
  'innerBounds': {'width': 230}
};

const GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.getLogger('SmartCardClientApp.BuiltInPinDialog');

/**
 * Backend that handles built-in PIN dialog requests received from the
 * executable module.
 *
 * On construction, subscribes at the passed message channel for receiving
 * messages of the special type representing PIN requests.
 *
 * Once the message with the PIN request is received, opens the built-in PIN
 * dialog and, once it finishes, sends its result as a message through the
 * message channel.
 * @param {!goog.messaging.AbstractChannel} executableModuleMessageChannel
 * @constructor
 */
SmartCardClientApp.BuiltInPinDialog.Backend = function(
    executableModuleMessageChannel) {
  // Note: the request receiver instance is not stored anywhere, as it makes
  // itself being owned by the message channel.
  new GSC.RequestReceiver(
      REQUESTER_NAME, executableModuleMessageChannel, handleRequest);
};

const Backend = SmartCardClientApp.BuiltInPinDialog.Backend;

/**
 * @param {!Object} payload
 * @return {!goog.Promise}
 */
function handleRequest(payload) {
  goog.log.info(logger, 'Starting PIN dialog...');

  const pinPromise = GSC.PopupOpener.runModalDialog(
      PIN_DIALOG_URL, PIN_DIALOG_WINDOW_OPTIONS_OVERRIDES);

  const promiseResolver = goog.Promise.withResolver();

  pinPromise.then(
      function(pin) {
        goog.log.info(logger, 'PIN dialog finished successfully');
        promiseResolver.resolve(createResponseMessagePayload(pin));
      },
      function(error) {
        goog.log.info(logger, 'PIN dialog finished with error: ' + error);
        promiseResolver.reject(error);
      });

  return promiseResolver.promise;
}

function createResponseMessagePayload(pin) {
  return goog.object.create(PIN_MESSAGE_KEY, pin);
}
});  // goog.scope
