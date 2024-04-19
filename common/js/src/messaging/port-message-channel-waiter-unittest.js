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

goog.require('GoogleSmartCard.PortMessageChannelWaiter');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const propertyReplacer = new goog.testing.PropertyReplacer();

/** @type {function(!Port)|undefined} */
let interceptedListener;

/**
 * Stub for `chrome.runtime.onConnect.addListener()`.
 * @param {function(!Port)} listener
 */
function fakeAddListener(listener) {
  assertUndefined(interceptedListener);
  interceptedListener = listener;
}

/**
 * Stub for `chrome.runtime.onConnect.removeListener()`.
 * @param {function(!Port)} listener
 */
function fakeRemoveListener(listener) {
  assertEquals(interceptedListener, listener);
  interceptedListener = undefined;
}

/**
 * @param {!Port} port
 */
function simulateOnConnect(port) {
  if (interceptedListener)
    interceptedListener(port);
}

goog.exportSymbol('testPortMessageChannelWaiter', {
  'setUp': function() {
    propertyReplacer.setPath(
        'chrome.runtime.onConnect.addListener', fakeAddListener);
    propertyReplacer.setPath(
        'chrome.runtime.onConnect.removeListener', fakeRemoveListener);
  },

  'tearDown': function() {
    propertyReplacer.reset();
    interceptedListener = undefined;
  },

  'testBasic': async function() {
    // Arrange.
    const PORT_NAME = 'someport';
    const waiter = new GSC.PortMessageChannelWaiter(PORT_NAME);
    const mockPort = new GSC.MockPort(PORT_NAME);

    // Act: simulate a port connection.
    simulateOnConnect(mockPort.getFakePort());

    // Assert: the waiter should return the port (wrapped into a channel
    // object).
    const portChannel = await waiter.getPromise();
    assertEquals(portChannel.getPortForTesting(), mockPort.getFakePort());

    // Cleanup.
    waiter.dispose();
  },

  // When multiple ports with the same name are connected, the waiter returns
  // the first and ignores others.
  'testSameNamedPorts': async function() {
    // Arrange.
    const PORT_NAME = 'someport';
    const waiter = new GSC.PortMessageChannelWaiter(PORT_NAME);
    const mockPort1 = new GSC.MockPort(PORT_NAME);
    const mockPort2 = new GSC.MockPort(PORT_NAME);

    // Act: simulate port connections.
    simulateOnConnect(mockPort1.getFakePort());
    simulateOnConnect(mockPort2.getFakePort());

    // Assert: the waiter should return the first port.
    const portChannel = await waiter.getPromise();
    assertEquals(portChannel.getPortForTesting(), mockPort1.getFakePort());

    // Cleanup.
    waiter.dispose();
  },

  // The waiter ignores unrelated port connections.
  'testUnrelatedPortIgnored': async function() {
    // Arrange.
    const PORT_NAME = 'someport';
    const OTHER_PORT_NAME = 'otherport';
    const waiter = new GSC.PortMessageChannelWaiter(PORT_NAME);
    const mockOtherPort = new GSC.MockPort(OTHER_PORT_NAME);
    const mockPort = new GSC.MockPort(PORT_NAME);

    // Act: simulate port connections.
    simulateOnConnect(mockOtherPort.getFakePort());
    simulateOnConnect(mockPort.getFakePort());

    // Assert: the waiter should return the needed port.
    const portChannel = await waiter.getPromise();
    assertEquals(portChannel.getPortForTesting(), mockPort.getFakePort());

    // Cleanup.
    waiter.dispose();
  },

  // The waiter's promise is rejected if no port has been connected by the
  // disposal time.
  'testDisposedWithoutPort': async function() {
    // Arrange.
    const waiter = new GSC.PortMessageChannelWaiter('someport');

    // Act.
    waiter.dispose();

    // Assert.
    await assertRejects(waiter.getPromise());

    // Cleanup.
    waiter.dispose();
  },
});
});  // goog.scope
