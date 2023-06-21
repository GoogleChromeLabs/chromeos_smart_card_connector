/**
 * @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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
 * @fileoverview This file contains definitions that allow formatting log
 * messages.
 */

goog.provide('GoogleSmartCard.LogFormatting');

goog.require('goog.Uri');
goog.require('goog.debug.TextFormatter');
goog.require('goog.log.LogRecord');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * @type {!goog.debug.TextFormatter}
 */
const defaultFormatter = new goog.debug.TextFormatter();
defaultFormatter.showAbsoluteTime = true;
defaultFormatter.showRelativeTime = false;
defaultFormatter.showSeverityLevel = true;

/**
 * @type {!goog.debug.TextFormatter}
 */
const systemLogFormatter = new goog.debug.TextFormatter();
systemLogFormatter.showAbsoluteTime = false;
systemLogFormatter.showRelativeTime = false;
systemLogFormatter.showSeverityLevel = true;

/**
 * @param {string} documentLocation
 * @param {boolean} isCompact
 * @return {string}
 */
function getFormattedDocumentLocation(documentLocation, isCompact) {
  if (!isCompact)
    return `<${documentLocation}>`;
  let uri;
  try {
    uri = goog.Uri.parse(documentLocation);
  } catch (URIError) {
    return `<${documentLocation}>`;
  }
  if (uri.getPath() == '/_generated_background_page.html')
    return '';
  return `<${uri.getPath()}>`;
}

/**
 * @param {string} documentLocation
 * @param {!goog.log.LogRecord} logRecord
 * @param {boolean} isCompact
 * @return {!goog.log.LogRecord}
 */
function prefixLogRecord(documentLocation, logRecord, isCompact) {
  const loggerNameParts = [];
  const formattedDocumentLocation =
      getFormattedDocumentLocation(documentLocation, isCompact);
  if (formattedDocumentLocation)
    loggerNameParts.push(formattedDocumentLocation);
  if (logRecord.getLoggerName())
    loggerNameParts.push(logRecord.getLoggerName());
  const prefixedLoggerName = loggerNameParts.join('.');

  return new goog.log.LogRecord(
      logRecord.getLevel(), logRecord.getMessage(), prefixedLoggerName,
      logRecord.getMillis(), logRecord.getSequenceNumber());
}

/**
 * Returns a formatted representation of the log record suitable for logging
 * into NaCl stderr console.
 * @param {string} documentLocation
 * @param {!goog.log.LogRecord} logRecord
 * @return {string}
 */
GSC.LogFormatting.formatLogRecordForNaclStderr = function(
    documentLocation, logRecord) {
  return defaultFormatter.formatRecord(
      prefixLogRecord(documentLocation, logRecord, /*isCompact=*/ false));
};

/**
 * Returns a formatted representation of the log record suitable for the log
 * dump that can be accessed via the extension's UI.
 *
 * The representation omits the extension ID and the background page label.
 * @param {string} documentLocation
 * @param {!goog.log.LogRecord} logRecord
 * @return {string}
 */
GSC.LogFormatting.formatLogRecordForExportUi = function(
    documentLocation, logRecord) {
  return defaultFormatter.formatRecord(
      prefixLogRecord(documentLocation, logRecord, /*isCompact=*/ true));
};

/**
 * Returns a formatted representation of the log record suitable for the system
 * log (see the `chrome.systemLog` API).
 *
 * The representation omits time, the extension ID, the background page label.
 * @param {string} documentLocation
 * @param {!goog.log.LogRecord} logRecord
 * @return {string}
 */
GSC.LogFormatting.formatLogRecordForSystemLog = function(
    documentLocation, logRecord) {
  return systemLogFormatter.formatRecord(
      prefixLogRecord(documentLocation, logRecord, /*isCompact=*/ true));
};
});
