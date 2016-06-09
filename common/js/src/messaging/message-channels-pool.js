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

/** TODO @fileoverview */

goog.provide('GoogleSmartCard.MessageChannelsPool');

goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('goog.log.Logger');
goog.require('goog.structs.Map');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * TODO: class description, @param?, @extends?
 *
 * @constructor
 */
GSC.MessageChannelsPool = function() {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('MessageChannelsPool');

  /**
   * @type {goog.structs.Map.<string, !GSC.SingleMessageBasedChannel>}
   * @private
   */
  this.channels_ = new goog.structs.Map;

  this.logger.fine('Initialized successfully');
};

/** @const */
var MessageChannelsPool = GSC.MessageChannelsPool;

/** @type {function(string):GSC.SingleMessageBasedChannel} */
MessageChannelsPool.prototype.getChannel = function(extensionId) {
  return this.channels_.get(extensionId, null);
};

/** @type {function(!GSC.SingleMessageBasedChannel)} */
MessageChannelsPool.prototype.addChannel = function(messageChannel) {
  var extensionId = messageChannel.extensionId;
  this.logger.fine('Added a new channel, id = ' + extensionId);
  this.channels_.set(extensionId, messageChannel);
  messageChannel.addOnDisposeCallback(
      this.handleChannelDisposed_.bind(this, extensionId));
};

/** @private @type {function(string)} */
MessageChannelsPool.prototype.handleChannelDisposed_ = function(extensionId) {
  this.logger.fine('Disposed of channel id = ' + extensionId);
  this.channels_.remove(extensionId);
};

});  // goog.scope
