/**
 * @license
 * Copyright 2018 Google Inc. All Rights Reserved.
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
 * @fileoverview This script implements showing the main Smart Card Connector
 * App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Background.MainWindowManaging');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PopupOpener');

goog.scope(function() {

const WINDOW_URL = 'window.html';
const WINDOW_ID = 'main-window';

/**
 * Note: the bounds of the window were carefully adjusted in order to fit the
 * USB device selector dialog (as shown by the chrome.usb.getUserSelectedDevices
 * method: refer to
 * <https://developer.chrome.com/apps/usb#method-getUserSelectedDevices>).
 * @type {WindowOptions} 
 */
const WINDOW_OPTIONS = {
  'frame': 'none',
  'hidden': true,
  'id': WINDOW_ID,
  'width': 530,
  'resizable': false
};


const GSC = GoogleSmartCard;

/**
 * Opens the main window due to user request. Does nothing if the window is
 * already opened.
 */
GSC.ConnectorApp.Background.MainWindowManaging.openWindowDueToUserRequest =
    function() {
  GSC.PopupOpener.createWindow(WINDOW_URL, WINDOW_OPTIONS);
};
});  // goog.scope
