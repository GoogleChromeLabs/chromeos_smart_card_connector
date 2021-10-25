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
 * @fileoverview Client side of the helper library for implementing the popup
 * windows.
 *
 * Provides methods that are expected to be used inside the panel window script.
 *
 * See also the popup-opener.js file.
 */

goog.provide('GoogleSmartCard.InPopupMainScript');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Packaging');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.events.KeyCodes');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('PopupWindow.InPopupMainScript');

/**
 * Returns the additional data that was specified during the popup creation.
 * @return {!Object}
 */
GSC.InPopupMainScript.getData = function() {
  return goog.object.get(window, 'passedData', {});
};

/**
 * Shows the window.
 */
GSC.InPopupMainScript.showWindow = function() {
  goog.log.fine(logger, 'Showing the window...');
  chrome.app.window.current().show();
};

/**
 * Resolves the modal dialog, passing the specified result to the caller in the
 * parent window (i.e. in the App's background page) and closing the current
 * window.
 * @param {*} result
 */
GSC.InPopupMainScript.resolveModalDialog = function(result, popupId = 0) {
  const callback = GSC.InPopupMainScript.getData()['resolveModalDialog'];
  GSC.Logging.checkWithLogger(logger, callback);
  goog.log.fine(
      logger,
      'The modal dialog is resolved with the following result: ' +
          GSC.DebugDump.debugDump(result));

  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    callback(result);
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    goog.global['opener'][`resolveModalDialog${popupId}`](result);
  }

  closeWindow();
};

/**
 * Rejects the modal dialog, passing the specified error to the caller in the
 * parent window (i.e. in the App's background page) and closing the current
 * window.
 * @param {*} error
 */
GSC.InPopupMainScript.rejectModalDialog = function(error, popupId = 0) {
  const callback = GSC.InPopupMainScript.getData()['rejectModalDialog'];
  GSC.Logging.checkWithLogger(logger, callback);
  goog.log.fine(
      logger,
      'The modal dialog is rejected with the following error: ' + error);

  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    callback(error);
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    goog.global['opener'][`rejectModalDialog${popupId}`](error);
  }

  closeWindow();
};

/**
 * Shows the (initially hidden) dialog modal window, performing the setup steps
 * necessary when the default modal dialog options are used.
 */
GSC.InPopupMainScript.prepareAndShowAsModalDialog = function() {
  GSC.InPopupMainScript.setWindowHeightToFitContent();
  GSC.InPopupMainScript.setupClosingOnEscape();
  GSC.InPopupMainScript.setupRejectionOnWindowClose();
  // In the App packaging mode, popup windows are initially opened hidden (see
  // the flags passed to chrome.app.window.create() in popup-opener.js), to
  // avoid flickering while the UI is being initialized. In the Extension
  // packaging mode, there's no way to open an initially-hidden window, so
  // there's no need to change the visibility here.
  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP)
    GSC.InPopupMainScript.showWindow();
};

/**
 * Resizes the window height so that it fits the entire content without
 * overflowing (though still conforming to the minimum/maximum bounds when they
 * are specified for the window).
 */
GSC.InPopupMainScript.setWindowHeightToFitContent = function() {
  const wholeContentHeight = document.documentElement['offsetHeight'];
  GSC.Logging.checkWithLogger(
      logger,
      wholeContentHeight !== undefined &&
          typeof wholeContentHeight === 'number');
  goog.log.fine(
      logger, 'Resizing the window size to ' + wholeContentHeight + 'px');
  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    chrome.app.window.current().innerBounds.height = wholeContentHeight;
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    chrome.windows.update(
        chrome.windows.WINDOW_ID_CURRENT, {'height': wholeContentHeight});
  }
};

/**
 * Adds a listener for unhandled ESC key presses.
 *
 * The listener closes the window.
 */
GSC.InPopupMainScript.setupClosingOnEscape = function() {
  goog.events.listen(
      document, goog.events.EventType.KEYDOWN,
      documentClosingOnEscapeKeyDownListener);
  goog.log.fine(logger, 'ESC key press handler was set up');
};

/**
 * Adds listener for the window close event, which rejects the dialog (this
 * covers the case when the dialog is closed by user by clicking at the close
 * button).
 */
GSC.InPopupMainScript.setupRejectionOnWindowClose = function() {
  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    chrome.app.window.current().onClosed.addListener(
        windowCloseDialogRejectionListener);
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    chrome.windows.onRemoved.addListener(windowCloseDialogRejectionListener);
  }
};

function closeWindow() {
  goog.log.fine(logger, 'Closing the window...');
  window.close();
}

function documentClosingOnEscapeKeyDownListener(event) {
  if (event.keyCode == goog.events.KeyCodes.ESC) {
    goog.log.fine(logger, 'ESC key press received, the window will be closed');
    closeWindow();
  }
}

function windowCloseDialogRejectionListener() {
  GSC.InPopupMainScript.rejectModalDialog(new Error('Dialog was closed'));
}
});  // goog.scope
