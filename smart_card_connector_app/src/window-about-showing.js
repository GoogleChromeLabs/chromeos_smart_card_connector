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
 * @fileoverview This script implements showing the About window in the Smart
 * Card Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.AboutShowing');

goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.EventType');

goog.scope(function() {

const ABOUT_WINDOW_URL = 'about-window.html';
const ABOUT_WINDOW_OPTIONS = {
  'id': 'about-window',
  'innerBounds': {'width': 700, 'height': 500}
};

const GSC = GoogleSmartCard;

/** @type {!Element} */
const openAboutElement =
    /** @type {!Element} */ (goog.dom.getElement('open-about'));

/**
 * @param {!Event} e
 */
function openAboutClickListener(e) {
  e.preventDefault();

  if (GSC.Packaging.MODE == GSC.Packaging.Mode.APP) {
    chrome.app.window.create(ABOUT_WINDOW_URL, ABOUT_WINDOW_OPTIONS);
  } else if (GSC.Packaging.MODE == GSC.Packaging.Mode.EXTENSION) {
    chrome.windows.create({
      url: ABOUT_WINDOW_URL,
      type: 'popup',
      width: ABOUT_WINDOW_OPTIONS['innerBounds']['width'],
      height: ABOUT_WINDOW_OPTIONS['innerBounds']['height']
    });
  }
}

GSC.ConnectorApp.Window.AboutShowing.initialize = function() {
  goog.events.listen(
      openAboutElement, goog.events.EventType.CLICK, openAboutClickListener);
};
});  // goog.scope
