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
 * @fileoverview Server side of the helper library for implementing popup
 * windows.
 */

goog.provide('GoogleSmartCard.PopupWindow.Server');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.Promise');
goog.require('goog.log.Logger');
goog.require('goog.object');
goog.require('goog.promise.Resolver');

goog.scope(function() {

/** @const */
var DEFAULT_DIALOG_CREATE_WINDOW_OPTIONS = {
  'alwaysOnTop': true,
  'frame': 'none',
  'hidden': true,
  'resizable': false,
  'visibleOnAllWorkspaces': true
};

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('PopupWindow.Server');

/**
 * Creates a new window.
 * @param {string} url
 * @param {!chrome.app.window.CreateWindowOptions} createWindowOptions Window
 * options.
 * @param {!Object=} opt_data Optional data to be passed to the created window.
 */
GSC.PopupWindow.Server.createWindow = function(
    url, createWindowOptions, opt_data) {
  var createdWindowExtends = {};
  createdWindowExtends[GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME] =
      GSC.Logging.getLogBuffer();
  if (opt_data !== undefined)
    createdWindowExtends['passedData'] = opt_data;

  logger.fine(
      'Creating a popup window with url="' + url + '", options=' +
      GSC.DebugDump.debugDump(createWindowOptions) + ', data=' +
      GSC.DebugDump.debugDump(opt_data));

  /** @preserveTry */
  try {
    chrome.app.window.create(
        url,
        createWindowOptions,
        createWindowCallback.bind(null, createdWindowExtends));
  } catch (exc) {
    GSC.Logging.failWithLogger(
        logger,
        'Failed to create the popup window with URL "' + url + '" and ' +
        'options ' + GSC.DebugDump.debugDump(createWindowOptions) + ': ' + exc);
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
GSC.PopupWindow.Server.runModalDialog = function(
    url, opt_createWindowOptionsOverrides, opt_data) {
  var createWindowOptions = goog.object.clone(
      DEFAULT_DIALOG_CREATE_WINDOW_OPTIONS);
  if (opt_createWindowOptionsOverrides) {
    GSC.Logging.checkWithLogger(
        logger,
        !goog.object.containsKey(opt_createWindowOptionsOverrides, 'id'),
        '"id" window option is disallowed for the modal dialogs (as this ' +
        'option may result in not creating the new modal dialog)');
    Object.assign(createWindowOptions, opt_createWindowOptionsOverrides);
  }

  var promiseResolver = goog.Promise.withResolver();

  var dataWithDialogCallbacks = {
    'resolveModalDialog': promiseResolver.resolve,
    'rejectModalDialog': promiseResolver.reject
  };
  if (opt_data !== undefined)
    Object.assign(dataWithDialogCallbacks, opt_data);

  GSC.PopupWindow.Server.createWindow(
      url, createWindowOptions, dataWithDialogCallbacks);

  return promiseResolver.promise;
};

/**
 * @param {!Object} createdWindowExtends
 * @param {!chrome.app.window.AppWindow} createdWindow
 */
function createWindowCallback(createdWindowExtends, createdWindow) {
  GSC.Logging.checkWithLogger(
      logger,
      createdWindow,
      'Failed to create the popup window: the returned App window object is ' +
      'false');

  var createdWindowScope = createdWindow['contentWindow'];

  logger.finer(
      'The popup window callback is executed, injecting the following data ' +
      'into the created window: ' +
      GSC.DebugDump.debugDump(createdWindowExtends));
  Object.assign(createdWindowScope, createdWindowExtends);
}

});  // goog.scope
