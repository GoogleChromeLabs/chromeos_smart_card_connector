/**
 * @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.LibusbLoginStateHook');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('goog.Thenable');
goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const USER_PROFILE_TYPE = 'USER_PROFILE';
const SIGNIN_PROFILE_TYPE = 'SIGNIN_PROFILE';
const IN_SESSION_STATE = 'IN_SESSION';
const IN_LOCK_SCREEN_STATE = 'IN_LOCK_SCREEN';
const FAKE_DEVICE_ID = 123;
const FAKE_DEVICE = {
  'deviceId': FAKE_DEVICE_ID,
  'vendorId': 1,
  'productId': 2
};
const FAKE_DEVICE_CONFIG = {
  'active': true,
  'configurationValue': 4
};
const FAKE_DEVICE_HANDLE = 456;

let sessionStateListeners = [];

/**
 * Sets up mocks for the chrome.loginState Extensions API.
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 * @param {string} fakeProfileType Fake data to be returned from
 * getProfileType().
 * @param {string} fakeSessionState Fake data to be returned from
 * getSessionState().
 */
function setUpChromeLoginStateMock(
    propertyReplacer, fakeProfileType, fakeSessionState) {
  propertyReplacer.set(chrome, 'loginState', {
    'getProfileType': function(callback) {
      callback(fakeProfileType);
    },
    'getSessionState': function(callback) {
      callback(fakeSessionState);
    },
    'onSessionStateChanged': {
      'addListener': function(callback) {
        sessionStateListeners.push(callback);
      }
    },
    'ProfileType': {
      'USER_PROFILE': USER_PROFILE_TYPE,
      'SIGNIN_PROFILE': SIGNIN_PROFILE_TYPE
    },
    'SessionState':
        {'IN_SESSION': IN_SESSION_STATE, 'IN_LOCK_SCREEN': IN_LOCK_SCREEN_STATE}
  });
}

/**
 * Wraps the given test function, providing the necessary setup and teardown.
 * @param {string} fakeProfileType Fake data to be
 * returned from chrome.loginState.getProfileType().
 * @param {string} fakeSessionState Fake data to be
 * returned from chrome.loginState.getSessionState().
 * @param {function(!goog.testing.PropertyReplacer)} testCallback
 * The test function to be run after the needed setup.
 * @return {function()} The wrapped test function.
 */
function makeTest(fakeProfileType, fakeSessionState, testCallback) {
  return function() {
    const propertyReplacer = new goog.testing.PropertyReplacer();

    function cleanup() {
      propertyReplacer.reset();
      sessionStateListeners = [];
    }

    setUpChromeLoginStateMock(
        propertyReplacer, fakeProfileType, fakeSessionState);

    let testPromise;
    /** @preserveTry */
    try {
      testPromise = testCallback(propertyReplacer);
    } catch (exc) {
      // Cleanup after the test fatally failed synchronously.
      cleanup();
      throw exc;
    }
    // Return a child promise that performs cleanup after the test.
    return testPromise.then(cleanup, cleanup);
  };
}

class FakeLibusbProxyHookDelegate extends GSC.LibusbToJsApiAdaptor {
  /** @override */
  async listDevices() {
    return [FAKE_DEVICE];
  }
  /** @override */
  async getConfigurations(deviceId) {
    assertEquals(deviceId, FAKE_DEVICE_ID);
    return [FAKE_DEVICE_CONFIG];
  }
  /** @override */
  async openDeviceHandle(deviceId) {
    return FAKE_DEVICE_HANDLE;
  }
  /** @override */
  async closeDeviceHandle(deviceId, deviceHandle) {
    fail('Unexpected call');
    goog.asserts.fail();
  }
  /** @override */
  async claimInterface(deviceId, deviceHandle, interfaceNumber) {
    fail('Unexpected call');
    goog.asserts.fail();
  }
  /** @override */
  async releaseInterface(deviceId, deviceHandle, interfaceNumber) {
    fail('Unexpected call');
    goog.asserts.fail();
  }
  /** @override */
  async resetDevice(deviceId, deviceHandle) {
    fail('Unexpected call');
    goog.asserts.fail();
  }
  /** @override */
  async controlTransfer(deviceId, deviceHandle, parameters) {
    fail('Unexpected call');
    goog.asserts.fail();
  }
  /** @override */
  async bulkTransfer(deviceId, deviceHandle, parameters) {
    fail('Unexpected call');
    goog.asserts.fail();
  }
  /** @override */
  async interruptTransfer(deviceId, deviceHandle, parameters) {
    fail('Unexpected call');
    goog.asserts.fail();
  }
}

