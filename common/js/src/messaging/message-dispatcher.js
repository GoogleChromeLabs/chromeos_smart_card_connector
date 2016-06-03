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

goog.provide('GoogleSmartCard.MessageDispatcher');

goog.require('GoogleSmartCard.OneTimeMessageChannel');
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
GSC.MessageDispatcher = function() {
  //MessageDispatcher.base(this, 'constructor');

  /** TODO
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('OneTimeMessageDispatcher');

  /**
   * @type {goog.structs.Map.<string, !GSC.OneTimeMessageChannel>}
   * @private
   */
  this.channels_ = new goog.structs.Map;

  this.logger.fine('Initialized successfully');
};

/** @const */
var MessageDispatcher = GSC.MessageDispatcher;

/** @type {function(string):GSC.OneTimeMessageChannel} */
MessageDispatcher.prototype.getChannel = function(extensionId) {
  return this.channels_.get(extensionId, null);
};

/** @type {function(string,function()=):!GSC.OneTimeMessageChannel} */
MessageDispatcher.prototype.createChannel = function(extensionId, opt_onEstablished) {
  var channel = this.getChannel(extensionId);
  if (!channel) {
    this.logger.fine('Created a new channel, id = ' + extensionId);
    channel = new GSC.OneTimeMessageChannel(extensionId, opt_onEstablished);
    this.channels_.set(extensionId, channel);
    channel.addOnDisposeCallback(
        this.handleChannelDisposed_.bind(this, extensionId));
  }
  return channel;
};

/** @private @type {function()} */
MessageDispatcher.prototype.handleChannelDisposed_ = function(extensionId) {
  this.logger.fine('Disposed of channel id = ' + extensionId);
  this.channels_.remove(extensionId);
};

/** @type {function(string,string,string)} */
MessageDispatcher.prototype.send = function(extensionId, serviceName, payload) {
  this.createChannel(extensionId).send(serviceName, payload);
};

});  // goog.scope
