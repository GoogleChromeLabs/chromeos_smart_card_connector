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
 * @fileoverview This file contains definitions that provide the ability to keep
 * the emitted log messages for the further exporting of them.
 */

goog.provide('GoogleSmartCard.LogBuffer');

goog.require('GoogleSmartCard.LogFormatting');
goog.require('goog.Disposable');
goog.require('goog.array');
goog.require('goog.iter');
goog.require('goog.log');
goog.require('goog.log.LogRecord');
goog.require('goog.log.Logger');
goog.require('goog.structs.CircularBuffer');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * The structure that is used for returning the immutable snapshot of the log
 * buffer state (see the LogBuffer.prototype.getState method).
 * @unrestricted
 */
class State {
  /**
   * @param {number} logCount
   * @param {!Array.<string>} formattedLogsPrefix
   * @param {number} skippedLogCount
   * @param {!Array.<string>} formattedLogsSuffix
   */
  constructor(
      logCount, formattedLogsPrefix, skippedLogCount, formattedLogsSuffix) {
    this['logCount'] = logCount;
    this['formattedLogsPrefix'] = formattedLogsPrefix;
    this['skippedLogCount'] = skippedLogCount;
    this['formattedLogsSuffix'] = formattedLogsSuffix;
  }
}

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
 * @unrestricted
 */
GSC.LogBuffer = class extends goog.Disposable {
  /**
   * @param {number} capacity The maximum number of stored log messages.
   */
  constructor(capacity) {
    super();

    /** @private @const */
    this.capacity_ = capacity;

    /** @private */
    this.size_ = 0;

    /** @private @const */
    this.logsPrefixCapacity_ = Math.trunc(capacity / 2);
    /** @type {!Array.<string>} @private @const */
    this.formattedLogsPrefix_ = [];
    /**
     * @type {!Array.<function(string, !goog.log.LogRecord)>}
     * @private
     */
    this.observers_ = [];

    /** @type {!goog.structs.CircularBuffer.<string>} @private @const */
    this.formattedLogsSuffix_ =
        new goog.structs.CircularBuffer(capacity - this.logsPrefixCapacity_);
  }

  /** @override */
  disposeInternal() {
    this.formattedLogsPrefix_.length = 0;
    this.observers_.length = 0;
    this.formattedLogsSuffix_.clear();
    super.disposeInternal();
  }

  /**
   * @return {number}
   */
  getCapacity() {
    return this.capacity_;
  }

  /**
   * Returns the immutable snapshot of the log buffer state.
   *
   * The reason for this way of returning the internal state against the usual
   * accessor methods is that it's quite possible that new log messages will be
   * emitted while the client is still iterating over the kept state, which
   * would make writing a robust client code difficult.
   * @return {!State}
   */
  getState() {
    return new State(
        this.size_, goog.array.clone(this.formattedLogsPrefix_),
        this.size_ - this.formattedLogsPrefix_.length -
            this.formattedLogsSuffix_.getCount(),
        this.formattedLogsSuffix_.getValues());
  }

  /**
   * Adds an observer for captured log records.
   * @param {function(string, !goog.log.LogRecord)} observer
   */
  addObserver(observer) {
    this.observers_.push(observer);
  }

  /**
   * @param {function(string, !goog.log.LogRecord)} observer
   */
  removeObserver(observer) {
    this.observers_ = this.observers_.filter((value) => {
      return value !== observer;
    });
  }

  /**
   * Copies all log records that we've aggregated to the specified log buffer.
   * @param {!GSC.LogBuffer} otherLogBuffer
   */
  copyToOtherBuffer(otherLogBuffer) {
    if (this.isDisposed())
      return;
    // Take a snapshot before copying, to protect against new items being
    // appended while we're iterating over old ones.
    const state = this.getState();

    // Note: accessing the member function by indexing the properties, in order
    // to avoid issues due to the code minification. When `this` and
    // `otherLogBuffer` are coming from two different pages, we want to run the
    // function from the `otherLogBuffer`s page; failing to do so may cause
    // subtle errors, as property names in two pages might be shortened in two
    // different ways by the compiler.
    const logCopier =
        otherLogBuffer['addFormattedLogRecord_'].bind(otherLogBuffer);
    for (const formattedLogRecord of state['formattedLogsPrefix'])
      logCopier(formattedLogRecord);
    for (const formattedLogRecord of state['formattedLogsSuffix'])
      logCopier(formattedLogRecord);
  }

  /**
   * @param {string} documentLocation
   * @param {?goog.log.Level} logLevel
   * @param {string} logMsg
   * @param {string} loggerName
   * @param {number=} logTime
   * @param {number=} logSequenceNumber
   */
  addLogRecord(
      documentLocation, logLevel, logMsg, loggerName, logTime,
      logSequenceNumber) {
    if (this.isDisposed())
      return;

    const logRecord = new goog.log.LogRecord(
        logLevel, logMsg, loggerName, logTime, logSequenceNumber);
    for (const observer of this.observers_)
      observer(documentLocation, logRecord);

    const formattedLogRecord = GSC.LogFormatting.formatLogRecordForExportUi(
        documentLocation, logRecord);
    this.addFormattedLogRecord_(formattedLogRecord);
  }

  /**
   * @param {string} formattedLogRecord
   * @private
   */
  addFormattedLogRecord_(formattedLogRecord) {
    if (this.isDisposed())
      return;

    if (this.formattedLogsPrefix_.length < this.logsPrefixCapacity_)
      this.formattedLogsPrefix_.push(formattedLogRecord);
    else
      this.formattedLogsSuffix_.add(formattedLogRecord);
    ++this.size_;
  }
};

/**
 * Attaches the given log buffer to the given logger, making the buffer collect
 * all logs sent to the logger. Additionally, all collected logs are attributed
 * with the specified document location, which simplifies interpreting them.
 * @param {!GSC.LogBuffer} logBuffer
 * @param {!goog.log.Logger} logger
 * @param {string} documentLocation
 */
GSC.LogBuffer.attachBufferToLogger = function(
    logBuffer, logger, documentLocation) {
  goog.log.addHandler(logger, (logRecord) => {
    logBuffer.addLogRecord(
        documentLocation, logRecord.getLevel(), logRecord.getMessage(),
        logRecord.getLoggerName(), logRecord.getMillis(),
        logRecord.getSequenceNumber());
  });
};

// Export symbols.
GSC.LogBuffer.State = State;
goog.exportProperty(GSC.LogBuffer, 'State', GSC.LogBuffer.State);
goog.exportSymbol('GoogleSmartCard.LogBuffer', GSC.LogBuffer);
goog.exportProperty(
    GSC.LogBuffer, 'attachBufferToLogger', GSC.LogBuffer.attachBufferToLogger);
goog.exportProperty(
    GSC.LogBuffer.prototype, 'getCapacity',
    GSC.LogBuffer.prototype.getCapacity);
goog.exportProperty(
    GSC.LogBuffer.prototype, 'getState', GSC.LogBuffer.prototype.getState);
goog.exportProperty(
    GSC.LogBuffer.prototype, 'addLogRecord',
    GSC.LogBuffer.prototype.addLogRecord);
goog.exportProperty(
    GSC.LogBuffer.prototype, 'addFormattedLogRecord_',
    GSC.LogBuffer.prototype.addFormattedLogRecord_);
});  // goog.scope
