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
 * @fileoverview Main script of the Smart Card Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.Main');

goog.require('GoogleSmartCard.ConnectorApp.Window.AboutShowing');
goog.require('GoogleSmartCard.ConnectorApp.Window.AppsDisplaying');
goog.require('GoogleSmartCard.ConnectorApp.Window.DevicesDisplaying');
goog.require('GoogleSmartCard.ConnectorApp.Window.HelpShowing');
goog.require('GoogleSmartCard.ConnectorApp.Window.LogsExporting');
goog.require('GoogleSmartCard.ConnectorApp.Window.StatusDisplaying');
goog.require('GoogleSmartCard.I18n');
goog.require('GoogleSmartCard.InPopupMainScript');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Packaging');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.log');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

goog.log.info(logger, 'The main window is created');

const closeButton = goog.dom.getElement('close-window');
if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
  goog.events.listen(
      closeButton, goog.events.EventType.CLICK, closeWindowClickListener);
} else {
  // It's easy to close the action popup by clicking elsewhere - there's no need
  // for a special button.
  closeButton.remove();
}

GSC.ConnectorApp.Window.AboutShowing.initialize();
GSC.ConnectorApp.Window.AppsDisplaying.initialize();
GSC.ConnectorApp.Window.DevicesDisplaying.initialize();
GSC.ConnectorApp.Window.HelpShowing.initialize();
GSC.ConnectorApp.Window.LogsExporting.initialize();
GSC.ConnectorApp.Window.StatusDisplaying.initialize();

GSC.I18n.adjustAllElementsTranslation();

if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP)
  GSC.InPopupMainScript.showWindow();

function closeWindowClickListener() {
  window.close();
}
});  // goog.scope
