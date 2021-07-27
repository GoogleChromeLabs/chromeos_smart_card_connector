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

goog.provide('GoogleSmartCard.BackgroundPageUnloadPreventing');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.dom');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('BackgroundPageUnloadPreventing');

/** @type {boolean} */
let enabled = false;

/**
 * Enables preventing of the background page from being unloaded.
 *
 * By default, App's background page gets automatically unloaded by Chrome after
 * some period of inactivity (see
 * <https://developer.chrome.com/apps/event_pages>).
 *
 * This function allows to suppress this behavior by creating an iframe with the
 * script that opens a Port connecting to the same App (the other end
 * is handled here, in the background page). Keeping the Port opened prevents
 * the background page from being unloaded.
 */
GSC.BackgroundPageUnloadPreventing.enable = function() {
  if (enabled) {
    logger.fine('Trying to enable for the second time. Ignoring this attempt');
    return;
  }
  enabled = true;

  chrome.runtime.onConnect.addListener(connectListener);

  chrome.runtime.onSuspend.addListener(suspendListener);

  logger.fine('Enabling: creating an iframe...');
  createIFrameElement();
};

function createIFrameElement() {
  const iframe = goog.dom.createDom(
      'iframe', {'src': 'background-page-unload-preventing-iframe.html'});
  GSC.Logging.checkWithLogger(logger, document.body);
  document.body.appendChild(iframe);
}

/**
 * @param {!Port} port
 */
function connectListener(port) {
  if (port.name == 'background-page-unload-preventing') {
    logger.fine('Success: received a port opened by the iframe');

    port.onDisconnect.addListener(function() {
      GSC.Logging.failWithLogger(
          logger, 'The message port to the iframe was disconnected');
    });

    port.onMessage.addListener(function() {
      GSC.Logging.failWithLogger(
          logger,
          'Unexpectedly received a message from the message port from the ' +
              'iframe');
    });
  }
}

function suspendListener() {
  GSC.Logging.failWithLogger(logger, 'Suspend event was received');
}
});  // goog.scope
