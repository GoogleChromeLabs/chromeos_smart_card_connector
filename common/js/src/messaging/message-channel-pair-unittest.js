/**
 * @license
 * Copyright 2021 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.MessageChannelPair');

goog.require('goog.messaging.AbstractChannel');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Wraps the given message channel into a promise, for simplifying writing unit
 * tests.
 * @param {!goog.messaging.AbstractChannel} messageChannel
 * @return {!Promise<!Object|string>} serviceName
 */
function getPromiseForReceivedMessage(messageChannel, serviceName) {
  return new Promise((resolve, reject) => {
    messageChannel.registerService(serviceName, (receivedMessagePayload) => {
      resolve(receivedMessagePayload);
    });
  });
}

/**
 * Adds a listener that aborts the test when an unexpected message is received
 * (i.e., technically, when the message channel's default service is triggered).
 * @param {!goog.messaging.AbstractChannel} messageChannel
 */
function failWhenReceivedUnexpectedMessage(messageChannel) {
  messageChannel.registerDefaultService((payload) => {
    fail(`Unexpected message received: ${payload}`);
  });
}

/**
 * @param {number} delayMilliseconds
 */
async function sleep(delayMilliseconds) {
  return new Promise((resolve, reject) => {
    setTimeout(resolve, delayMilliseconds);
  });
}

// Send a message via the first item of the pair and check that it's received at
// the second item of the pair.
goog.exportSymbol('test_MessageChannelPair_sendViaFirst', async function() {
  const messageChannelPair = new GSC.MessageChannelPair();

  // Prepare: add listeners.
  const promiseForReceivedMessage = getPromiseForReceivedMessage(
      messageChannelPair.getSecond(), /*serviceName=*/ 'some-service');
  failWhenReceivedUnexpectedMessage(messageChannelPair.getSecond());

  // Act: send the message.
  messageChannelPair.getFirst().send(
      /*serviceName=*/ 'some-service', /*payload=*/ 'data');

  // Assert: wait for delivery and check the result.
  assertEquals(await promiseForReceivedMessage, 'data');
});

// Inversion of the test above.
goog.exportSymbol('test_MessageChannelPair_sendViaSecond', async function() {
  const messageChannelPair = new GSC.MessageChannelPair();

  // Prepare: add listeners.
  const promiseForReceivedMessage = getPromiseForReceivedMessage(
      messageChannelPair.getFirst(), /*serviceName=*/ 'some-service');
  failWhenReceivedUnexpectedMessage(messageChannelPair.getFirst());

  // Act: send the message.
  messageChannelPair.getSecond().send(
      /*serviceName=*/ 'some-service', /*payload=*/ 'data');

  // Assert: wait for delivery and check the result.
  assertEquals(await promiseForReceivedMessage, 'data');
});

// That that the disposal of the channel pair causes disposing of both items of
// the pair.
goog.exportSymbol('test_MessageChannelPair_dispose', function() {
  const messageChannelPair = new GSC.MessageChannelPair();
  const first = messageChannelPair.getFirst();
  const second = messageChannelPair.getSecond();
  messageChannelPair.dispose();
  assertTrue(first.isDisposed());
  assertTrue(second.isDisposed());
});

// That that the disposal of one item of the channel pair causes disposing of
// the channel pair itself and the other item too.
goog.exportSymbol('test_MessageChannelPair_disposeItem', function() {
  const messageChannelPair = new GSC.MessageChannelPair();
  const first = messageChannelPair.getFirst();
  const second = messageChannelPair.getSecond();
  first.dispose();
  assertTrue(messageChannelPair.isDisposed());
  assertTrue(second.isDisposed());
});

// Test that the message to be sent is discarded if the channel pair is disposed
// of before the sending task starts.
goog.exportSymbol(
    'test_MessageChannelPair_disposePairQuicklyAfterSend', async function() {
      const messageChannelPair = new GSC.MessageChannelPair();

      // Prepare: add listeners.
      failWhenReceivedUnexpectedMessage(messageChannelPair.getSecond());

      // Act: Send a message and dispose the channel pair immediately after
      // that. The message delivery should be canceled, because the channel
      // pair's API guarantees it to happen asynchronously.
      messageChannelPair.getFirst().send(
          /*serviceName=*/ 'some-service', /*payload=*/ 'data');
      messageChannelPair.dispose();

      // Assert: Wait for some time before resolving the test, to let the bug,
      // if it's present, manifest itself. There's no reliable way to wait for
      // this, since normally the message sent after disposal should be
      // silently discarded.
      await sleep(/*delayMilliseconds=*/ 1000);
    });
});  // goog.scope
