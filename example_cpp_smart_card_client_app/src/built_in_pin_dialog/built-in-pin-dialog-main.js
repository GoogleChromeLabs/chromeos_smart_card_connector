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
 * @fileoverview Main script of the built-in PIN dialog window.
 *
 * Once the dialog is finished with some result (either successfully returning
 * data entered by user, or being canceled), it uses the methods of the
 * GoogleSmartCard.PopupWindow library for returning the result to the caller
 * window (i.e. to the App's background page).
 */

goog.provide('SmartCardClientApp.BuiltInPinDialog.Main');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PopupWindow.Client');
goog.require('goog.dom');
goog.require('goog.dom.forms');
goog.require('goog.events');

goog.scope(function() {

const GSC = GoogleSmartCard;

function okClickListener() {
  const pin = goog.dom.forms.getValue(goog.dom.getElement('input'));
  GSC.PopupWindow.Client.resolveModalDialog(pin);
}

function cancelClickListener() {
  GSC.PopupWindow.Client.rejectModalDialog(new Error(
      'PIN dialog was canceled'));
}

goog.events.listen(goog.dom.getElement('ok'), 'click', okClickListener);
goog.events.listen(goog.dom.getElement('cancel'), 'click', cancelClickListener);

GSC.PopupWindow.Client.prepareAndShowAsModalDialog();

});  // goog.scope
