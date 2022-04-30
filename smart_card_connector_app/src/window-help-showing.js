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
 * @fileoverview This script implements showing the Help Article in a browser
 * window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.HelpShowing');

goog.require('GoogleSmartCard.Packaging');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.BrowserEvent');
goog.require('goog.events.EventType');

goog.scope(function() {

const HELP_WINDOW_URL = 'https://support.google.com/chrome/a/answer/7014689';

const GSC = GoogleSmartCard;

const openHelpElement =
    /** @type {!Element} */ (goog.dom.getElement('open-help'));

/**
 * @param {!goog.events.BrowserEvent} event
 */
function openHelpClickListener(event) {
  event.preventDefault();
  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    chrome.browser.openTab({url: HELP_WINDOW_URL});
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    chrome.windows.create({url: HELP_WINDOW_URL});
  }
}

GSC.ConnectorApp.Window.HelpShowing.initialize = function() {
  goog.events.listen(
      openHelpElement, goog.events.EventType.CLICK, openHelpClickListener);
};
});  // goog.scope
