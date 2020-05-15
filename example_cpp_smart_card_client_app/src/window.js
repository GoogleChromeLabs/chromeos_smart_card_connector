/** @license
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
 * @fileoverview Script that runs in the context of the App's main window.
 */

goog.provide('SmartCardClientApp.WindowMain');

goog.require('GoogleSmartCard.I18n');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PopupWindow.Client');
goog.require('goog.dom');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.getLogger(
    'SmartCardClientAppWindow',
    goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO);

logger.info('The main window is created');

// Load the localized strings into the HTML elements.
GSC.I18n.adjustElementsTranslation();

// Called when the "close" button is clicked. Closes the window.
function onCloseWindowClicked(e) {
  e.preventDefault();
  chrome.app.window.current().close();
}

// Set up event listeners.
goog.events.listen(
    goog.dom.getElement('close-window'),
    goog.events.EventType.CLICK,
    onCloseWindowClicked);

// Show the window, after all the initialization above has been done.
GSC.PopupWindow.Client.showWindow();

});  // goog.scope