/**
 * Simulates Chrome changing the session state.
 * @param {string} fakeNewSessionState
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 */
function changeSessionState(fakeNewSessionState, propertyReplacer) {
  propertyReplacer.replace(
      chrome.loginState, 'getSessionState', function(callback) {
        callback(fakeNewSessionState);
      });
  goog.array.forEach(sessionStateListeners, function(listener) {
    listener(fakeNewSessionState);
  });
}

goog.exportSymbol('test_ChromeLoginStateHook_NoApi', async function() {
  const loginStateHook = new GSC.LibusbLoginStateHook();
  loginStateHook.setDelegate(new FakeLibusbProxyHookDelegate());
  const devices = await loginStateHook.listDevices();
  assertObjectEquals(devices, [FAKE_DEVICE]);
  const configs = await loginStateHook.getConfigurations(FAKE_DEVICE_ID);
  assertObjectEquals(configs, [FAKE_DEVICE_CONFIG]);
});

goog.exportSymbol(
    'test_ChromeLoginStateHook_DoesNotFilterInSession',
    makeTest(USER_PROFILE_TYPE, IN_SESSION_STATE, async function() {
      const loginStateHook = new GSC.LibusbLoginStateHook();
      loginStateHook.setDelegate(new FakeLibusbProxyHookDelegate());
      const devices = await loginStateHook.listDevices();
      assertObjectEquals(devices, [FAKE_DEVICE]);
      const configs = await loginStateHook.getConfigurations(FAKE_DEVICE_ID);
      assertObjectEquals(configs, [FAKE_DEVICE_CONFIG]);
    }));

goog.exportSymbol(
    'test_ChromeLoginStateHook_FiltersInLockScreen',
    makeTest(USER_PROFILE_TYPE, IN_LOCK_SCREEN_STATE, async function() {
      const loginStateHook = new GSC.LibusbLoginStateHook();
      loginStateHook.setDelegate(new FakeLibusbProxyHookDelegate());
      const devices = await loginStateHook.listDevices();
      assertEquals(devices.length, 0);
      const configs = await loginStateHook.getConfigurations(FAKE_DEVICE_ID);
      assertEquals(configs.length, 0);
      // only listDevices and getConfigurations should get filtered
      const handle = await loginStateHook.openDeviceHandle(FAKE_DEVICE_ID);
      assertEquals(handle, FAKE_DEVICE_HANDLE);
    }));

goog.exportSymbol(
    'test_ChromeLoginStateHook_DoesNotFilterForSignInProfile',
    makeTest(SIGNIN_PROFILE_TYPE, IN_SESSION_STATE, async function() {
      const loginStateHook = new GSC.LibusbLoginStateHook();
      loginStateHook.setDelegate(new FakeLibusbProxyHookDelegate());
      const devices = await loginStateHook.listDevices();
      assertObjectEquals(devices, [FAKE_DEVICE]);
    }));

goog.exportSymbol(
    'test_ChromeLoginStateHook_ChangeToLockScreen',
    makeTest(
        USER_PROFILE_TYPE, IN_SESSION_STATE, async function(propertyReplacer) {
          const loginStateHook = new GSC.LibusbLoginStateHook();
          loginStateHook.setDelegate(new FakeLibusbProxyHookDelegate());
          changeSessionState(IN_LOCK_SCREEN_STATE, propertyReplacer);
          const devices = await loginStateHook.listDevices();
          assertEquals(devices.length, 0);
        }));

goog.exportSymbol(
    'test_ChromeLoginStateHook_ChangeToSession',
    makeTest(
        USER_PROFILE_TYPE, IN_LOCK_SCREEN_STATE,
        async function(propertyReplacer) {
          const loginStateHook = new GSC.LibusbLoginStateHook();
          loginStateHook.setDelegate(new FakeLibusbProxyHookDelegate());
          changeSessionState(IN_SESSION_STATE, propertyReplacer);
          const devices = await loginStateHook.listDevices();
          assertObjectEquals(devices, [FAKE_DEVICE]);
        }));
});  // goog.scope
