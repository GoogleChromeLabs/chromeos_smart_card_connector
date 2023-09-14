/**
 * @license
 * Copyright 2018 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.Requester');
goog.require('GoogleSmartCard.RequesterMessage');
goog.require('GoogleSmartCard.RequesterMessage.RequestMessageData');
goog.require('GoogleSmartCard.RequesterMessage.ResponseMessageData');
goog.require('goog.reflect');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.messaging.MockMessageChannel');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const Requester = GSC.Requester;
const RequestMessageData = GSC.RequesterMessage.RequestMessageData;
const ResponseMessageData = GSC.RequesterMessage.ResponseMessageData;
const getRequestMessageType = GSC.RequesterMessage.getRequestMessageType;
const getResponseMessageType = GSC.RequesterMessage.getResponseMessageType;

const ignoreArgument = goog.testing.mockmatchers.ignoreArgument;

class PromiseTracker {
  constructor(promise) {
    this.isResolved = false;
    this.isFulfilled = false;
    this.isRejected = false;
    promise.then(
        this.onPromiseFulfilled_.bind(this),
        this.onPromiseRejected_.bind(this));
  }

  /** @private */
  onPromiseFulfilled_() {
    this.isResolved = true;
    this.isFulfilled = true;
  }

  /** @private */
  onPromiseRejected_() {
    this.isResolved = true;
    this.isRejected = true;
  }
}

/**
 * @param {!goog.testing.messaging.MockMessageChannel} mockMessageChannel
 */
function disposeOfMockMessageChannel(mockMessageChannel) {
  mockMessageChannel.dispose();
  // Hack: call the protected disposeInternal() method in order to trigger the
  // disposal notifications; the MockMessageChannel.dispose() doesn't call
  // them.
  mockMessageChannel[goog.reflect.objectProperty(
      'disposeInternal', mockMessageChannel)]();
}

goog.exportSymbol('testRequester', function() {
  const REQUESTER_NAME = 'test-requester';
  const REQUEST_MESSAGE_TYPE = getRequestMessageType(REQUESTER_NAME);
  const RESPONSE_MESSAGE_TYPE = getResponseMessageType(REQUESTER_NAME);
  const REQUEST_0_PAYLOAD = {};
  const REQUEST_1_PAYLOAD = {foo: 'bar'};
  const REQUEST_2_PAYLOAD = {foo: [1, 2, 'test'], bar: {baz: 3}};
  const REQUEST_1_ERROR_MESSAGE = 'test error';
  const REQUEST_2_RESPONSE_PAYLOAD = {some_result: 123};

  const mockControl = new goog.testing.MockControl();
  const mockMessageChannel =
      new goog.testing.messaging.MockMessageChannel(mockControl);
  /** @type {?} */ mockMessageChannel.send;

  const requester = new Requester(REQUESTER_NAME, mockMessageChannel);

  // Set up expectation for three request messages.
  mockMessageChannel.send(
      REQUEST_MESSAGE_TYPE,
      (new RequestMessageData(0, REQUEST_0_PAYLOAD)).makeMessageData());
  mockMessageChannel.send(
      REQUEST_MESSAGE_TYPE,
      (new RequestMessageData(1, REQUEST_1_PAYLOAD)).makeMessageData());
  mockMessageChannel.send(
      REQUEST_MESSAGE_TYPE,
      (new RequestMessageData(2, REQUEST_2_PAYLOAD)).makeMessageData());
  mockMessageChannel.send.$replay();

  // Launch three requests. This sends three request messages.
  const request0Promise = requester.postRequest(REQUEST_0_PAYLOAD);
  const request1Promise = requester.postRequest(REQUEST_1_PAYLOAD);
  const request2Promise = requester.postRequest(REQUEST_2_PAYLOAD);
  mockMessageChannel.send.$verify();

  const request0PromiseTracker = new PromiseTracker(request0Promise);
  const request1PromiseTracker = new PromiseTracker(request1Promise);
  const request2PromiseTracker = new PromiseTracker(request2Promise);

  // Send success response message to request #2. The corresponding promise gets
  // fulfilled with the result.
  mockMessageChannel.receive(
      RESPONSE_MESSAGE_TYPE,
      (new ResponseMessageData(2, REQUEST_2_RESPONSE_PAYLOAD))
          .makeMessageData());
  const request2CompletionPromise =
      request2Promise.then(function(fulfillmentValue) {
        assertEquals(REQUEST_2_RESPONSE_PAYLOAD, fulfillmentValue);
        assert(!request0PromiseTracker.isResolved);
        assert(!request1PromiseTracker.isResolved);
        assert(request2PromiseTracker.isResolved);
      });

  // After that, send error response message to request #1. The corresponding
  // promise gets rejected with the error message.
  const request1CompletionPromise = request2CompletionPromise.then(function() {
    mockMessageChannel.receive(
        RESPONSE_MESSAGE_TYPE,
        (new ResponseMessageData(1, undefined, REQUEST_1_ERROR_MESSAGE))
            .makeMessageData());
    return request1Promise.then(null, function(rejectionError) {
      assertEquals(REQUEST_1_ERROR_MESSAGE, rejectionError['message']);
      assert(!request0PromiseTracker.isResolved);
      assert(request1PromiseTracker.isResolved);
      assert(request2PromiseTracker.isResolved);
    });
  });

  // After that, dispose of the message channel. The promise for request #0 gets
  // rejected.
  const request0CompletionPromise = request1CompletionPromise.then(function() {
    disposeOfMockMessageChannel(mockMessageChannel);
    return request0Promise.then(null, function(rejectionError) {
      assert(request0PromiseTracker.isResolved);
      assert(request1PromiseTracker.isResolved);
      assert(request2PromiseTracker.isResolved);
    });
  });

  return request0CompletionPromise;
});

