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
 * @fileoverview This script implements displaying a list of connected
 * applications in the Smart Card Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.AppsDisplaying');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.KnownApp');
goog.require('GoogleSmartCard.PcscLiteServerClientsManagement.PermissionsChecking.KnownAppsRegistry');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.log');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;
const PermissionsChecking =
    GSC.PcscLiteServerClientsManagement.PermissionsChecking;
const KnownApp = PermissionsChecking.KnownApp;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

/** @type {!Element} */
const appListElement =
    /** @type {!Element} */ (goog.dom.getElement('app-list'));

const knownAppsRegistry = new PermissionsChecking.KnownAppsRegistry();

/**
 * @type {goog.Promise.<!Array.<!KnownApp>>?}
 */
let lastKnownAppsPromise = null;

/**
 * @param {!goog.Promise.<!Array.<!KnownApp>>} knownAppsPromise
 * @param {!Array.<string>} appIds
 * @param {Array.<!KnownApp>?} knownApps
 * @return {boolean}
 */
function updateAppView(knownAppsPromise, appIds, knownApps) {
  if (knownAppsPromise !== lastKnownAppsPromise)
    return false;

  GSC.Logging.checkWithLogger(logger, appListElement !== null);
  goog.asserts.assert(appListElement);

  goog.dom.removeChildren(appListElement);

  for (let i = 0; i < appIds.length; i++) {
    const text =
        knownApps && knownApps[i] ? knownApps[i].name : '<' + appIds[i] + '>';
    const newElement = goog.dom.createDom('li', undefined, text);
    goog.dom.append(appListElement, newElement);
  }

  return true;
}

/**
 * @param {!Array.<string>} appListArg
 */
function onUpdateListener(appListArg) {
  const appList = goog.array.clone(appListArg);
  goog.array.sort(appList);
  goog.log.fine(
      logger,
      'Application list updated, refreshing the view. ' +
          'New list of id\'s: ' + GSC.DebugDump.dump(appList));

  const knownAppsPromise = knownAppsRegistry.tryGetByIds(appList);
  lastKnownAppsPromise = knownAppsPromise;

  knownAppsPromise.then(
      function(knownApps) {
        updateAppView(knownAppsPromise, appList, knownApps);
      },
      function(error) {
        if (!updateAppView(knownAppsPromise, appList, null))
          return;

        goog.log.warning(logger, 'Couldn\'t resolve appList: ' + error);
      });
}

GSC.ConnectorApp.Window.AppsDisplaying.initialize = function() {
  goog.log.fine(logger, 'Registering listener on connected apps update');
  // FIXME(emaxx): Do unsubscription too.
  // FIXME(emaxx): Use GSC.ObjectHelpers.extractKey to ensure that the expected
  // object is passed to the window.
  const data = GSC.InPopupMainScript.getData()['clientAppListUpdateSubscriber'];
  const clientAppListUpdateSubscriber =
      /**@type {function(function(!Array.<string>))} */ (data);
  clientAppListUpdateSubscriber(onUpdateListener);
};
});  // goog.scope
