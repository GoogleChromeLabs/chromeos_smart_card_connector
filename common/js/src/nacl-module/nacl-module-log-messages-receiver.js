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
 * @fileoverview This file contains helper definitions that can be used to
 * receive and handle the log messages from the message channel to a NaCl
 * module.
 */

goog.provide('GoogleSmartCard.NaclModuleLogMessagesReceiver');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.asserts');
goog.require('goog.log');
goog.require('goog.log.Level');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var SERVICE_NAME = 'log_message';

/** @const */
var LOG_LEVEL_MESSAGE_KEY = 'log_level';
/** @const */
var TEXT_MESSAGE_KEY = 'text';

/**
 * This class receives and handles the log messages from the message channel to
 * a NaCl module.
 *
 * The NaCl module sends the emitted log messages as messages of some special
 * structure. This class handles these messages, extracts their fields and
 * transforms them into emitting log messages into the given logger.
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {!goog.log.Logger} logger
 * @constructor
 */
GSC.NaclModuleLogMessagesReceiver = function(messageChannel, logger) {
  messageChannel.registerService(
      SERVICE_NAME, this.onMessageReceived_.bind(this), true);

  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = logger;
};

/** @const */
var NaclModuleLogMessagesReceiver = GSC.NaclModuleLogMessagesReceiver;

/**
 * @param {string|!Object} messageData
 * @private
 */
NaclModuleLogMessagesReceiver.prototype.onMessageReceived_ = function(
    messageData) {
  GSC.Logging.checkWithLogger(this.logger, goog.isObject(messageData));
  goog.asserts.assertObject(messageData);

  this.logger.log(
      this.extractLogMessageLevel_(messageData),
      this.extractLogMessageText_(messageData));
};

/**
 * @param {!Object} messageData
 * @return {!goog.log.Level}
 * @private
 */
NaclModuleLogMessagesReceiver.prototype.extractLogMessageLevel_ = function(
    messageData) {
  GSC.Logging.checkWithLogger(
      this.logger, goog.object.containsKey(messageData, LOG_LEVEL_MESSAGE_KEY));
  var value = messageData[LOG_LEVEL_MESSAGE_KEY];

  GSC.Logging.checkWithLogger(this.logger, typeof value === 'string');
  goog.asserts.assertString(value);

  var result = goog.log.Level.getPredefinedLevel(value);
  GSC.Logging.checkWithLogger(
      this.logger,
      result,
      'Unknown log level specified in the received message: "' + value + '"');
  goog.asserts.assert(result);

  return result;
};

/**
 * @param {!Object} messageData
 * @return {string}
 * @private
 */
NaclModuleLogMessagesReceiver.prototype.extractLogMessageText_ = function(
    messageData) {
  GSC.Logging.checkWithLogger(
      this.logger, goog.object.containsKey(messageData, TEXT_MESSAGE_KEY));
  var value = messageData[TEXT_MESSAGE_KEY];

  GSC.Logging.checkWithLogger(this.logger, typeof value === 'string');
  goog.asserts.assertString(value);

  return value;
};

});  // goog.scope
