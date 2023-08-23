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

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPinging.PingResponder');
goog.require('GoogleSmartCard.MessageChannelPinging.Pinger');
goog.require('GoogleSmartCard.MockPort');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const PortMessageChannel = GSC.PortMessageChannel;

const Pinger = GSC.MessageChannelPinging.Pinger;

const PingResponder = GSC.MessageChannelPinging.PingResponder;

const TypedMessage = GSC.TypedMessage;

/** Mock matcher that matches only the ping messages. */
const isPingMessageMockMatcher =
    new goog.testing.mockmatchers.ArgumentMatcher(function(value) {
      const typedMessage = TypedMessage.parseTypedMessage(value);
      return typedMessage && typedMessage.type == Pinger.SERVICE_NAME;
    }, 'isPingMessage');

/**
 * Sets up ping responding to ping messages posted through the mock port.
 * @param {!GSC.MockPort} mockPort
 */
function setUpPingRespondingForMockPort(mockPort) {
  const channelId = PingResponder.generateChannelId();
  mockPort.postMessage(isPingMessageMockMatcher)
      .$atLeastOnce()
      .$does(function(message) {
        const typedMessage = TypedMessage.parseTypedMessage(message);
        GSC.Logging.check(
            typedMessage && typedMessage.type == Pinger.SERVICE_NAME);
        const pingResponseMessage =
            new TypedMessage(
                PingResponder.SERVICE_NAME,
                PingResponder.createMessageData(channelId))
                .makeMessage();
        mockPort.fireOnMessage(pingResponseMessage);
      });
}

// Test that the channel is successfully established once a response to the ping
// request is received.
goog.exportSymbol('testPortMessageChannelEstablishing', function() {
  const testCasePromiseResolver = goog.Promise.withResolver();

  const mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  function onPortMessageChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  const portMessageChannel = new PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);
  portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that the channel is not established and is disposed of when there are no
// responses to the ping requests received.
goog.exportSymbol('testPortMessageChannelFailureToEstablish', {

  'setUp': function() {
    // Override the pinger's default timeouts, to prevent the test from hitting
    // the test framework's limit on the single test execution.
    Pinger.overrideTimeoutForTesting(400);
    Pinger.overrideIntervalForTesting(200);
  },

  'tearDown': function() {
    // Restore pinger's default timeouts.
    Pinger.overrideTimeoutForTesting(null);
    Pinger.overrideIntervalForTesting(null);
  },

  'test': function() {
    const testCasePromiseResolver = goog.Promise.withResolver();

    const mockPort = new GSC.MockPort('mock port');
    mockPort.postMessage(goog.testing.mockmatchers.isObject).$anyTimes();
    mockPort.postMessage.$replay();

    function onPortMessageChannelEstablished() {
      testCasePromiseResolver.reject();
    }

    function onPortMessageChannelDisposed() {
      mockPort.postMessage.$verify();
      testCasePromiseResolver.resolve();
      mockPort.dispose();
    }

    const portMessageChannel = new PortMessageChannel(
        mockPort.getFakePort(), onPortMessageChannelEstablished);
    portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

    return testCasePromiseResolver.promise;
  }
});

// Test that the messages sent through the port message channel are posted
// through the port with preserving the relative order.
goog.exportSymbol('testPortMessageChannelMessageSending', function() {
  const testCasePromiseResolver = goog.Promise.withResolver();

  const TEST_MESSAGES = [
    new TypedMessage('service x', {}),
    new TypedMessage('service x', {some_data: 1}),
    new TypedMessage('service y', {})
  ];

  const mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  for (const testMessage of TEST_MESSAGES) {
    mockPort
        .postMessage(new goog.testing.mockmatchers.ObjectEquals(
            testMessage.makeMessage()))
        .$once();
  }
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    for (const testMessage of TEST_MESSAGES)
      portMessageChannel.send(testMessage.type, testMessage.data);

    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  function onPortMessageChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  const portMessageChannel = new PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);
  portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that array buffers in sent messages are substituted with byte arrays.
goog.exportSymbol('testPortMessageChannelArrayBufferSending', function() {
  const MESSAGE_TYPE = 'foo';
  const MESSAGE_DATA = {x: (new Uint8Array([1, 255])).buffer};
  const EXPECTED_TRANSMITTED_DATA = {x: [1, 255]};
  const EXPECTED_TRANSMITTED_MESSAGE =
      new TypedMessage(MESSAGE_TYPE, EXPECTED_TRANSMITTED_DATA);

  const mockPort = new GSC.MockPort('mock port');
  mockPort
      .postMessage(new goog.testing.mockmatchers.ObjectEquals(
          EXPECTED_TRANSMITTED_MESSAGE.makeMessage()))
      .$once();
  mockPort.postMessage.$replay();

  const portMessageChannel = new PortMessageChannel(mockPort.getFakePort());
  portMessageChannel.send(MESSAGE_TYPE, MESSAGE_DATA);
  mockPort.postMessage.$verify();

  mockPort.dispose();
});

// Test that the port message channel passes the messages received from the port
// to the correct services with preserving the relative order.
goog.exportSymbol('testPortMessageChannelMessageReceiving', function() {
  const testCasePromiseResolver = goog.Promise.withResolver();

  const TEST_MESSAGES = [
    new TypedMessage('service x', {}),
    new TypedMessage('service x', {some_data: 1}),
    new TypedMessage('service y', {})
  ];

  const mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    const receivedMessages = [];

    for (const testMessage of TEST_MESSAGES) {
      portMessageChannel.registerService(
          testMessage.type, function(messageData) {
            GSC.Logging.check(goog.isObject(messageData));
            goog.asserts.assert(goog.isObject(messageData));
            receivedMessages.push(
                new TypedMessage(testMessage.type, messageData));
          }, true);
    }

    for (const testMessage of TEST_MESSAGES)
      mockPort.fireOnMessage(testMessage.makeMessage());

    assertObjectEquals(TEST_MESSAGES, receivedMessages);

    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  function onPortMessageChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  const portMessageChannel = new PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);
  portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that the port message channel is disposed of when the port gets
// disconnected.
goog.exportSymbol('testPortMessageChannelDisconnection', function() {
  const testCasePromiseResolver = goog.Promise.withResolver();

  const mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    mockPort.getFakePort().disconnect();
    assert(portMessageChannel.isDisposed());

    assertThrows(function() {
      portMessageChannel.send('foo', {});
    });

    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  const portMessageChannel = new PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);

  return testCasePromiseResolver.promise;
});
});  // goog.scope
