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
 * @fileoverview Main script of the Smart Card Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.Main');

goog.require('GoogleSmartCard.ConnectorApp.Window.AboutShowing');
goog.require('GoogleSmartCard.ConnectorApp.Window.AppsDisplaying');
goog.require('GoogleSmartCard.ConnectorApp.Window.DevicesDisplaying');
goog.require('GoogleSmartCard.ConnectorApp.Window.LogsExporting');
goog.require('GoogleSmartCard.I18n');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PopupWindow.Client');
goog.require('goog.dom');
goog.require('goog.events');
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

logger.fine('The window script has started...');

function closeWindowClickListener() {
  chrome.app.window.current().close();
}

goog.events.listen(
    goog.dom.getElement('close-window'),
    goog.events.EventType.CLICK,
    closeWindowClickListener);

GSC.ConnectorApp.Window.AboutShowing.initialize();
GSC.ConnectorApp.Window.AppsDisplaying.initialize();
GSC.ConnectorApp.Window.DevicesDisplaying.initialize();
GSC.ConnectorApp.Window.LogsExporting.initialize();

GSC.PopupWindow.Client.showWindow();

// This call does all of the translation work.
GSC.I18n.adjustElementsTranslation();

});  // goog.scope
