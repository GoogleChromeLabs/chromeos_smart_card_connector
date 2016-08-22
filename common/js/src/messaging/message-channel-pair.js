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
 * @fileoverview TODO(emaxx): Write docs.
 */

goog.provide('GoogleSmartCard.MessageChannelPair');

goog.require('goog.async.nextTick');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * TODO(emaxx): Write docs.
 * TODO(emaxx): Implement disposal semantics.
 * @constructor
 */
GSC.MessageChannelPair = function() {
  /** @private */
  this.items_ = [
      new MessageChannelPairItem(this, 0), new MessageChannelPairItem(this, 1)];

  this.first = this.items_[0];
  this.second = this.items_[1];
};

/** @const */
var MessageChannelPair = GSC.MessageChannelPair;

/**
 * @param {number} fromItemIndex
 * @param {string} serviceName
 * @param {!Object|string} payload
 * @private
 */
MessageChannelPair.prototype.sendFrom_ = function(
    fromItemIndex, serviceName, payload) {
  var targetItem = this.items_[1 - fromItemIndex];
  goog.async.nextTick(targetItem.deliver.bind(
      targetItem, serviceName, payload));
};

/**
 * @param {!MessageChannelPair} messageChannelPair
 * @param {number} indexInPair
 * @constructor
 * @extends goog.messaging.AbstractChannel
 */
var MessageChannelPairItem = function(messageChannelPair, indexInPair) {
  MessageChannelPairItem.base(this, 'constructor');

  /** @private */
  this.messageChannelPair_ = messageChannelPair;
  /** @private */
  this.indexInPair_ = indexInPair;
};

goog.inherits(MessageChannelPairItem, goog.messaging.AbstractChannel);

/** @override */
MessageChannelPairItem.prototype.send = function(serviceName, payload) {
  this.messageChannelPair_.sendFrom_(this.indexInPair_, serviceName, payload);
};

});  // goog.scope
