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
 * @fileoverview This file contains the pool that keeps track of all the
 * Closure-style AbstractChannel's used for messaging. A single extension can
 * have multiple channels associated with it.
 */

goog.provide('GoogleSmartCard.MessageChannelPool');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.labs.structs.Multimap');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * This class is a pool which keeps track of Closure-style AbstractChannel's.
 * @constructor
 */
GSC.MessageChannelPool = function() {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('MessageChannelPool');

  /**
   * Multimap from string extension id to !goog.messaging.AbstractChannel.
   *
   * TODO(isandrk): extensionId may be null (extension talks to itself)
   * @type {!goog.labs.structs.Multimap}
   * @private
   */
  this.channels_ = new goog.labs.structs.Multimap;

  /**
   * @type {!Array.<function(!Array.<string>)>}
   * @private
   */
  this.onUpdateListeners_ = [];

  this.logger.fine('Initialized successfully');
};

const MessageChannelPool = GSC.MessageChannelPool;

/**
 * @param {string} extensionId
 * @return {!Array.<!goog.messaging.AbstractChannel>}
 */
MessageChannelPool.prototype.getChannels = function(extensionId) {
  return /** @type {!Array.<!goog.messaging.AbstractChannel>} */ (
      this.channels_.get(extensionId));
};

/**
 * Returns the extensionId's of all the connected channels.
 * @return {!Array.<string>}
 */
MessageChannelPool.prototype.getExtensionIds = function() {
  return this.channels_.getKeys();
};

/**
 * @param {string} extensionId
 * @param {!goog.messaging.AbstractChannel} messageChannel
 */
MessageChannelPool.prototype.addChannel = function(
    extensionId, messageChannel) {
  if (this.channels_.containsEntry(extensionId, messageChannel)) {
    GSC.Logging.failWithLogger(
        this.logger, 'Tried to add a channel that was already present');
  }
  this.logger.fine('Added a new channel, extension id = ' + extensionId);
  this.channels_.add(extensionId, messageChannel);
  messageChannel.addOnDisposeCallback(
      this.handleChannelDisposed_.bind(this, extensionId, messageChannel));
  this.fireOnUpdateListeners_();
};

/**
 * @param {function(!Array.<string>)} listener
 * @param {!Object=} opt_scope
 */
MessageChannelPool.prototype.addOnUpdateListener = function(
    listener, opt_scope) {
  this.logger.fine('Added an OnUpdateListener');
  this.onUpdateListeners_.push(
      opt_scope !== undefined ? goog.bind(listener, opt_scope) : listener);
  // Fire it once immediately to update.
  this.fireOnUpdateListeners_();
};

/**
 * @private
 */
MessageChannelPool.prototype.fireOnUpdateListeners_ = function() {
  this.logger.fine('Firing channel update listeners');
  for (let listener of this.onUpdateListeners_) {
    listener(this.getExtensionIds());
  }
};

/**
 * @private
 * @param {string} extensionId
 * @param {!goog.messaging.AbstractChannel} messageChannel
 */
MessageChannelPool.prototype.handleChannelDisposed_ = function(
    extensionId, messageChannel) {
  this.logger.fine('Disposed of channel, extension id = ' + extensionId);
  if (!this.channels_.remove(extensionId, messageChannel)) {
    GSC.Logging.failWithLogger(
        this.logger, 'Tried to dispose of non-existing channel');
  }
  this.fireOnUpdateListeners_();
};
});  // goog.scope
