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
 * @fileoverview This file contains the class used to track readers and their
 * status based on messages received from the hook setup on PC/SC code inside
 * NaCl (check readerfactory_nacl.cc for more information).
 */

goog.provide('GoogleSmartCard.ReaderTracker');

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.asserts');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.structs.Map');

goog.scope(function() {

/** @const */
var READER_TRACKER_LOGGER_TITLE = 'ReaderTracker';

/** @const */
var GSC = GoogleSmartCard;

/**
 * Structure used to store information about the reader.
 * @param {string} name
 * @param {string} status
 * @param {string=} opt_error
 * @constructor
 * @struct
 */
var ReaderInfo = function(name, status, opt_error) {
  this.name = name;
  this.status = status;
  this.error = opt_error;
};

/**
 * This class tracks readers, provides methods to retrieve the list of readers
 * and get updates on change, and listens to messages received from the hook
 * living in the NaCl space.
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {!goog.log.Logger} parentLogger
 * @constructor
 */
GSC.ReaderTracker = function(messageChannel, parentLogger) {
  /** @private */
  this.logger_ = GSC.Logging.getChildLogger(
      parentLogger, READER_TRACKER_LOGGER_TITLE);

  messageChannel.registerService(
      'reader_init_add', this.readerInitAddListener_.bind(this), true);
  messageChannel.registerService(
      'reader_finish_add', this.readerFinishAddListener_.bind(this), true);
  messageChannel.registerService(
      'reader_remove', this.readerRemoveListener_.bind(this), true);

  /**
   * @type {!goog.structs.Map.<string, !ReaderInfo>}
   * @private
   */
  this.readers_ = new goog.structs.Map;

  /**
   * @type {!Array.<!function(!Array.<!ReaderInfo>)>}
   * @private
   */
  this.onUpdateListeners_ = [];

  this.logger_.fine('Initialized');
};

/** @const */
var ReaderTracker = GSC.ReaderTracker;

/**
 * @param {!Object|string} message
 * @private
 */
ReaderTracker.prototype.readerInitAddListener_ = function(message) {
  var name = message['readerName'];
  var port = message['port'];
  var device = message['device'];

  this.logger_.info(
      'readerInitAddListener_ called for ' + name + ' (port ' + port +
      ', device ' + device + ')');

  this.readers_.set(port, new ReaderInfo(name, 'yellow'));
  this.fireOnUpdateListeners_();
};

/**
 * @param {!Object|string} message
 * @private
 */
ReaderTracker.prototype.readerFinishAddListener_ = function(message) {
  var name = message['readerName'];
  var port = message['port'];
  var device = message['device'];
  var returnCode = message['returnCode'];
  var returnCodeHex = '0x' + (returnCode >>> 0).toString(16);

  var value = new ReaderInfo(name, 'green');

  var text =
      'readerFinishAddListener_ called for ' + name + ' (port ' + port +
      ', device ' + device + ') with return code ' + returnCodeHex;

  if (returnCode === 0) {
    this.logger_.info(text);
  } else {
    this.logger_.warning(text);
    value.status = 'red';
    value.error = returnCodeHex;
  }

  this.readers_.set(port, value);
  this.fireOnUpdateListeners_();
};

/**
 * @param {!Object|string} message
 * @private
 */
ReaderTracker.prototype.readerRemoveListener_ = function(message) {
  var name = message['readerName'];
  var port = message['port'];

  this.logger_.info('readerRemoveListener_ called for ' + name +
                    ' (port ' + port + ')');

  this.readers_.remove(port);
  this.fireOnUpdateListeners_();
};

/**
 * @param {function(!Array.<!ReaderInfo>)} listener
 */
ReaderTracker.prototype.addOnUpdateListener = function(listener) {
  this.logger_.fine('Added an OnUpdateListener');
  this.onUpdateListeners_.push(listener);
};

/**
 * @param {function(!Array.<!ReaderInfo>)} listener
 */
ReaderTracker.prototype.removeOnUpdateListener = function(listener) {
  this.logger_.fine('Removed an OnUpdateListener');
  var index = this.onUpdateListeners_.indexOf(listener);
  if (index >= 0) {
    this.onUpdateListeners_.splice(index, 1);
  }
};

/**
 * @private
 */
ReaderTracker.prototype.fireOnUpdateListeners_ = function() {
  var readers = this.getReaders();
  this.logger_.fine('Firing readers updated listeners with data ' +
                    GSC.DebugDump.dump(readers));
  for (let listener of this.onUpdateListeners_) {
    listener(readers);
  }
};

/**
 * @return {!Array.<!ReaderInfo>}
 */
ReaderTracker.prototype.getReaders = function() {
  return this.readers_.getValues();
};

});  // goog.scope
