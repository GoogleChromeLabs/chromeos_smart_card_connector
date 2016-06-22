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

/** TODO
 * @fileoverview This script implements displaying of the devices list and
 * requesting USB permissions in the Smart Card Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.AppsDisplaying');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.events.EventType');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

/**
 * @type {Element}
 * @const
 */
var appListElement = goog.dom.getElement('app-list');

/**
 * @param {!Array.<string>} app_list
 */
function onUpdateListener(app_list) {
  logger.fine('Application list updated, refreshing the view');

  GSC.Logging.checkWithLogger(logger, !goog.isNull(appListElement));
  goog.asserts.assert(appListElement);

  goog.dom.removeChildren(appListElement);

  for (let app_id of app_list) {
    var new_element = goog.dom.createDom('li', undefined, app_id);
    goog.dom.append(appListElement, new_element);
  }
}

GSC.ConnectorApp.Window.AppsDisplaying.initialize = function() {
  logger.fine('Registering listener on connected apps update');
  var addOnUpdateListener = /**@type {function(function(!Array.<string>))} */
                            (GSC.PopupWindow.Client.getData());
  addOnUpdateListener(onUpdateListener);
};

});  // goog.scope
