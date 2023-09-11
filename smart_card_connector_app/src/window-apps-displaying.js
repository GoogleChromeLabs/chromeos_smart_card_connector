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
goog.require('GoogleSmartCard.MessagingOrigin');
goog.require('GoogleSmartCard.ObjectHelpers');
goog.require('GoogleSmartCard.Packaging');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientInfo');
goog.require('GoogleSmartCard.PcscLiteServer.TrustedClientsRegistryImpl');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.log');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;
const TrustedClientInfo = GSC.PcscLiteServer.TrustedClientInfo;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

/** @type {!Element} */
const appListElement =
    /** @type {!Element} */ (goog.dom.getElement('app-list'));

const trustedClientsRegistry =
    new GSC.PcscLiteServer.TrustedClientsRegistryImpl();

/**
 * @type {goog.Promise.<!Array.<!TrustedClientInfo>>?}
 */
let lastTrustedClientInfosPromise = null;

/**
 * @param {!goog.Promise.<!Array.<!TrustedClientInfo>>}
 *     trustedClientInfosPromise
 * @param {!Array.<string>} appIds
 * @param {Array.<!TrustedClientInfo>?} trustedClientInfos
 * @return {boolean}
 */
function updateAppView(trustedClientInfosPromise, appIds, trustedClientInfos) {
  if (trustedClientInfosPromise !== lastTrustedClientInfosPromise)
    return false;

  GSC.Logging.checkWithLogger(logger, appListElement !== null);
  goog.asserts.assert(appListElement);

  goog.dom.removeChildren(appListElement);

  for (let i = 0; i < appIds.length; i++) {
    const text = trustedClientInfos && trustedClientInfos[i] ?
        trustedClientInfos[i].name :
        '<' + appIds[i] + '>';
    const newElement = goog.dom.createDom('li', undefined, text);
    goog.dom.append(appListElement, newElement);
  }

  return true;
}

/**
 * @param {!Array.<string>} clientOriginList
 */
function onUpdateListener(clientOriginList) {
  const sortedClientOriginList = goog.array.clone(clientOriginList);
  goog.array.sort(sortedClientOriginList);
  goog.log.fine(
      logger,
      'Application list updated, refreshing the view. ' +
          'New list of origins: ' +
          GSC.DebugDump.debugDumpFull(sortedClientOriginList));

  const trustedClientInfosPromise =
      trustedClientsRegistry.tryGetByOrigins(sortedClientOriginList);
  lastTrustedClientInfosPromise = trustedClientInfosPromise;

  trustedClientInfosPromise.then(
      function(trustedClientInfos) {
        updateAppView(
            trustedClientInfosPromise, sortedClientOriginList,
            trustedClientInfos);
      },
      function(error) {
        if (!updateAppView(
                trustedClientInfosPromise, sortedClientOriginList, null))
          return;

        goog.log.warning(logger, 'Couldn\'t resolve client origins: ' + error);
      });
}

/**
 * @param {!Window=} backgroundPage
 */
function initializeWithBackgroundPage(backgroundPage) {
  GSC.Logging.checkWithLogger(logger, backgroundPage);
  goog.asserts.assert(backgroundPage);

  goog.log.fine(logger, 'Registering listener on connected clients update');

  /**
   * Points to the "addOnUpdateListener" method of the MessageChannelPool
   * instance that is owned by the background page.
   */
  const appListSubscriber =
      /** @type {function(function(!Array.<string>))} */
      (GSC.ObjectHelpers.extractKey(
          backgroundPage, 'googleSmartCard_clientAppListUpdateSubscriber'));
  // Start tracking the current list of apps.
  appListSubscriber(onUpdateListener);

  /**
   * Points to the "removeOnUpdateListener" method of the MessageChannelPool
   * instance that is owned by the background page.
   */
  const appListUnsubscriber =
      /** @type {function(function(!Array.<string>))} */
      (GSC.ObjectHelpers.extractKey(
          backgroundPage, 'googleSmartCard_clientAppListUpdateUnsubscriber'));

  // Stop tracking the current list of apps when our window gets closed.
  if (GSC.Packaging.MODE === GSC.Packaging.Mode.APP) {
    chrome.app.window.current().onClosed.addListener(function() {
      appListUnsubscriber(onUpdateListener);
    });
  } else if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    window.addEventListener('unload', function() {
      appListUnsubscriber(onUpdateListener);
    });
  }
}

GSC.ConnectorApp.Window.AppsDisplaying.initialize = function() {
  chrome.runtime.getBackgroundPage(initializeWithBackgroundPage);
};
});  // goog.scope
