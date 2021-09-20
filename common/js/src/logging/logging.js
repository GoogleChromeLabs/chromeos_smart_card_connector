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
 * This constant specifies the name of the special window attribute in which our
 * log buffer is stored. This is used so that popup windows and other pages can
 * access the background page's log buffer and therefore use a centralized place
 * for aggregating logs.
 */
const GLOBAL_LOG_BUFFER_VARIABLE_NAME = 'googleSmartCard_logBuffer';

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
 * the user requests so. This variable is initialized to a new `LogBuffer`
 * instance, but if we're running outside the background page this variable is
 * later reassigned to the background page's log buffer.
 * @type {!GSC.LogBuffer}
 */
let logBuffer = new GSC.LogBuffer(LOG_BUFFER_CAPACITY);

/**
 * Sets up logging parameters and log buffering.
 *
 * This function is called automatically when this library file is included.
 */
GSC.Logging.setupLogging = function() {
  if (wasLoggingSetUp)
    return;
  wasLoggingSetUp = true;

  setupConsoleLogging();
  setupRootLoggerLevel();

  goog.log.fine(
      logger,
      'Logging was set up with level=' + ROOT_LOGGER_LEVEL.name +
          ' and enabled logging to JS console');

  setupLogBuffer();
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
  return GSC.Logging.getLogger(parentLogger.getName() + '.' + relativeName);
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
  scheduleAppReloadIfAllowed();
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

function scheduleAppReloadIfAllowed() {
  if (goog.DEBUG || !GSC.Logging.SELF_RELOAD_ON_FATAL_ERROR)
    return;
  GSC.Logging.CrashLoopDetection.handleImminentCrash()
      .then(function(isInCrashLoop) {
        if (isInCrashLoop) {
          goog.log.info(
              rootLogger,
              'Crash loop detected. The application is defunct, but the ' +
                  'execution state is kept in order to retain the failure logs.');
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

function setupConsoleLogging() {
  const console = new goog.debug.Console;
  const formatter = console.getFormatter();
  formatter.showAbsoluteTime = true;
  formatter.showRelativeTime = false;
  console.setCapturing(true);
}

function setupRootLoggerLevel() {
  goog.log.setLevel(rootLogger, ROOT_LOGGER_LEVEL);
}

function setupLogBuffer() {
  GSC.LogBuffer.attachBufferToLogger(
      logBuffer, rootLogger, document.location.href);

  if (!chrome || !chrome.runtime || !chrome.runtime.getBackgroundPage) {
    // We don't know whether we're running inside the background page and
    // the API for talking to it is unavailable - therefore no action needed,
    // i.e., our page will continue using our log buffer. This should only
    // happen in tests or if this code is running outside an app/extension.
    return;
  }

  // Expose our log buffer in the global window properties. Pages other than the
  // background will use it to access the background page's log buffer - see the
  // code directly below.
  goog.global[GLOBAL_LOG_BUFFER_VARIABLE_NAME] = logBuffer;

  chrome.runtime.getBackgroundPage(function(backgroundPage) {
    GSC.Logging.check(backgroundPage);
    goog.asserts.assert(backgroundPage);

    if (backgroundPage === window) {
      // We're running inside the background page - no action needed.
      return;
    }

    // We've discovered we're running outside the background page - so need to
    // switch to using the background page's log buffer in order to keep all
    // logs aggregated and available in a single place.
    // First, obtain a reference to the background page's log buffer.
    const backgroundLogBuffer =
        /** @type {GSC.LogBuffer} */ (
            backgroundPage[GLOBAL_LOG_BUFFER_VARIABLE_NAME]);
    GSC.Logging.check(backgroundLogBuffer);
    goog.asserts.assert(backgroundLogBuffer);
    // Copy the logs we've accumulated in the current page into the background
    // page's log buffer.
    logBuffer.copyToOtherBuffer(backgroundLogBuffer);
    // From now, start using the background page's buffer for collecting data
    // from our page's loggers. Dispose of our log buffer to avoid storing new
    // logs twice.
    GSC.LogBuffer.attachBufferToLogger(
        backgroundLogBuffer, rootLogger, document.location.href);
    logBuffer.dispose();
    // Switch our reference to the background page's log buffer.
    logBuffer = backgroundLogBuffer;
    // The global reference is not needed if we're not the background page.
    delete goog.global[GLOBAL_LOG_BUFFER_VARIABLE_NAME];
  });
}

GSC.Logging.setupLogging();
});  // goog.scope
