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
 * @fileoverview Backend that handles PIN dialog requests received from the NaCl
 * module.
 */

goog.provide('SmartCardClientApp.PinDialog.Backend');

goog.require('GoogleSmartCard.PopupWindow.Server');
goog.require('GoogleSmartCard.RequestHandler');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('goog.Promise');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

/** @const */
var REQUESTER_NAME = 'pin_dialog';

/** @const */
var PIN_MESSAGE_KEY = 'pin';

/** @const */
var PIN_DIALOG_URL = 'pin-dialog.html';

/** @const */
var PIN_DIALOG_WINDOW_OPTIONS_OVERRIDES = {
  'innerBounds': {
    'width': 230
  }
};

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getLogger('SmartCardClientApp.PinDialog');

/**
 * Backend that handles PIN dialog requests received from the NaCl module.
 *
 * On construction, subscribes at the passed message channel for receiving
 * messages of the special type representing PIN requests.
 *
 * Once the message with the PIN request is received, opens the PIN dialog and,
 * once it finishes, sends its result as a message through the message channel.
 * @param {!goog.messaging.AbstractChannel} naclModuleMessageChannel
 * @constructor
 */
SmartCardClientApp.PinDialog.Backend = function(naclModuleMessageChannel) {
  /** @private */
  this.chromeUsbRequestReceiver_ = new GSC.RequestReceiver(
      REQUESTER_NAME, naclModuleMessageChannel, new PinDialogRequestHandler);
};

/**
 * @implements {GSC.RequestHandler}
 * @constructor
 */
function PinDialogRequestHandler() {}

/**
 * @param {!Object} payload
 * @return {!goog.Promise}
 */
PinDialogRequestHandler.prototype.handleRequest = function(payload) {
  logger.info('Starting PIN dialog...');

  var pinPromise = GSC.PopupWindow.Server.runModalDialog(
      PIN_DIALOG_URL, PIN_DIALOG_WINDOW_OPTIONS_OVERRIDES);

  var promiseResolver = goog.Promise.withResolver();

  pinPromise.then(function(pin) {
    logger.info('PIN dialog finished successfully');
    promiseResolver.resolve(createResponseMessagePayload(pin));
  }, function(error) {
    logger.info('PIN dialog finished with error: ' + error);
    promiseResolver.reject(error);
  });

  return promiseResolver.promise;
};

function createResponseMessagePayload(pin) {
  return goog.object.create(PIN_MESSAGE_KEY, pin);
}

});  // goog.scope
