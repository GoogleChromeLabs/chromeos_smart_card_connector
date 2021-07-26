/** @license
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

goog.require('GoogleSmartCard.Libusb.ChromeLoginStateHook');
goog.require('GoogleSmartCard.Libusb.ChromeUsbBackend');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('goog.Promise');
goog.require('goog.array');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

/** @const */
const GSC = GoogleSmartCard;

/** @const */
const USER_PROFILE_TYPE = 'USER_PROFILE';
/** @const */
const SIGNIN_PROFILE_TYPE = 'SIGNIN_PROFILE';
/** @const */
const IN_SESSION_STATE = 'IN_SESSION';
/** @const */
const IN_LOCK_SCREEN_STATE = 'IN_LOCK_SCREEN';

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
  propertyReplacer.set(
      chrome, 'loginState',
      {
        getProfileType: function(callback) { callback(fakeProfileType); },
        getSessionState: function(callback) { callback(fakeSessionState); },
        onSessionStateChanged: {
          addListener: function(callback) {
            sessionStateListeners.push(callback);
          }
        },
        ProfileType: {
          USER_PROFILE: USER_PROFILE_TYPE,
          SIGNIN_PROFILE: SIGNIN_PROFILE_TYPE
        },
        SessionState: {
          IN_SESSION: IN_SESSION_STATE,
          IN_LOCK_SCREEN: IN_LOCK_SCREEN_STATE
        }
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
    const propertyReplacer = new goog.testing.PropertyReplacer;

    function cleanup() {
      propertyReplacer.reset();
      sessionStateListeners = [];
    }
    
    setUpChromeLoginStateMock(
        propertyReplacer, fakeProfileType, fakeSessionState);

    /** @preserveTry */
    try {
      var testPromise = testCallback(propertyReplacer);
    } catch (exc) {
      // Cleanup after the test fatally failed synchronously.
      cleanup();
      throw exc;
    }
    // Return a child promise that performs cleanup after the test.
    return testPromise.then(cleanup, cleanup);
  };
}

/**
 * Simulates Chrome changing the session state.
 * @param {string} fakeNewSessionState
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 */
function changeSessionState(fakeNewSessionState, propertyReplacer) {
  propertyReplacer.replace(
      chrome.loginState,
      'getSessionState', 
      function(callback) { callback(fakeNewSessionState); });
  goog.array.forEach(sessionStateListeners, function(listener) {
    listener(fakeNewSessionState);
  });
}

goog.exportSymbol(
    'test_ChromeLoginStateHook_NoApi', function() {
      const loginStateHook = new GSC.Libusb.ChromeLoginStateHook();
      // Expect that getHookReadyPromise() gets rejected, because the
      // chrome.loginState mock is not set up.
      return loginStateHook.getHookReadyPromise().then(() => {
        fail('Unexpected successful response');
      }, () => {});
    });

goog.exportSymbol(
    'test_ChromeLoginStateHook_DoesNotFilterInSession', makeTest(
      USER_PROFILE_TYPE, IN_SESSION_STATE,
      function() {
        const loginStateHook = new GSC.Libusb.ChromeLoginStateHook();
        return loginStateHook.getHookReadyPromise().then(() => {
          const hook = loginStateHook.getRequestSuccessHook();
          let apiCallResult = [['fakeResult']];
          const hookResult = hook('getDevices', apiCallResult);
          assertObjectEquals([['fakeResult']], hookResult);
        });
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_FiltersInLockScreen', makeTest(
      USER_PROFILE_TYPE, IN_LOCK_SCREEN_STATE,
      function() {
        const loginStateHook = new GSC.Libusb.ChromeLoginStateHook();
        return loginStateHook.getHookReadyPromise().then(() => {
          const hook = loginStateHook.getRequestSuccessHook();

          let apiCallResult = [['fakeResult']];
          let hookResult = hook('getDevices', apiCallResult);
          assertObjectEquals([[]], hookResult);

          apiCallResult = [['fakeResult']];
          hookResult = hook('getConfigurations', apiCallResult);
          assertObjectEquals([[]], hookResult);

          //only getDevices and getConfigurations should get filtered
          apiCallResult = ['otherCallResult'];
          hookResult = hook('listInterfaces', apiCallResult);
          assertObjectEquals(['otherCallResult'], hookResult);
        });
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_DoesNotFilterForSignInProfile', makeTest(
      SIGNIN_PROFILE_TYPE, IN_SESSION_STATE,
      function() {
        const loginStateHook = new GSC.Libusb.ChromeLoginStateHook();
        return loginStateHook.getHookReadyPromise().then(() => {
          const hook = loginStateHook.getRequestSuccessHook();
          const apiCallResult = [['fakeResult']];
          const hookResult = hook('getDevices', apiCallResult);
          assertObjectEquals([['fakeResult']], hookResult);
        });
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_ChangeToLockScreen', makeTest(
      USER_PROFILE_TYPE, IN_SESSION_STATE,
      function(propertyReplacer) {
        const loginStateHook = new GSC.Libusb.ChromeLoginStateHook();
        return loginStateHook.getHookReadyPromise().then(() => {
          changeSessionState(IN_LOCK_SCREEN_STATE, propertyReplacer);
          const hook = loginStateHook.getRequestSuccessHook();
          const apiCallResult = [['fakeResult']];
          const hookResult = hook('getDevices', apiCallResult);
          assertObjectEquals([[]], hookResult);
        });
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_ChangeToSession', makeTest(
      USER_PROFILE_TYPE, IN_LOCK_SCREEN_STATE,
      function(propertyReplacer) {
        const loginStateHook = new GSC.Libusb.ChromeLoginStateHook();
        return loginStateHook.getHookReadyPromise().then(() => {
          changeSessionState(IN_SESSION_STATE, propertyReplacer);
          const hook = loginStateHook.getRequestSuccessHook();
          const apiCallResult = [['fakeResult']];
          const hookResult = hook('getDevices', apiCallResult);
          assertObjectEquals([['fakeResult']], hookResult);
        });
      }
    ));

});  // goog.scope
