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

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

// Send a message via the first item of the pair and check that it's received at
// the second item of the pair.
goog.exportSymbol('test_MessageChannelPair_sendViaFirst', function() {
  return new Promise((resolve, reject) => {
    const messageChannelPair = new GSC.MessageChannelPair();

    // Set up the expectation - once the right message is received on the
    // second item, make the test complete by resolving the promise.
    messageChannelPair.getSecond().registerService(
        /*serviceName=*/ 'some-service', (payload) => {
          assertEquals(payload, 'data');
          resolve();
        });
    // Any unexpected messages on the second item should abort the test.
    messageChannelPair.getSecond().registerDefaultService((payload) => {
      reject();
    })

    messageChannelPair.getFirst().send(
        /*serviceName=*/ 'some-service', /*payload=*/ 'data');
  });
});

// Inversion of the test above.
goog.exportSymbol('test_MessageChannelPair_sendViaSecond', function() {
  return new Promise((resolve, reject) => {
    const messageChannelPair = new GSC.MessageChannelPair();

    messageChannelPair.getFirst().registerService(
        /*serviceName=*/ 'some-service', (payload) => {
          assertEquals(payload, 'data');
          resolve();
        });
    messageChannelPair.getFirst().registerDefaultService((payload) => {
      reject();
    })

    messageChannelPair.getSecond().send(
        /*serviceName=*/ 'some-service', /*payload=*/ 'data');
  });
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
    'test_MessageChannelPair_disposePairQuicklyAfterSend', function() {
      return new Promise((resolve, reject) => {
        const messageChannelPair = new GSC.MessageChannelPair();

        // Abort the test if the message got delivered despite the disposal.
        messageChannelPair.getSecond().registerDefaultService((payload) => {
          reject();
        })

        // Send a message and dispose the channel pair immediately after that.
        // The message delivery should be canceled, because the channel pair's
        // API guarantees it to happen asynchronously.
        messageChannelPair.getFirst().send(
            /*serviceName=*/ 'some-service', /*payload=*/ 'data');
        messageChannelPair.dispose();

        // Wait for some time before resolving the test, to let the bug, if it's
        // present, manifest itself. There's no reliable way to wait for this,
        // since normally the message sent after disposal should be silently
        // discarded.
        setTimeout(resolve, 1000);
      });
    });
});  // goog.scope
