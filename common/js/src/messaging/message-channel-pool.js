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
 * Closure-style AbstractChannel's used for messaging. A single origin can have
 * multiple channels associated with it.
 */

goog.provide('GoogleSmartCard.MessageChannelPool');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.array');
goog.require('goog.labs.structs.Multimap');
goog.require('goog.log');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * This class is a pool which keeps track of Closure-style AbstractChannel's.
 */
GSC.MessageChannelPool = class {
constructor() {
  /**
   * @type {!goog.log.Logger}
   * @const
   */
  this.logger = GSC.Logging.getScopedLogger('MessageChannelPool');

  /**
   * Multimap from string messaging origin to !goog.messaging.AbstractChannel.
   *
   * TODO(isandrk): the origin may be null (extension talks to itself)
   * @type {!goog.labs.structs.Multimap}
   * @private @const
   */
  this.channels_ = new goog.labs.structs.Multimap;

  /** @type {!Array.<function(!Array.<string>)>} @private @const */
  this.onUpdateListeners_ = [];

  goog.log.fine(this.logger, 'Initialized successfully');
}

/**
 * @param {string} messagingOrigin
 * @return {!Array.<!goog.messaging.AbstractChannel>}
 */
getChannels(messagingOrigin) {
  return /** @type {!Array.<!goog.messaging.AbstractChannel>} */ (
      this.channels_.get(messagingOrigin));
}

/**
 * Returns the messagingOrigin's of all connected channels.
 * @return {!Array.<string>}
 */
getMessagingOrigins() {
  return this.channels_.getKeys();
}

/**
 * @param {string} messagingOrigin
 * @param {!goog.messaging.AbstractChannel} messageChannel
 */
addChannel(messagingOrigin, messageChannel) {
  if (this.channels_.containsEntry(messagingOrigin, messageChannel)) {
    GSC.Logging.failWithLogger(
        this.logger, 'Tried to add a channel that was already present');
  }
  goog.log.fine(
      this.logger, 'Added a new channel, origin = ' + messagingOrigin);
  this.channels_.add(messagingOrigin, messageChannel);
  messageChannel.addOnDisposeCallback(
      this.handleChannelDisposed_.bind(this, messagingOrigin, messageChannel));
  this.fireOnUpdateListeners_();
}

/**
 * Adds a listener that will be called with the list of app information
 * objects each time it gets updated.
 * @param {function(!Array.<string>)} listener
 * @param {!Object=} opt_scope
 */
addOnUpdateListener(listener, opt_scope) {
  this.onUpdateListeners_.push(
      opt_scope !== undefined ? goog.bind(listener, opt_scope) : listener);
  goog.log.fine(this.logger, 'Added an OnUpdateListener');

  // Fire it once immediately to update.
  this.fireOnUpdateListeners_();
}

/**
 * Removes a previously added listener function.
 * @param {function(!Array.<string>)} listener
 */
removeOnUpdateListener(listener) {
  if (goog.array.remove(this.onUpdateListeners_, listener)) {
    goog.log.fine(this.logger, 'Removed an update listener');
  } else {
    goog.log.warning(
        this.logger,
        'Failed to remove an update listener: the passed ' +
            'function was not found');
  }
}

/**
 * @private
 */
fireOnUpdateListeners_() {
  goog.log.fine(this.logger, 'Firing channel update listeners');
  for (let listener of this.onUpdateListeners_) {
    try {
      listener(this.getMessagingOrigins());
    } catch (exc) {
      // Rethrow the listener's exception asynchronously, in order to not break
      // our caller and also to still notify the remaining listeners.
      setTimeout(() => {
        throw exc;
      })
    }
  }
}

/**
 * @private
 * @param {string} messagingOrigin
 * @param {!goog.messaging.AbstractChannel} messageChannel
 */
handleChannelDisposed_(messagingOrigin, messageChannel) {
  goog.log.fine(
      this.logger, 'Disposed of channel, origin = ' + messagingOrigin);
  if (!this.channels_.remove(messagingOrigin, messageChannel)) {
    GSC.Logging.failWithLogger(
        this.logger, 'Tried to dispose of non-existing channel');
  }
  this.fireOnUpdateListeners_();
}
};
});  // goog.scope
