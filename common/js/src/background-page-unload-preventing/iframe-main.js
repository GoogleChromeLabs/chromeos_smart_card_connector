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
 * @fileoverview Entry point of the main script in the background page unload
 * preventing iframe.
 *
 * By default, App's background page gets automatically unloaded by Chrome after
 * some period of inactivity (see
 * <https://developer.chrome.com/apps/event_pages>). In order to suppress such
 * behavior, an iframe with this script can be created from the background
 * page. The script opens a Port that connects to the same App (the other end
 * should be handled in the background page). Keeping the Port opened prevents
 * the background page from being unloaded.
 */

goog.provide('GoogleSmartCard.BackgroundPageUnloadPreventing.IFrameMain');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger =
    GSC.Logging.getScopedLogger('BackgroundPageUnloadPreventing.IFrame');

logger.fine('Opening a port to the background page...');
/** @type {!Port} */
const port =
    chrome.runtime.connect({'name': 'background-page-unload-preventing'});

port.onDisconnect.addListener(function() {
  GSC.Logging.failWithLogger(
      logger, 'The message port to the background page was disconnected');
});

port.onMessage.addListener(function() {
  GSC.Logging.failWithLogger(
      logger,
      'Unexpectedly received a message through the message port from the ' +
      'background page');
});

});  // goog.scope
