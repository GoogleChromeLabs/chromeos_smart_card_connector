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

goog.require('GoogleSmartCard.Packaging');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.BrowserEvent');
goog.require('goog.events.EventType');

goog.scope(function() {

const ABOUT_WINDOW_URL = 'about-window.html';

const ABOUT_WINDOW_WIDTH = 700;
const ABOUT_WINDOW_HEIGHT = 500;

const APP_ABOUT_WINDOW_OPTIONS = {
  'id': 'about-window',
  'innerBounds': {'width': ABOUT_WINDOW_WIDTH, 'height': ABOUT_WINDOW_HEIGHT}
};

const EXTENSION_ABOUT_WINDOW_OPTIONS = {
  url: ABOUT_WINDOW_URL,
  type: 'popup',
  width: ABOUT_WINDOW_WIDTH,
  height: ABOUT_WINDOW_HEIGHT
};

const GSC = GoogleSmartCard;

/** @type {!Element} */
const openAboutElement =
    /** @type {!Element} */ (goog.dom.getElement('open-about'));

/**
 * @param {!goog.events.BrowserEvent} e
 */
function openAboutClickListener(e) {
  e.preventDefault();

  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    chrome.app.window.create(ABOUT_WINDOW_URL, APP_ABOUT_WINDOW_OPTIONS);
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    chrome.windows.create(EXTENSION_ABOUT_WINDOW_OPTIONS);
  }
}

GSC.ConnectorApp.Window.AboutShowing.initialize = function() {
  goog.events.listen(
      openAboutElement, goog.events.EventType.CLICK, openAboutClickListener);
};
});  // goog.scope
