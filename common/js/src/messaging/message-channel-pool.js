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
 * @fileoverview This file containts the pool that keeps track of all the
 * Closure-style AbstractChannel's used for messaging
 * (PortMessageChannel & SingleMesageBasedChannel).
 */

goog.provide('GoogleSmartCard.MessageChannelPool');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.structs.Map');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

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
   * TODO(isandrk): extensionId may be null (extension talks to itself)
   * TODO(isandrk): support storing both SingleMesageBasedChannel and
   *                PortMessageChannel in the map ("multimap")
   * @type {!goog.structs.Map.<string, !goog.messaging.AbstractChannel>}
   * @private
   */
  this.channels_ = new goog.structs.Map;

  this.logger.fine('Initialized successfully');
};

/** @const */
var MessageChannelPool = GSC.MessageChannelPool;

/**
 * @param {string} extensionId
 * @return {goog.messaging.AbstractChannel}
 */
MessageChannelPool.prototype.getChannel = function(extensionId) {
  return this.channels_.get(extensionId, null);
};

/**
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @param {string} extensionId
 */
MessageChannelPool.prototype.addChannel = function(
    messageChannel, extensionId) {
  if (this.getChannel(extensionId)) {
    GSC.Logging.failWithLogger(
        this.logger, 'Tried to add a channel that was already present');
  }
  this.logger.fine('Added a new channel, extension id = ' + extensionId);
  this.channels_.set(extensionId, messageChannel);
  messageChannel.addOnDisposeCallback(
      this.handleChannelDisposed_.bind(this, extensionId));
};

/**
 * @private
 * @param {string} extensionId
 */
MessageChannelPool.prototype.handleChannelDisposed_ = function(extensionId) {
  this.logger.fine('Disposed of channel, extension id = ' + extensionId);
  if (!this.channels_.remove(extensionId)) {
    GSC.Logging.failWithLogger(
        this.logger, 'Tried to dispose of non-existing channel');
  }
};

});  // goog.scope
