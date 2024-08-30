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
 * @fileoverview This file contains additional helper definitions on top of the
 * Google Closure Library's logging subsystem (see
 * <http://google.github.io/closure-library/api/namespace_goog_log.html>).
 *
 * Aside from providing several helper functions, this file, when executed, sets
 * up the logging subsystem parameters:
 *
 * * The logging level of the root logger is set up according to whether or not
 *   the compilation is performed in a debug mode (see
 *   <http://google.github.io/closure-library/api/namespace_goog.html#DEBUG>).
 * * Log messages that bubbled till the root logger are emitted to the
 *   JavaScript Console.
 * * Log messages are set up to be kept (probably, truncated) in a background
 *   page's log buffer, which allows to export them later.
 */

goog.provide('GoogleSmartCard.Logging');

goog.require('GoogleSmartCard.LogBuffer');
goog.require('GoogleSmartCard.Logging.CrashLoopDetection');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.debug');
goog.require('goog.debug.Console');
goog.require('goog.log');
goog.require('goog.log.Level');
goog.require('goog.log.LogRecord');
goog.require('goog.log.Logger');
goog.require('goog.object');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * @define {boolean} Whether to make every logger created via this library a
 * child of the |LOGGER_SCOPE|.

 * Overriding it to false allows to reduce the boilerplate printed in every
 * logged message; the default true value, on the other hand, allows to avoid
 * clashes in case the extension creates and manages its own Closure Library
 * loggers.
 */
GSC.Logging.USE_SCOPED_LOGGERS =
    goog.define('GoogleSmartCard.Logging.USE_SCOPED_LOGGERS', true);

/**
 * @define {boolean} Whether to trigger the extension reload in case a fatal
 * error occurs in Release mode.
 */
GSC.Logging.SELF_RELOAD_ON_FATAL_ERROR =
    goog.define('GoogleSmartCard.Logging.SELF_RELOAD_ON_FATAL_ERROR', false);

/**
 * @define {boolean} Whether to additionally send logs to the system log (see
 * the `chrome.systemLog` API).
 *
 * The system log can be viewed on ChromeOS at chrome://device-log and is also
 * included into Feedback Reports sent by users.
 */
GSC.Logging.WRITE_TO_SYSTEM_LOG =
    goog.define('GoogleSmartCard.Logging.WRITE_TO_SYSTEM_LOG', false);

/**
 * Every logger created via this library is created as a child of this logger,
 * as long as the |USE_SCOPED_LOGGERS| constant is true. Ignored when that
 * constant is false.
 */
const LOGGER_SCOPE = 'GoogleSmartCard';

/**
 * The logging level that will be applied to the root logger (and therefore
 * would be effective for all loggers unless the ones that have an explicitly
 * set level).
 */
const ROOT_LOGGER_LEVEL =
    goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO;

/**
 * The capacity of the buffer that stores the emitted log messages.
 *
 * When the number of log messages exceeds this capacity, the messages from the
 * middle will be removed (so only some first and some last messages will be
 * kept at any given moment of time).
 */
const LOG_BUFFER_CAPACITY = goog.DEBUG ? 20 * 1000 : 2000;

/**
 * Name of the port that's used for sending logs to the Service Worker or
 * Background Page from all other pages.
 */
const LOG_MESSAGES_PORT_NAME = 'logs';

/**
 * @type {!goog.log.Logger}
 */
const rootLogger =
    goog.asserts.assert(goog.log.getLogger(goog.log.ROOT_LOGGER_NAME));

/**
 * @type {!goog.log.Logger}
 */
const logger = GSC.Logging.USE_SCOPED_LOGGERS ?
    goog.asserts.assert(goog.log.getLogger(LOGGER_SCOPE)) :
    rootLogger;

/** @type {boolean} */
let wasLoggingSetUp = false;

/**
 * The log buffer that aggregates all log messages, to let them be exported if
 * the user requests so.
 * @type {!GSC.LogBuffer}
 */
const logBuffer = new GSC.LogBuffer(LOG_BUFFER_CAPACITY);

/**
 * Whether we should self-reload our App/Extension when a fatal log message.
 *
 * The default value is determined from build-time constants, and it can also be
 * controlled at runtime by calling
 * `GoogleSmartCard.disableSelfReloadOnFatalError()`.
 * @type {boolean}
 */
let selfReloadOnFatalError =
    !goog.DEBUG && GSC.Logging.SELF_RELOAD_ON_FATAL_ERROR;

goog.exportSymbol('GoogleSmartCard.disableSelfReloadOnFatalError', function() {
  selfReloadOnFatalError = false;
});

/**
 * A message that represents a log record.
 *
 * These messages are sent to the Service Worker by all other pages, so that the
 * former can collect all logs and export them accordingly.
 * @typedef {{
 *            document_location:string,
 *            level:string,
 *            message:string,
 *            logger:string,
 *            time:number,
 *            sequence_number:number,
 *          }}
 */
let LogMessage;

/**
 * Sets up logging parameters and log buffering.
 *
 * This function is called automatically when this library file is included.
 */
GSC.Logging.setupLogging = function() {
  if (wasLoggingSetUp)
    return;
  wasLoggingSetUp = true;

  if (GSC.Logging.WRITE_TO_SYSTEM_LOG) {
    setupSystemLogLogging();
  }
  setupConsoleLogging();
  setupRootLoggerLevel();

  goog.log.fine(
      logger,
      'Logging was set up with level=' + ROOT_LOGGER_LEVEL.name +
          ' and enabled logging to JS console');

  if (isLogCollectorPage()) {
    // We're running in the "collector" page (the Service Worker or, in case of
    // mv2, the Background Page). The log buffer will aggregate the current
    // page's messages as well as receive logs from other pages.
    GSC.LogBuffer.attachBufferToLogger(
        logBuffer, rootLogger, getDocumentLocation());
    setupLogReceiving();
  } else {
    // Forward our logs to the "collector" page.
    setupLogSending();
  }
};

/**
 * Returns a logger.
 * @param {string} name
 * @param {!goog.log.Level=} opt_level
 * @return {!goog.log.Logger}
 */
GSC.Logging.getLogger = function(name, opt_level) {
  const logger = goog.log.getLogger(name, opt_level);
  GSC.Logging.check(logger);
  goog.asserts.assert(logger);
  return logger;
};

/**
 * Returns a library-scoped logger.
 * @param {string} name
 * @param {!goog.log.Level=} opt_level
 * @return {!goog.log.Logger}
 */
GSC.Logging.getScopedLogger = function(name, opt_level) {
  let fullName;
  if (GSC.Logging.USE_SCOPED_LOGGERS && name)
    fullName = `${LOGGER_SCOPE}.${name}`;
  else if (GSC.Logging.USE_SCOPED_LOGGERS)
    fullName = LOGGER_SCOPE;
  else
    fullName = name;
  return GSC.Logging.getLogger(fullName, opt_level);
};

/**
 * Returns the logger with the specified name relative to the specified parent
 * logger.
 * @param {!goog.log.Logger} parentLogger
 * @param {string} relativeName
 * @param {!goog.log.Level=} opt_level
 * @return {!goog.log.Logger}
 */
GSC.Logging.getChildLogger = function(parentLogger, relativeName, opt_level) {
  return GSC.Logging.getLogger(
      parentLogger.getName() + '.' + relativeName, opt_level);
};

/**
 * Changes the logger level so that the logger is not more verbose than the
 * specified level.
 * @param {!goog.log.Logger} logger
 * @param {!goog.log.Level} boundaryLevel
 */
GSC.Logging.setLoggerVerbosityAtMost = function(logger, boundaryLevel) {
  const effectiveLevel = goog.log.getEffectiveLevel(logger);
  if (!effectiveLevel || effectiveLevel.value < boundaryLevel.value)
    goog.log.setLevel(logger, boundaryLevel);
};

/**
 * Checks if the condition evaluates to true.
 *
 * In contrast to goog.asserts.assert method, this method works in non-Debug
 * builds too.
 * @template T
 * @param {T} condition The condition to check.
 * @param {string=} opt_message Error message in case of failure.
 */
GSC.Logging.check = function(condition, opt_message) {
  if (!condition)
    GSC.Logging.fail(opt_message);
};

/**
 * The same as GSC.Logging.check, but the message is prefixed with the logger
 * title.
 * @template T
 * @param {!goog.log.Logger} logger The logger which name is to be prepended
 * to the error message.
 * @param {T} condition The condition to check.
 * @param {string=} opt_message Error message in case of failure.
 */
GSC.Logging.checkWithLogger = function(logger, condition, opt_message) {
  if (!condition)
    GSC.Logging.failWithLogger(logger, opt_message);
};

/**
 * Throws an exception and emits severe log with the specified message.
 *
 * In the release mode, this additionally asynchronously initiates the App
 * reload, unless a crash-and-reload loop is detected.
 * @param {string=} opt_message Error message in case of failure.
 */
GSC.Logging.fail = function(opt_message) {
  const message = opt_message ? opt_message : 'Failure';
  goog.log.error(rootLogger, message);
  scheduleSelfReloadIfAllowed();
  throw new Error(message);
};

/**
 * Same as GSC.Logging.fail, but the message is prefixed with the logger title.
 * @param {!goog.log.Logger} logger The logger which name is to be prepended
 * to the error message.
 * @param {string=} opt_message Error message in case of failure.
 */
GSC.Logging.failWithLogger = function(logger, opt_message) {
  const messagePrefix = 'Failure in ' + logger.getName();
  if (opt_message !== undefined) {
    const transformedMessage = messagePrefix + ': ' + opt_message;
    GSC.Logging.fail(transformedMessage);
  } else {
    GSC.Logging.fail(messagePrefix);
  }
};

/**
 * Returns the log buffer instance.
 *
 * The log buffer instance was either created during this script execution, or
 * was reused from the background page's global attribute.
 * @return {!GSC.LogBuffer}
 */
GSC.Logging.getLogBuffer = function() {
  return logBuffer;
};

/**
 * Returns whether the current page should act as a collector of logs from all
 * other pages of our extension.
 */
function isLogCollectorPage() {
  if (typeof document === 'undefined') {
    // We're running in the Service Worker.
    return true;
  }
  if (self.location.pathname === '/_generated_background_page.html') {
    // We're running in the Background Page (in a Manifest v2 Extension/App).
    return true;
  }
  return false;
}

function scheduleSelfReloadIfAllowed() {
  if (!selfReloadOnFatalError)
    return;
  GSC.Logging.CrashLoopDetection.handleImminentCrash()
      .then(function(isInCrashLoop) {
        if (isInCrashLoop) {
          goog.log.info(
              rootLogger,
              'Crash loop detected. The application is defunct, but the ' +
                  'execution state is kept in order to retain the failure ' +
                  'logs.');
          return;
        }
        goog.log.info(
            rootLogger, 'Reloading the application due to the fatal error...');
        reloadApp();
      })
      .catch(function() {
        // Don't do anything for repeated crashes within a single run.
      });
}

function reloadApp() {
  // This method works only in non-kiosk mode. Since this is a much more common
  // case and as this function doesn't generate errors in any case, this method
  // is called first.
  chrome.runtime.reload();

  // This method works only in kiosk mode.
  chrome.runtime.restart();
}

function setupSystemLogLogging() {
  // Access the Extension API via string literals, to make sure the Closure
  // Compiler won't attempt type-checking or optimizations with it.
  const systemLog = chrome['systemLog'];
  if (!systemLog) {
    // The API is unavailable (because the extension lacks the "systemLog"
    // permission or ChromeOS is too old) - bail out silently.
    return;
  }
  // Cache values that are common across all invocations of the handler below.
  const systemLogAdd = systemLog['add'];
  const documentLocation = getDocumentLocation();

  goog.log.addHandler(rootLogger, (logRecord) => {
    const formattedLogRecord = GSC.LogFormatting.formatLogRecordForSystemLog(
        documentLocation, logRecord);
    systemLogAdd({'message': formattedLogRecord});
  });
}

function setupConsoleLogging() {
  const console = new goog.debug.Console();
  const formatter = console.getFormatter();
  formatter.showAbsoluteTime = true;
  formatter.showRelativeTime = false;
  console.setCapturing(true);
}

function setupRootLoggerLevel() {
  goog.log.setLevel(rootLogger, ROOT_LOGGER_LEVEL);
}

function getDocumentLocation() {
  if (typeof document === 'undefined') {
    // We're likely inside a Service Worker.
    return self.location.href;
  }
  return document.location.href;
}

/**
 * Sets up sending every emitted log record as a message.
 */
function setupLogSending() {
  if (!chrome || !chrome.runtime || !chrome.runtime.connect) {
    // This should only happen in tests.
    return;
  }
  const port = chrome.runtime.connect({'name': LOG_MESSAGES_PORT_NAME});
  goog.log.addHandler(rootLogger, (logRecord) => {
    /** @type {!LogMessage} */
    const message = {
      'document_location': getDocumentLocation(),
      'level': logRecord.getLevel().name,
      'message': logRecord.getMessage(),
      'logger': logRecord.getLoggerName(),
      'time': logRecord.getMillis(),
      'sequence_number': logRecord.getSequenceNumber(),
    };
    port.postMessage(message);
  });
}

/**
 * Listens for and handles incoming log messages sent from other pages.
 */
function setupLogReceiving() {
  if (!chrome || !chrome.runtime || !chrome.runtime.onConnect) {
    // This should only happen in tests.
    return;
  }
  chrome.runtime.onConnect.addListener((port) => {
    if (port.name !== LOG_MESSAGES_PORT_NAME) {
      return;
    }
    port.onMessage.addListener((message) => {
      onLogMessageReceived(
          message['document_location'],
          new goog.log.LogRecord(
              goog.log.Level.getPredefinedLevel(message['level']),
              message['message'], message['logger'], message['time'],
              message['sequence_number']));
    });
  });
}

/**
 * @param {string} documentLocation
 * @param {!goog.log.LogRecord} logRecord
 */
function onLogMessageReceived(documentLocation, logRecord) {
  logBuffer.addLogRecord(
      documentLocation, logRecord.getLevel(), logRecord.getMessage(),
      logRecord.getLoggerName(), logRecord.getMillis(),
      logRecord.getSequenceNumber());
}

GSC.Logging.setupLogging();
});  // goog.scope
