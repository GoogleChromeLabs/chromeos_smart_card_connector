/** @license
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
goog.require('goog.testing.MockControl');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.messaging.MockMessageChannel');

goog.setTestOnly();

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var Requester = GSC.Requester;
/** @const */
var RequestMessageData = GSC.RequesterMessage.RequestMessageData;
/** @const */
var ResponseMessageData = GSC.RequesterMessage.ResponseMessageData;
/** @const */
var getRequestMessageType = GSC.RequesterMessage.getRequestMessageType;
/** @const */
var getResponseMessageType = GSC.RequesterMessage.getResponseMessageType;

/** @constructor */
function PromiseTracker(promise) {
  this.isResolved = false;
  this.isFulfilled = false;
  this.isRejected = false;
  promise.then(
      this.onPromiseFulfilled_.bind(this), this.onPromiseRejected_.bind(this));
}

PromiseTracker.prototype.onPromiseFulfilled_ = function() {
  this.isResolved = true;
  this.isFulfilled = true;
};

PromiseTracker.prototype.onPromiseRejected_ = function() {
  this.isResolved = true;
  this.isRejected = true;
};

goog.exportSymbol('testRequester', function() {
  /** @const */
  var REQUESTER_NAME = 'test-requester';
  /** @const */
  var REQUEST_MESSAGE_TYPE = getRequestMessageType(REQUESTER_NAME);
  /** @const */
  var RESPONSE_MESSAGE_TYPE = getResponseMessageType(REQUESTER_NAME);
  /** @const */
  var REQUEST_0_PAYLOAD = {};
  /** @const */
  var REQUEST_1_PAYLOAD = {foo: 'bar'};
  /** @const */
  var REQUEST_2_PAYLOAD = {foo: [1, 2, 'test'], bar: {baz: 3}};
  /** @const */
  var REQUEST_1_ERROR_MESSAGE = 'test error';
  /** @const */
  var REQUEST_2_RESPONSE_PAYLOAD = {some_result: 123};

  var mockControl = new goog.testing.MockControl;
  var mockMessageChannel = new goog.testing.messaging.MockMessageChannel(
      mockControl);
  /** @type {?} */ mockMessageChannel.send;

  var requester = new GSC.Requester(REQUESTER_NAME, mockMessageChannel);

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
  var request0Promise = requester.postRequest(REQUEST_0_PAYLOAD);
  var request1Promise = requester.postRequest(REQUEST_1_PAYLOAD);
  var request2Promise = requester.postRequest(REQUEST_2_PAYLOAD);
  mockMessageChannel.send.$verify();

  var request0PromiseTracker = new PromiseTracker(request0Promise);
  var request1PromiseTracker = new PromiseTracker(request1Promise);
  var request2PromiseTracker = new PromiseTracker(request2Promise);

  // Send success response message to request #2. The corresponding promise gets
  // fulfilled with the result.
  mockMessageChannel.receive(
      RESPONSE_MESSAGE_TYPE,
      (new ResponseMessageData(2, REQUEST_2_RESPONSE_PAYLOAD))
          .makeMessageData());
  var request2CompletionPromise = request2Promise.then(
      function(fulfillmentValue) {
        assertEquals(REQUEST_2_RESPONSE_PAYLOAD, fulfillmentValue);
        assert(!request0PromiseTracker.isResolved);
        assert(!request1PromiseTracker.isResolved);
        assert(request2PromiseTracker.isResolved);
      });

  // After that, send error response message to request #1. The corresponding
  // promise gets rejected with the error message.
  var request1CompletionPromise = request2CompletionPromise.then(function() {
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
  var request0CompletionPromise = request1CompletionPromise.then(function() {
    mockMessageChannel.dispose();
    // Hack: call the protected disposeInternal() method in order to trigger the
    // disposal notifications; the MockMessageChannel.dispose() doesn't call
    // them.
    mockMessageChannel['disposeInternal']();
    return request0Promise.then(null, function(rejectionError) {
      assert(request0PromiseTracker.isResolved);
      assert(request1PromiseTracker.isResolved);
      assert(request2PromiseTracker.isResolved);
    });
  });

  return request0CompletionPromise;
});

});  // goog.scope
