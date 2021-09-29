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
goog.require('GoogleSmartCard.Packaging');
goog.require('goog.Promise');
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
 * Creates a new window.
 * @param {string} url
 * @param {!WindowOptions} windowOptions
 * @param {!Object=} opt_data Optional data to be passed to the created window.
 */
GSC.PopupOpener.createWindow = function(url, windowOptions, opt_data) {
  console.log("ayag createWindow in popup-opener.js");
  const createdWindowExtends = {};
  if (opt_data !== undefined)
    createdWindowExtends['passedData'] = opt_data;

  goog.log.fine(
      logger,
      'Creating a popup window with url="' + url +
          '", options=' + GSC.DebugDump.debugDump(windowOptions) +
          ', data=' + GSC.DebugDump.debugDump(opt_data));

  /** @preserveTry */
  try {
    if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
      chrome.app.window.create(
          url, {
            'alwaysOnTop': windowOptions['alwaysOnTop'],
            'frame': windowOptions['frame'],
            'hidden': windowOptions['hidden'],
            'id': windowOptions['id'],
            'innerBounds': {
              'width': windowOptions['width'],
            },
            'resizable': windowOptions['resizeable'],
            'visibleOnAllWorkspaces': windowOptions['visibleOnAllWorkspaces']
          },
          createWindowCallback.bind(null, createdWindowExtends));
    } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
      console.log("ayag window created window SUCCESSFULLY");
      chrome.windows.create({
        'url': url,
        'type': 'popup',
        'width': windowOptions['width'],
        'setSelfAsOpener': true
      });
    } else {
      GSC.Logging.failWithLogger(
          logger, `Unexpected packaging mode ${GSC.Packaging.MODE}`);
    }
  } catch (exc) {
    GSC.Logging.failWithLogger(
        logger,
        'Failed to create the popup window with URL "' + url + '" and ' +
            'options ' + GSC.DebugDump.debugDump(windowOptions) + ': ' + exc);
  }
};

/**
 * Creates a new modal dialog and returns a promise of the data it returns.
 * @param {string} url
 * @param {!chrome.app.window.CreateWindowOptions=}
 * opt_createWindowOptionsOverrides Overrides to the default window options.
 * Note that the 'id' option is disallowed.
 * @param {!Object=} opt_data Optional data to be passed to the created dialog.
 * @return {!goog.Promise}
 */
GSC.PopupOpener.runModalDialog = function(
    url, popupId, opt_createWindowOptionsOverrides, opt_data) {
  console.log("ayag runModalDialog in popup-opener.js");
  const createWindowOptions =
      goog.object.clone(DEFAULT_DIALOG_CREATE_WINDOW_OPTIONS);
  if (opt_createWindowOptionsOverrides) {
    GSC.Logging.checkWithLogger(
        logger,
        !goog.object.containsKey(opt_createWindowOptionsOverrides, 'id'),
        '"id" window option is disallowed for the modal dialogs (as this ' +
            'option may result in not creating the new modal dialog)');
    Object.assign(createWindowOptions, opt_createWindowOptionsOverrides);
  }

  const promiseResolver = goog.Promise.withResolver();

  const dataWithDialogCallbacks = {
    'resolveModalDialog': promiseResolver.resolve,
    'rejectModalDialog': promiseResolver.reject
  };

  goog.global[`resolveModalDialog${popupId}`] = promiseResolver.resolve;
  goog.global[`rejectModalDialog${popupId}`] = promiseResolver.reject;

  if (opt_data !== undefined)
    Object.assign(dataWithDialogCallbacks, opt_data);

  GSC.PopupOpener.createWindow(
      url, createWindowOptions, dataWithDialogCallbacks);

  console.log("ayag finished runModalDialog");
  return promiseResolver.promise;
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
          GSC.DebugDump.debugDump(createdWindowExtends));
  Object.assign(createdWindowScope, createdWindowExtends);
}
});  // goog.scope
