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
goog.require('goog.json.NativeJsonProcessor');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.object');
goog.require('goog.structs.Map');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

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
  this.logger_ = parentLogger;

  // TODO: do we need "unregisterService"?
  messageChannel.registerService(
      'reader_add', this.readerAddedListener_.bind(this));
  messageChannel.registerService(
      'reader_remove', this.readerRemovedListener_.bind(this));

  /**
   * @type {goog.structs.Map}
   * @private
   */
  this.readers_ = new goog.structs.Map;

  /**
   * @type {!Array.<!function()>}
   * @private
   */
  this.onUpdateListeners_ = [];

  this.logger_.fine('Initialized');
};

/** @const */
var ReaderTracker = GSC.ReaderTracker;

/**
 * TODO: I need to figure out the param because the closure compiler complains
 *       about mismatched types, will do it at a later time.
 * param {Object|string} message
 * @private
 */
ReaderTracker.prototype.readerAddedListener_ = function(message) {
  this.logger_.warning('readerAddedListener_ ' + message);

  // TODO: needs a promise? (check json.js)
  var processor = new goog.json.NativeJsonProcessor;
  var json = /** @type{Object} */ (processor.parse(message));

  var name = json['readerName'];
  var port = json['port'];
  var device = json['device'];
  var returnCode = json['returnCode'];

  var value = {'name': name};

  if (returnCode === undefined) {
    value.color = 'yellow';
  } else if (returnCode === 0) {
    value.color = 'green';
  } else {
    value.color = 'red';
    // TODO: Send an actual error message, and not just the error id.
    value.error = returnCode;
  }

  this.readers_.set(port, value);
  this.fireOnUpdateListeners_();
};

/**
 * param {Object|string} message
 * @private
 */
ReaderTracker.prototype.readerRemovedListener_ = function(message) {
  this.logger_.warning('readerRemovedListener_ ' + message);

  var processor = new goog.json.NativeJsonProcessor;
  var json = /** @type{Object} */ (processor.parse(message));

  var name = json['readerName'];
  var port = json['port'];

  this.readers_.remove(port);
  this.fireOnUpdateListeners_();
};

/**
 * param {function()} listener
 */
ReaderTracker.prototype.addOnUpdateListener = function(listener) {
  this.logger_.fine('Added an OnUpdateListener');
  this.onUpdateListeners_.push(listener);
};

/**
 * @private
 */
ReaderTracker.prototype.fireOnUpdateListeners_ = function() {
  this.logger_.fine('Firing readers updated listeners');
  for (let listener of this.onUpdateListeners_) {
    listener();
  }
};

ReaderTracker.prototype.getReaders = function() {
  return this.readers_.getValues();
};

});  // goog.scope