goog.exportSymbol('testRequester_disposed', function() {
  const REQUESTER_NAME = 'test-requester';

  const mockControl = new goog.testing.MockControl();
  const mockMessageChannel =
      new goog.testing.messaging.MockMessageChannel(mockControl);
  /** @type {?} */ mockMessageChannel.send;

  const requester = new Requester(REQUESTER_NAME, mockMessageChannel);
  requester.dispose();

  const requestPromise = requester.postRequest({});
  // No message should be sent.
  mockMessageChannel.send.$verify();

  return requestPromise.then(() => {
    fail('Unexpected success');
  }, () => {});
});

// Test that an exception happening while sending the request message results in
// a rejected response promise.
goog.exportSymbol('testRequester_exceptionWhileSending', async function() {
  const REQUESTER_NAME = 'test-requester';
  const SEND_ERROR_MESSAGE = 'fake send error';

  const mockControl = new goog.testing.MockControl();
  const mockMessageChannel =
      new goog.testing.messaging.MockMessageChannel(mockControl);
  /** @type {?} */ mockMessageChannel.send;
  // Set up mock that throws an exception when a message is sent.
  mockMessageChannel.send(ignoreArgument, ignoreArgument)
      .$throws(new Error(SEND_ERROR_MESSAGE));
  mockMessageChannel.send.$replay();
  const requester = new Requester(REQUESTER_NAME, mockMessageChannel);

  try {
    await requester.postRequest({});
  } catch (e) {
    // This is the expected branch. Verify the error message is passed through.
    assertContains(SEND_ERROR_MESSAGE, e.toString());
    return;
  }
  fail('Message posting unexpectedly succeeded');
});

// Test that if sending the request message results in immediate disposal and an
// exception, the response promise is rejected correctly.
goog.exportSymbol(
    'testRequester_exceptionAndDisposalWhileSending', async function() {
      const REQUESTER_NAME = 'test-requester';
      const SEND_ERROR_MESSAGE = 'fake send error';

      const mockControl = new goog.testing.MockControl();
      const mockMessageChannel =
          new goog.testing.messaging.MockMessageChannel(mockControl);
      /** @type {?} */ mockMessageChannel.send;
      // Set up mock that throws an exception when a message is sent.
      mockMessageChannel.send(ignoreArgument, ignoreArgument).$does(() => {
        disposeOfMockMessageChannel(mockMessageChannel);
        throw new Error(SEND_ERROR_MESSAGE);
      });
      mockMessageChannel.send.$replay();
      const requester = new Requester(REQUESTER_NAME, mockMessageChannel);

      try {
        await requester.postRequest({});
      } catch (e) {
        // This is the expected branch.
        return;
      }
      fail('Message posting unexpectedly succeeded');
    });
});  // goog.scope
