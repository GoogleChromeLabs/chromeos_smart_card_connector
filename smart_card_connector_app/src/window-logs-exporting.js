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
 * @fileoverview This script implements logs exporting in the Smart Card
 * Connector App window.
 */

goog.provide('GoogleSmartCard.ConnectorApp.Window.LogsExporting');

goog.require('GoogleSmartCard.Clipboard');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.Packaging');
goog.require('goog.Timer');
goog.require('goog.dom');
goog.require('goog.events');
goog.require('goog.events.BrowserEvent');
goog.require('goog.events.EventType');
goog.require('goog.log');
goog.require('goog.log.Logger');

goog.scope(function() {

const EXPORT_LOGS_EXPORTED_TIMEOUT_MILLISECONDS = 5000;

const GSC = GoogleSmartCard;

const EXPORT_LOGS_ELEMENT_TEXT_ID = 'exportLogs';
const EXPORT_LOGS_ELEMENT_EXPORTING_TEXT_ID = 'exportLogsExporting';
const EXPORT_LOGS_ELEMENT_EXPORTED_TEXT_ID = 'exportLogsExported';

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('ConnectorApp.MainWindow');

/** @type {!Element} */
const exportLogsElement =
    /** @type {!Element} */ (goog.dom.getElement('export-logs'));

let isInProgress = false;

/**
 * @param {!goog.events.BrowserEvent} e
 */
function exportLogsClickListener(e) {
  e.preventDefault();

  if (isInProgress) {
    // Don't allow export run concurrently.
    return;
  }
  isInProgress = true;

  showInProgressNotification();
  // Delay the actual collection by a fraction of a second to let the UI render
  // the notification.
  goog.Timer.callOnce(async () => {
    try {
      await exportLogs();
    } finally {
      isInProgress = false;
      hideInProgressNotification();
    }
  });
}

async function exportLogs() {
  const logs = await GSC.Logging.getLogsForExport();

  goog.log.fine(
      logger,
      `Prepared a (possibly truncated) dump of log messages, the size is ${
          logs.length} characters`);

  if (GSC.Packaging.MODE === GSC.Packaging.Mode.EXTENSION) {
    // Download the logs as a blob file.
    const blobUrl = URL.createObjectURL(
        new Blob([logs], {'type': 'application/octet-binary'}));
    await chrome.downloads.download({
      'filename': 'smart-card-connector-log.txt',
      'url': blobUrl,
    });
  } else {
    // Copy the logs into the clipboard (the downloads-based solution is
    // infeasible for Chrome Apps).
    const copyingSuccess = GSC.Clipboard.copyToClipboard(logs);
    if (!copyingSuccess) {
      return;
    }
    // Display the hint to the user.
    exportLogsElement.textContent =
        chrome.i18n.getMessage(EXPORT_LOGS_ELEMENT_EXPORTED_TEXT_ID);
    await new Promise((resolve, reject) => {
      setTimeout(resolve, EXPORT_LOGS_EXPORTED_TIMEOUT_MILLISECONDS);
    });
  }
}

function showInProgressNotification() {
  exportLogsElement.textContent =
      chrome.i18n.getMessage(EXPORT_LOGS_ELEMENT_EXPORTING_TEXT_ID);
}

function hideInProgressNotification() {
  exportLogsElement.textContent =
      chrome.i18n.getMessage(EXPORT_LOGS_ELEMENT_TEXT_ID);
}

GSC.ConnectorApp.Window.LogsExporting.initialize = function() {
  goog.events.listen(
      exportLogsElement, goog.events.EventType.CLICK, exportLogsClickListener);
};
});  // goog.scope
