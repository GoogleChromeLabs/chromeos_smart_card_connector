/**
 * @license
 * Copyright 2023 Google Inc. All Rights Reserved.
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
 * @fileoverview This script implements displaying the application's overall
 * status in the Smart Card Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.StatusDisplaying');

goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.ObjectHelpers');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.dom.classlist');
goog.require('goog.log');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

function displayNonChromeOsWarningIfNeeded() {
  chrome.runtime.getPlatformInfo(function(platformInfo) {
    /** @type {string} */
    const os = platformInfo['os'];
    if (os != 'cros') {
      goog.log.info(
          logger,
          'Displaying the warning regarding non-ChromeOS system ' +
              '(the current OS is "' + os + '")');
      goog.dom.classlist.remove(
          goog.dom.getElement('non-chrome-os-warning'), 'hidden');
    }
  });
}

/**
 * @param {!Window=} backgroundPage
 */
function initializeWithBackgroundPage(backgroundPage) {
  GSC.Logging.checkWithLogger(logger, backgroundPage);
  goog.asserts.assert(backgroundPage);

  /**
   * Points to the "addOnExecutableModuleDisposedListener" method from the
   * background page.
   */
  const executableModuleDisposalSubscriber =
      /** @type {function(function())} */
      (GSC.ObjectHelpers.extractKey(
          backgroundPage,
          'googleSmartCard_executableModuleDisposalSubscriber'));
  // Start tracking the disposal status. Note that the callback is called
  // immediately if the executable module is already disposed of.
  executableModuleDisposalSubscriber(onExecutableModuleDisposed);
}

/**
 * Called when the C/C++ executable module gets disposed of.
 */
function onExecutableModuleDisposed() {
  goog.dom.classlist.remove(goog.dom.getElement('crash-warning'), 'hidden');
}

GSC.ConnectorApp.Window.StatusDisplaying.initialize = function() {
  displayNonChromeOsWarningIfNeeded();
  chrome.runtime.getBackgroundPage(initializeWithBackgroundPage);
};
});
