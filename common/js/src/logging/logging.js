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
 * * Log messages are set up to be kept (probably, truncated) in a log buffer,
 *   which allows to export them later. The log buffer is either created or, if
 *   a specially-named window attribute is set (see the
 *   GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME constant), the existing log
 *   buffer is reused (which allows, in particular, to collect the logs from all
 *   App's windows in one place).
 */

goog.provide('GoogleSmartCard.Logging');

goog.require('GoogleSmartCard.LogBuffer');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.debug');
goog.require('goog.debug.Console');
goog.require('goog.log');
goog.require('goog.log.Level');
goog.require('goog.log.LogRecord');
goog.require('goog.log.Logger');
goog.require('goog.string');

goog.scope(function() {

/** @const */
var LOGGER_SCOPE = 'GoogleSmartCard';

/**
 * @type {!goog.log.Level}
 * @const
 */
var ROOT_LOGGER_LEVEL = goog.DEBUG ? goog.log.Level.FINE : goog.log.Level.INFO;

/** @const */
var LOG_BUFFER_CAPACITY = goog.DEBUG ? 10 * 1000 : 1000;

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var rootLogger = goog.asserts.assert(goog.log.getLogger(
    goog.log.ROOT_LOGGER_NAME));

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = goog.asserts.assert(goog.log.getLogger(LOGGER_SCOPE));

/** @type {boolean} */
var wasLoggingSetUp = false;

/**
 * This constant specifies the name of the special window attribute, that should
 * contain the already existing log buffer instance, if there is any.
 *
 * The window attribute with this name is checked when this script is executed.
 *
 * Providing the already existing log buffer allows to collect the logs from all
 * App's windows in one place.
 * @const
 */
GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME = 'googleSmartCardLogBuffer';

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

  logger.fine('Logging was set up with level=' + ROOT_LOGGER_LEVEL.name +
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
  var logger = goog.log.getLogger(name, opt_level);
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
  var fullName = LOGGER_SCOPE;
  if (name)
    fullName += '.' + name;
  return GSC.Logging.getLogger(fullName);
};

/**
 * Returns the logger with the specified name relative to the specified parent
 * logger.
 * @param {!goog.log.Logger} parentLogger
 * @param {string} relativeName
 * @param {!goog.log.Level=} opt_level
 * @return {!goog.log.Logger}
 */
GSC.Logging.getChildLogger = function(
    parentLogger, relativeName, opt_level) {
  return GSC.Logging.getLogger(parentLogger.getName() + '.' + relativeName);
};

/**
 * Checks if the condition evaluates to true.
 *
 * In contrast to goog.asserts.assert method, this method works in non-Debug
 * builds too.
 * @template T
 * @param {T} condition The condition to check.
 * @param {string=} opt_message Error message in case of failure.
 * @param {...*} var_args The items to substitute into the failure message.
 */
GSC.Logging.check = function(condition, opt_message, var_args) {
  if (!condition)
    GSC.Logging.fail(opt_message, Array.prototype.slice.call(arguments, 2));
};

/**
 * The same as GSC.Logging.check, but the message is prefixed with the logger
 * title.
 * @template T
 * @param {!goog.log.Logger} logger The logger which name is to be prepended
 * to the error message.
 * @param {T} condition The condition to check.
 * @param {string=} opt_message Error message in case of failure.
 * @param {...*} var_args The items to substitute into the failure message.
 */
GSC.Logging.checkWithLogger = function(
    logger, condition, opt_message, var_args) {
  if (!condition)
    GSC.Logging.failWithLogger(logger, opt_message, var_args);
};

/**
 * Fails with the specified message.
 *
 * In the debug mode, this function effectively throws an instance of
 * goog.asserts.AssertionError.
 *
 * In the release mode, this function emits severe log message and initiates the
 * App reload.
 * @param {string=} opt_message Error message in case of failure.
 * @param {...*} var_args The items to substitute into the failure message.
 */
GSC.Logging.fail = function(opt_message, var_args) {
  if (goog.DEBUG) {
    throwAssertionError(opt_message, var_args);
  } else {
    var messageAndArgs = Array.prototype.slice.call(arguments);
    rootLogger.severe.apply(rootLogger, messageAndArgs);
    rootLogger.info('Reloading the App due to the fatal error...');
    reloadApp();
  }
};

/**
 * Same as GSC.Logging.fail, but the message is prefixed with the logger title.
 * @param {!goog.log.Logger} logger The logger which name is to be prepended
 * to the error message.
 * @param {string=} opt_message Error message in case of failure.
 * @param {...*} var_args The items to substitute into the failure message.
 */
GSC.Logging.failWithLogger = function(logger, opt_message, var_args) {
  var messagePrefix = 'Failure in ' + logger.getName();
  if (goog.isDef(opt_message)) {
    var transformedMessage = messagePrefix + ': ' + opt_message;
    var args = Array.prototype.slice.call(arguments, 2);
    GSC.Logging.fail.apply(
        GSC.Logging, goog.array.concat(transformedMessage, args));
  } else {
    GSC.Logging.fail(messagePrefix);
  }
};

/**
 * Returns the log buffer instance.
 *
 * The log buffer instance was either created during this script execution, or
 * was reused from the window's attribute with name
 * GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME (see this constant docs for the
 * details).
 * @return {!GSC.LogBuffer}
 */
GSC.Logging.getLogBuffer = function() {
  return window[GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME];
};

/**
 * @param {string=} opt_message Error message in case of failure.
 * @param {...*} var_args The items to substitute into the failure message.
 */
function throwAssertionError(opt_message, var_args) {
  if (goog.asserts.ENABLE_ASSERTS) {
    var messageAndArgs = Array.prototype.slice.call(arguments);
    goog.asserts.fail.apply(goog.asserts, messageAndArgs);
  } else {
    // This branch is the last resort, and should generally never happen
    // (unless the goog.asserts.ENABLE_ASSERTS constant was changed from its
    // default value for some reason).
    throw new Error('Assertion failure');
  }
}

function reloadApp() {
  // This method works only in kiosk mode, and does nothing otherwise.
  chrome.runtime.restart();

  // This method works only in non-kiosk mode.
  chrome.runtime.reload();
}

function setupConsoleLogging() {
  var console = new goog.debug.Console;
  console.setCapturing(true);
}

function setupRootLoggerLevel() {
  rootLogger.setLevel(ROOT_LOGGER_LEVEL);
}

function setupLogBuffer() {
  /** @type {GSC.LogBuffer} */
  var logBuffer;
  if (goog.object.containsKey(
          window, GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME)) {
    logBuffer = window[GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME];
    logger.fine('Detected an existing log buffer instance, attaching it to ' +
                'the root logger');
  } else {
    var logBufferLoggerPrefix = goog.string.subs(
        '<%s:%s>', chrome.runtime.id, document.location.pathname);
    logBuffer = new GSC.LogBuffer(LOG_BUFFER_CAPACITY, logBufferLoggerPrefix);
    window[GSC.Logging.GLOBAL_LOG_BUFFER_VARIABLE_NAME] = logBuffer;
    logger.fine(
        'Created a new log buffer instance, attaching it to the root logger');
  }

  logBuffer.attachToLogger(rootLogger);
}

GSC.Logging.setupLogging();

});  // goog.scope
