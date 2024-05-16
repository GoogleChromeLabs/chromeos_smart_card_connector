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
 * @fileoverview Server side of the helper library for implementing popup
 * windows.
 */

goog.provide('GoogleSmartCard.PopupOpener');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageWaiter');
goog.require('GoogleSmartCard.Packaging');
goog.require('GoogleSmartCard.PopupConstants');
goog.require('GoogleSmartCard.PortMessageChannelWaiter');
goog.require('goog.Promise');
goog.require('goog.Thenable');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * The window options to be used for the resulting window.
 * APP mode supports all following options.
 * EXTENSION mode supports 'width' only.
 * @typedef {{
 *            alwaysOnTop:(boolean|undefined),
 *            frame:(string|undefined),
 *            hidden:(boolean|undefined),
 *            id:(string|undefined),
 *            width:(number|undefined),
 *            resizeable:(boolean|undefined),
 *            visibleOnAllWorkspaces:(boolean|undefined)
 *          }}
 */
let WindowOptions;

/** @const */
GSC.PopupOpener.WindowOptions = WindowOptions;

/** @type {!Map.<string, number>} */
const windowIdMap = new Map();

/** @type {!GSC.PopupOpener.WindowOptions} */
const DEFAULT_DIALOG_CREATE_WINDOW_OPTIONS = {
  'alwaysOnTop': true,
  'frame': 'none',
  'hidden': true,
  'resizable': false,
  'visibleOnAllWorkspaces': true
};

/**
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.getScopedLogger('PopupWindow.PopupOpener');

/**
 * ID used to differentiate between different popup instances.
 */
let lastUsedPopupId = 0;

/**
 * Variable to track whether window removal is already being observed.
 */
let observingWindowRemoval = false;

/**
 * Creates a new window.
 * @param {string} url
 * @param {!WindowOptions} windowOptions
 * @param {!Object=} opt_data Optional data to be passed to the created window.
 */
GSC.PopupOpener.createWindow = function(url, windowOptions, opt_data) {
  const createdWindowExtends = {};
  if (opt_data !== undefined)
    createdWindowExtends['passedData'] = opt_data;

  goog.log.fine(
      logger,
      'Creating a popup window with url="' + url +
          '", options=' + GSC.DebugDump.debugDumpSanitized(windowOptions) +
          ', data=' + GSC.DebugDump.debugDumpSanitized(opt_data));

  /** @preserveTry */
  try {
    if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
      chrome.app.window.create(
          url, {
            'alwaysOnTop': windowOptions['alwaysOnTop'],
            'frame': windowOptions['frame'],
            'hidden': windowOptions['hidden'],
            'id': windowOptions['id'],
            'innerBounds': {'width': windowOptions['width']},
            'resizable': windowOptions['resizeable'],
            'visibleOnAllWorkspaces': windowOptions['visibleOnAllWorkspaces']
          },
          createWindowCallback.bind(null, createdWindowExtends));
    } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
      if (observingWindowRemoval === false) {
        chrome.windows.onRemoved.addListener((windowId) => {
          windowIdMap.delete(windowId);
        });
        observingWindowRemoval = true;
      }
      chrome.windows.create(
          {'url': url, 'type': 'popup', 'width': windowOptions['width']},
          (createdWindow) => {
            windowIdMap.set(windowOptions['id'], createdWindow.id);
          });
    } else {
      GSC.Logging.failWithLogger(
          logger, `Unexpected packaging mode ${GSC.Packaging.MODE}`);
    }
  } catch (exc) {
    GSC.Logging.failWithLogger(
        logger,
        'Failed to create the popup window with URL "' + url + '" and ' +
            'options ' + GSC.DebugDump.debugDumpSanitized(windowOptions) +
            ': ' + exc);
  }
};

/**
 * Creates a new modal dialog and returns a promise of the data it returns.
 * @param {string} url
 * @param {!WindowOptions=} opt_windowOptions
 * @param {!Object=} opt_data Optional data to be passed to the created dialog.
 * @return {!Promise}
 */
GSC.PopupOpener.runModalDialog = function(url, opt_windowOptions, opt_data) {
  const createWindowOptions =
      goog.object.clone(DEFAULT_DIALOG_CREATE_WINDOW_OPTIONS);
  if (opt_windowOptions !== undefined)
    Object.assign(createWindowOptions, opt_windowOptions);

  // Additionally pass the auto-generated popup ID. The popup will use it to
  // talk back to us when sending the result.
  ++lastUsedPopupId;
  const modifiedData = {[GSC.PopupConstants.POPUP_ID_KEY]: lastUsedPopupId};
  if (opt_data !== undefined)
    Object.assign(modifiedData, opt_data);

  const modifiedUrl = new URL(url, globalThis.location.href);

  if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    modifiedUrl.searchParams.append(
        'passed_data', JSON.stringify(modifiedData));
  }

  // Start listening for the response message before opening the popup.
  const promise = waitForPopupResult(lastUsedPopupId);

  GSC.PopupOpener.createWindow(
      modifiedUrl.pathname + modifiedUrl.search + modifiedUrl.hash,
      createWindowOptions, modifiedData);
  return promise;
};

/**
 * Close the modal dialog with the given window ID.
 * @param {string} id The window ID of the window to be closed.
 */
GSC.PopupOpener.closeModalDialog = async function(id) {
  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    const window = chrome.app.window.get(id);
    if (window !== null) {
      window.close();
    }
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    const windowId = windowIdMap.get(id);
    if (windowId !== undefined) {
      windowIdMap.delete(id);
      await chrome.windows.remove(windowId);
    }
  }
};

/**
 * @param {!Object} createdWindowExtends
 * @param {!chrome.app.window.AppWindow} createdWindow
 */
function createWindowCallback(createdWindowExtends, createdWindow) {
  GSC.Logging.checkWithLogger(
      logger, createdWindow,
      'Failed to create the popup window: the returned App window object is ' +
          'false');

  const createdWindowScope = createdWindow['contentWindow'];

  goog.log.log(
      logger, goog.log.Level.FINER,
      'The popup window callback is executed, injecting the following data ' +
          'into the created window: ' +
          GSC.DebugDump.debugDumpSanitized(createdWindowExtends));
  Object.assign(createdWindowScope, createdWindowExtends);
}

/**
 * Resolves/rejects to the result/error received from the specified popup.
 * @param {number} popupId
 * @return {!Promise}
 */
async function waitForPopupResult(popupId) {
  const portWaiter = new GSC.PortMessageChannelWaiter(
      GSC.PopupConstants.PORT_NAME_PREFIX + popupId);
  const port = await portWaiter.getPromise();
  // Info-level messages from the generic port aren't very useful here.
  GSC.Logging.setLoggerVerbosityAtMost(port.logger, goog.log.Level.WARNING);

  // This can throw, indicating that the dialog has been closed.
  const result = await GSC.MessageWaiter.wait(
      port, GSC.PopupConstants.RESULT_MESSAGE_TYPE);

  // Extract the result: either an error (needs to be thrown) or a value.
  if (result.hasOwn(GSC.PopupConstants.RESULT_ERROR_KEY)) {
    throw result[GSC.PopupConstants.RESULT_ERROR_KEY];
  }
  return result[GSC.PopupConstants.RESULT_VALUE_KEY];
}
});  // goog.scope
