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
 * @fileoverview Script that runs in the context of the App's main window.
 */

goog.provide('SmartCardClientApp.WindowMain');

goog.require('GoogleSmartCard.I18n');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.ObjectHelpers');
goog.require('GoogleSmartCard.PopupWindow.Client');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.log');
goog.require('goog.log.Level');
goog.require('goog.log.Logger');
goog.require('goog.messaging.MessageChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.getLogger(
    'SmartCardClientAppWindow',
    goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO);

goog.log.info(logger, 'The main window is created');

//#revisitcode
// Obtain the message channel that is used for communication with the executable
// module.
// const executableModuleMessageChannel =
//     /** @type {!goog.messaging.MessageChannel} */
//     (GSC.ObjectHelpers.extractKey(
//          GSC.PopupWindow.Client.getData(), 'executableModuleMessageChannel'));

// Load the localized strings into the HTML elements.
GSC.I18n.adjustAllElementsTranslation();

// Called when the "close" button is clicked. Closes the window.
function onCloseWindowClicked(e) {
  e.preventDefault();
  // chrome.app.window.current().close();
}

// Called when the "run test" button is clicked. Sends a command to the
// executable module to start the test.
// function onRunTestClicked(e) {
//   e.preventDefault();
//   executableModuleMessageChannel.send('ui_backend', {command: 'run_test'});
// }

// Called when a "output_message" message is received from the executable
// module. Adds a line into the "output" <pre> element in the UI.
function displayOutputMessage(text) {
  const outputElem = goog.dom.getElement('output');
  outputElem.innerHTML = outputElem.innerHTML + text + '\n';
}

//revisitcode
// Set up UI event listeners.
// goog.events.listen(
//     goog.dom.getElement('close-window'), goog.events.EventType.CLICK,
//     onCloseWindowClicked);
// goog.events.listen(
//     goog.dom.getElement('run-test'), goog.events.EventType.CLICK,
//     onRunTestClicked);

//revisitcode
// Handle messages from the executable module.
// executableModuleMessageChannel.registerService('ui', (payload) => {
//   //
//   // CHANGE HERE:
//   // Place your custom code here:
//   //

//   if (payload['output_message'])
//     displayOutputMessage(payload['output_message']);
// }, /*opt_objectPayload=*/ true);

//#revisitcode
// Unregister from receiving messages from the module when our window is closed.
// chrome.app.window.current().onClosed.addListener(() => {
//   executableModuleMessageChannel.registerService('ui', () => {});
// });

// Show the window, after all the initialization above has been done.
// GSC.PopupWindow.Client.showWindow();

});  // goog.scope
