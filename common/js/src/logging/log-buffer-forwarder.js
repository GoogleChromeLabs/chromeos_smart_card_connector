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
 * @fileoverview This file contains definitions that allow forwarding contents
 * of a log buffer to some recipient.
 */

goog.provide('GoogleSmartCard.LogBufferForwarder');

goog.require('GoogleSmartCard.LogBuffer');
goog.require('GoogleSmartCard.LogFormatting');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.log.LogRecord');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.structs.CircularBuffer');

goog.scope(function() {

const GSC = GoogleSmartCard;

const POSTPONING_BUFFER_CAPACITY = 100;

/**
 * This class observes log messages that are put into the given log buffer and
 * forwards them to the message channel specified via startForwarding(). The
 * message format is |{formatted_log_message: string}|.
 *
 * Log messages observed before the call to startForwarding() are accumulated
 * and forwarded as soon as it's called.
 *
 * NOTE: It's the responsibility of the caller to make sure that the specified
 * message channel doesn't emit logs when sending a message or that they are
 * ignored - failing to guarantee that can result in infinite recursion. (This
 * class provides some heuristical protection against synchronous failures, but
 * it won't help when the problematic logs are emitted asynchronously.)
 *
 * NOTE 2: It's expected that startForwarding() is called "soon", because this
 * class has only limited capacity for storing messages before that.
 */
GSC.LogBufferForwarder = class {
  /**
   * @param {!GSC.LogBuffer} logBuffer
   * @param {string} messageChannelServiceName
   */
  constructor(logBuffer, messageChannelServiceName) {
    /** @type {string} @private @const */
    this.messageChannelServiceName_ = messageChannelServiceName;
    /** @type {!Set} @private @const */
    this.ignoredLoggerNames_ = new Set();
    /**
     * @type {?goog.messaging.AbstractChannel}
     * @private
     */
    this.messageChannel_ = null;
    /**
     * @type {boolean}
     * @private
     */
    this.logCapturingEnabled_ = true;
    /** @type {!goog.structs.CircularBuffer<string>} @private @const */
    this.postponedLogRecords_ = new goog.structs.CircularBuffer();

    logBuffer.addObserver(this.onLogRecordObserved_.bind(this));
  }

  /**
   * Sets up forwarding of the log records to the specified message channel.
   *
   * Also immediately sends the log records that have been accumulated so far.
   * @param {!goog.messaging.AbstractChannel} messageChannel
   */
  startForwarding(messageChannel) {
    this.messageChannel_ = messageChannel;

    for (const logRecord of this.postponedLogRecords_.getValues())
      this.sendLogRecord_(logRecord);
    this.postponedLogRecords_.clear();
  }

  /**
   * Adds a logger whose messages have to be ignored.
   *
   * This only affects the messages collected after the call. This does NOT
   * affect messages from children of the specified logger.
   * @param {string} loggerName
   */
  ignoreLogger(loggerName) {
    this.ignoredLoggerNames_.add(loggerName);
  }

  /**
   * @param {string} documentLocation
   * @param {!goog.log.LogRecord} logRecord
   * @private
   */
  onLogRecordObserved_(documentLocation, logRecord) {
    if (!this.logCapturingEnabled_ ||
        this.ignoredLoggerNames_.has(logRecord.getLoggerName())) {
      return;
    }
    const formattedLogRecord =
        GSC.LogFormatting.formatLogRecord(documentLocation, logRecord);
    if (!this.messageChannel_) {
      this.postponedLogRecords_.add(formattedLogRecord);
      return;
    }
    this.sendLogRecord_(formattedLogRecord);
  }

  /**
   * @param {string} formattedLogRecord
   * @private
   */
  sendLogRecord_(formattedLogRecord) {
    // Ignore log messages emitted while sending the log, to minimize the risk
    // of infinite recursion if the message channel emits a non-ignored message.
    this.logCapturingEnabled_ = false;

    GSC.Logging.check(this.messageChannel_);
    const message = {'formatted_log_message': formattedLogRecord};
    this.messageChannel_.send(this.messageChannelServiceName_, message);
    GSC.Logging.check(!this.logCapturingEnabled_);

    this.logCapturingEnabled_ = true;
  }
};

goog.exportSymbol('GoogleSmartCard.LogBufferForwarder', GSC.LogBufferForwarder);
});
