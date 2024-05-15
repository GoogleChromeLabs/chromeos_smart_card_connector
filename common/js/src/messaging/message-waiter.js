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

/**
 * @fileoverview A helper for waiting for an incoming message with the specified
 * type.
 */

goog.provide('GoogleSmartCard.MessageWaiter');

goog.require('goog.messaging.AbstractChannel');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Waits for the message with the specified type to arrive on the given channel,
 * and returns it. Rejects the promise if the channel is disposed of.
 * @param {goog.messaging.AbstractChannel} channel
 * @param {string} awaitedMessageType
 * @return {!Promise}
 */
GSC.MessageWaiter.wait = function(channel, awaitedMessageType) {
  return new Promise((resolve, reject) => {
    // Listen for the message.
    channel.registerService(awaitedMessageType, (data) => {
      resolve(data);
    }, /*opt_objectPayload=*/ true);

    // Listen for the disposal event.
    channel.addOnDisposeCallback(
        () => reject(new Error('Message channel disposed of')));
  });
};
});
