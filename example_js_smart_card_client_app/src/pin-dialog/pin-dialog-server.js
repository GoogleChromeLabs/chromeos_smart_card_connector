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

goog.provide('SmartCardClientApp.PinDialog.Server');

goog.require('GoogleSmartCard.PopupOpener');
goog.require('goog.Promise');

goog.scope(function() {

const PIN_DIALOG_URL = new URL('pin-dialog.html', window.location.href);

const PIN_DIALOG_WINDOW_OPTIONS_OVERRIDES = {
  'innerBounds': {'width': 230}
};

const GSC = GoogleSmartCard;

/**
 * @return {!goog.Promise.<string>}
 */
SmartCardClientApp.PinDialog.Server.requestPin = function() {
  return GSC.PopupOpener.runModalDialog(
      PIN_DIALOG_URL, PIN_DIALOG_WINDOW_OPTIONS_OVERRIDES);
};
});  // goog.scope
