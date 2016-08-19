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
 * @fileoverview This file contains definitions that provide the ability to keep
 * the emitted log messages for the further exporting of them.
 */

goog.provide('GoogleSmartCard.LogBuffer');

goog.require('goog.array');
goog.require('goog.debug.TextFormatter');
goog.require('goog.log.LogRecord');
goog.require('goog.log.Logger');
goog.require('goog.structs.CircularBuffer');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * This class is the log buffer that allows to keep the log messages emitted by
 * by the loggers to which it is attached.
 *
 * The main aim is to provide the ability to export the emitted log messages at
 * any time.
 *
 * The log buffer has some fixed capacity; when it's exceeded, the kept log
 * messages start to be dropped, so that only the first and the last messages
 * are preserved.
 *
 * This class is a replacement of the goog.debug.LogBuffer class. The primary
 * difference is that the latter one, when its capacity its exceeded, keeps only
 * last of the log messages, meanwhile the very first messages may also contain
 * the crucial information.
 * @param {number} capacity The maximum number of stored log messages.
 * @param {string=} opt_loggerPrefix
 * @constructor
 */
GSC.LogBuffer = function(capacity, opt_loggerPrefix) {
  /** @private */
  this.capacity_ = capacity;
  /** @private */
  this.loggerPrefix_ = opt_loggerPrefix;

  /** @private */
  this.size_ = 0;

  /** @private */
  this.logsPrefixCapacity_ = Math.trunc(capacity / 2);
  /** @private */
  this.logsPrefix_ = [];

  /** @private */
  this.logsSuffix_ = new goog.structs.CircularBuffer(
      capacity - this.logsPrefixCapacity_);
};

/** @const */
var LogBuffer = GSC.LogBuffer;

goog.exportSymbol('GoogleSmartCard.LogBuffer', LogBuffer);

/**
 * @param {!goog.log.Logger} logger
 */
LogBuffer.prototype.attachToLogger = function(logger) {
  logger.addHandler(this.onLogMessage_.bind(this));
};

goog.exportProperty(
    LogBuffer.prototype, 'attachToLogger', LogBuffer.prototype.attachToLogger);

/**
 * The structure that is used for returning the immutable snapshot of the log
 * buffer state (see the LogBuffer.prototype.getState method).
 * @param {number} logCount
 * @param {!Array.<!goog.log.LogRecord>} logsPrefix
 * @param {number} skippedLogCount
 * @param {!Array.<!goog.log.LogRecord>} logsSuffix
 * @constructor
 */
LogBuffer.State = function(logCount, logsPrefix, skippedLogCount, logsSuffix) {
  this.logCount = logCount;
  this.logsPrefix = logsPrefix;
  this.skippedLogCount = skippedLogCount;
  this.logsSuffix = logsSuffix;
};

goog.exportProperty(LogBuffer, 'State', LogBuffer.State);

/**
 * Returns the textual dump of the internal state.
 *
 * The dump contains the formatted log messages, together with the information
 * about the dropped log messages (if there are any).
 * @return {string}
 */
LogBuffer.State.prototype.dumpToText = function() {
  var textFormatter = new goog.debug.TextFormatter;
  var result = '';

  goog.array.forEach(this.logsPrefix, function(logRecord) {
    result += textFormatter.formatRecord(logRecord);
  });

  if (this.skippedLogCount)
    result += '\n... skipped ' + this.skippedLogCount + ' messages ...\n\n';

  goog.array.forEach(this.logsSuffix, function(logRecord) {
    result += textFormatter.formatRecord(logRecord);
  });

  return result;
};

goog.exportProperty(
    LogBuffer.State.prototype,
    'dumpToText',
    LogBuffer.State.prototype.dumpToText);

/**
 * Returns the immutable snapshot of the log buffer state.
 *
 * The reason for this way of returning the internal state against the usual
 * accessor methods is that it's quite possible that new log messages may be
 * emitted while the client is still iterating over the kept state, which would
 * make writing a robust client code difficult.
 * @return {!LogBuffer.State}
 */
LogBuffer.prototype.getState = function() {
  return new LogBuffer.State(
      this.size_,
      goog.array.clone(this.logsPrefix_),
      this.size_ - this.logsPrefix_.length - this.logsSuffix_.getCount(),
      this.logsSuffix_.getValues());
};

goog.exportProperty(
    LogBuffer.prototype, 'getState', LogBuffer.prototype.getState);

/**
 * @param {!goog.log.LogRecord} logRecord
 * @private
 */
LogBuffer.prototype.onLogMessage_ = function(logRecord) {
  this.addLogRecord_(logRecord);
};

/**
 * @param {!goog.log.LogRecord} logRecord
 * @private
 */
LogBuffer.prototype.addLogRecord_ = function(logRecord) {
  var loggerNameParts = [];
  if (this.loggerPrefix_)
    loggerNameParts.push(this.loggerPrefix_);
  if (logRecord.getLoggerName())
    loggerNameParts.push(logRecord.getLoggerName());
  var prefixedLoggerName = loggerNameParts.join('.');
  var prefixedLogRecord = new goog.log.LogRecord(
      logRecord.getLevel(),
      logRecord.getMessage(),
      prefixedLoggerName,
      logRecord.getMillis(),
      logRecord.getSequenceNumber());

  if (this.logsPrefix_.length < this.logsPrefixCapacity_)
    this.logsPrefix_.push(prefixedLogRecord);
  else
    this.logsSuffix_.add(prefixedLogRecord);
  ++this.size_;
};

});  // goog.scope
