/**
 * @license
 * Copyright 2024 Google Inc. All Rights Reserved.
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

goog.provide('GoogleSmartCard.DelayedMessageChannel');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Forwards all communication to the given channel once it's ready for
 * communication, but remembers all to-be-sent messages until that happens.
 *
 * The correct order of operations:
 * create ==> `setUnderlyingChannel()` ==> `setReady()`.
 */
GSC.DelayedMessageChannel = class extends goog.messaging.AbstractChannel {
  constructor() {
    super();
    /** @type {!goog.messaging.AbstractChannel|null} @private */
    this.underlyingChannel_ = null;
    /**
     * @type {!Array<!{serviceName: string, payload:
     *     (string|!Object)}>} @private
     */
    this.pendingOutgoingMessages_ = [];
    /** @type {boolean} @private */
    this.ready_ = false;
  }

  /**
   * Associates us with the given channel, but doesn't unblock sending messages
   * to it yet.
   * @param {!goog.messaging.AbstractChannel} underlyingChannel
   */
  setUnderlyingChannel(underlyingChannel) {
    if (this.isDisposed())
      return;
    GSC.Logging.check(!this.underlyingChannel_);
    this.underlyingChannel_ = underlyingChannel;
    // Let ourselves receive all (unhandled) messages that were received on
    // `underlyingChannel`.
    underlyingChannel.registerDefaultService((serviceName, payload) => {
      this.deliver(serviceName, payload);
    });
  }

  /**
   * Unblocks sending messages to the underlying channel. Previously accumulated
   * messages are sent immediately.
   */
  setReady() {
    if (this.isDisposed())
      return;
    GSC.Logging.check(this.underlyingChannel_);
    GSC.Logging.check(!this.ready_);
    this.ready_ = true;
    // Send all previously enqueued messages. Note that, in theory, new items
    // might be added to the array while we're iterating over it, which should
    // be fine as the for-of loop will visit all of them.
    for (const message of this.pendingOutgoingMessages_)
      this.underlyingChannel_.send(message.serviceName, message.payload);
    this.pendingOutgoingMessages_ = [];
  }

  /** @override */
  send(serviceName, payload) {
    if (this.isDisposed())
      return;
    if (!this.ready_ || this.pendingOutgoingMessages_.length > 0) {
      // Enqueue the message until the proxied channel becomes ready and all
      // previously enqueued messages are sent.
      this.pendingOutgoingMessages_.push({serviceName, payload});
      return;
    }
    this.underlyingChannel_.send(serviceName, payload);
  }

  /** @override */
  disposeInternal() {
    this.pendingOutgoingMessages_ = [];
    if (this.underlyingChannel_) {
      this.underlyingChannel_.dispose();
      this.underlyingChannel_ = null;
    }
    super.disposeInternal();
  }
};
});
