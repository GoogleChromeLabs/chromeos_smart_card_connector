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
 * @fileoverview Client side of the helper library for implementing the popup
 * windows.
 *
 * Provides methods that are expected to be used inside the panel window script.
 *
 * See also the server.js file.
 */

goog.provide('GoogleSmartCard.PopupWindow.Client');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.events.KeyCodes');
goog.require('goog.log.Logger');
goog.require('goog.object');
goog.require('goog.string');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('PopupWindow.Client');

/**
 * Returns the additional data that was specified during the popup creation.
 * @return {!Object}
 */
GSC.PopupWindow.Client.getData = function() {
  return goog.object.get(window, 'passedData', {});
};

/**
 * Shows the window.
 */
GSC.PopupWindow.Client.showWindow = function() {
  logger.fine('Showing the window...');
  chrome.app.window.current().show();
};

/**
 * Resolves the modal dialog, passing the specified result to the caller in the
 * parent window (i.e. in the App's background page) and closing the current
 * window.
 * @param {*} result
 */
GSC.PopupWindow.Client.resolveModalDialog = function(result) {
  var callback = GSC.PopupWindow.Client.getData()['resolveModalDialog'];
  GSC.Logging.checkWithLogger(logger, callback);
  logger.fine('The modal dialog is resolved with the following result: ' +
              GSC.DebugDump.debugDump(result));
  callback(result);
  closeWindow();
};

/**
 * Rejects the modal dialog, passing the specified error to the caller in the
 * parent window (i.e. in the App's background page) and closing the current
 * window.
 * @param {*} error
 */
GSC.PopupWindow.Client.rejectModalDialog = function(error) {
  var callback = GSC.PopupWindow.Client.getData()['rejectModalDialog'];
  GSC.Logging.checkWithLogger(logger, callback);
  logger.fine('The modal dialog is rejected with the following error: ' +
              error);
  callback(error);
  closeWindow();
};

/**
 * Shows the (initially hidden) dialog modal window, performing the setup steps
 * necessary when the default modal dialog options are used.
 */
GSC.PopupWindow.Client.prepareAndShowAsModalDialog = function() {
  GSC.PopupWindow.Client.setWindowHeightToFitContent();
  GSC.PopupWindow.Client.setupClosingOnEscape();
  GSC.PopupWindow.Client.setupRejectionOnWindowClose();
  GSC.PopupWindow.Client.showWindow();
};

/**
 * Resizes the window height so that it fits the entire content without
 * overflowing (though still conforming to the minimum/maximum bounds when they
 * are specified for the window).
 */
GSC.PopupWindow.Client.setWindowHeightToFitContent = function() {
  var wholeContentHeight = document.documentElement.offsetHeight;
  logger.fine('Resizing the window size to ' + wholeContentHeight + 'px');
  chrome.app.window.current().innerBounds.height = wholeContentHeight;
};

/**
 * Adds a listener for unhandled ESC key presses.
 *
 * The listener closes the window.
 */
GSC.PopupWindow.Client.setupClosingOnEscape = function() {
  goog.events.listen(
      document,
      goog.events.EventType.KEYDOWN,
      documentClosingOnEscapeKeyDownListener);
  logger.fine('ESC key press handler was set up');
};

/**
 * Adds listener for the window close event, which rejects the dialog (this
 * covers the case when the dialog is closed by user by clicking at the close
 * button).
 */
GSC.PopupWindow.Client.setupRejectionOnWindowClose = function() {
  chrome.app.window.current().onClosed.addListener(
      windowCloseDialogRejectionListener);
};

function closeWindow() {
  logger.fine('Closing the window...');
  chrome.app.window.current().close();
}

function documentClosingOnEscapeKeyDownListener(event) {
  if (event.keyCode == goog.events.KeyCodes.ESC) {
    logger.fine('ESC key press received, the window will be closed');
    closeWindow();
  }
}

function windowCloseDialogRejectionListener() {
  GSC.PopupWindow.Client.rejectModalDialog(new Error('Dialog was closed'));
}

});  // goog.scope
