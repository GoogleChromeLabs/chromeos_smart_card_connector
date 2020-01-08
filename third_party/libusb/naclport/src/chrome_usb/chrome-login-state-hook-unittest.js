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

goog.require('GoogleSmartCard.Libusb.ChromeLoginStateHook');
goog.require('GoogleSmartCard.Libusb.ChromeUsbBackend');
goog.require('GoogleSmartCard.RemoteCallMessage');
goog.require('GoogleSmartCard.Requester');
goog.require('GoogleSmartCard.RequestReceiver');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('goog.Promise');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var userProfileType = 1;
/** @const */
var signInProfileType = 2;
/** @const */
var inSessionState = 1;
/** @const */
var inLockScreenState = 2;

var sessionStateListeners = [];

/**
 * Sets up mocks for the chrome.loginState Extensions API.
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 * @param {number} fakeProfileType Fake data to be 
 * returned from getProfileType().
 * @param {number} fakeSessionState Fake data to be 
 * returned from getSessionState().
 */
function setUpChromeLoginStateMock(
    propertyReplacer, fakeProfileType, fakeSessionState) {
  propertyReplacer.set(
      chrome, 'loginState',
      {getProfileType: function(callback) { callback(fakeProfileType); },
      getSessionState: function(callback) { callback(fakeSessionState); },
      onSessionStateChanged: {
          addListener: function(callback) {
            sessionStateListeners.push(callback);
          }
      },
      ProfileType: {
        USER_PROFILE: userProfileType,
        SIGNIN_PROFILE: signInProfileType
      },
      SessionState: {
        IN_SESSION: inSessionState,
        IN_LOCK_SCREEN: inLockScreenState
      }});
}

/**
 * Wraps the given test function, providing the necessary setup and teardown.
 * @param {number} fakeProfileType Fake data to be 
 * returned from chrome.loginState.getProfileType().
 * @param {number} fakeSessionState Fake data to be 
 * returned from chrome.loginState.getSessionState().
 * @param {function(!GSC.Libusb.ChromeLoginStateHook)} testCallback
 * The test function to be run after the needed setup.
 * @return {function()} The wrapped test function.
 */
function makeTest(fakeProfileType, fakeSessionState, testCallback) {
  return function() {
    var propertyReplacer = new goog.testing.PropertyReplacer;

    function cleanup() {
      propertyReplacer.reset();
      sessionStateListeners = [];
    }
    
    setUpChromeLoginStateMock(
        propertyReplacer, fakeProfileType, fakeSessionState);

    /** @preserveTry */
    try {
      testCallback(new GSC.Libusb.ChromeLoginStateHook());
    } finally {
      cleanup();
    }
  };
}

/**
 * Notifies listeners on chrome.loginState.sessionStateChanged with the new
 * session state. Does not change the value returned by 
 * chrome.loginState.getSessionState() !
 * @param {number} fakeNewSessionState
 */
function notifySessionStateListeners(fakeNewSessionState) {
  goog.array.forEach(sessionStateListeners, function(listener) {
      listener(fakeNewSessionState);
  });
}

goog.exportSymbol(
    'test_ChromeLoginStateHook_DoesNotFilterInSession', makeTest(
      userProfileType, inSessionState,
      function(loginStateHook) {
        var hook = loginStateHook.getRequestSuccessHook();
        var apiCallResult = ['fakeResult'];
        hook('getDevices', apiCallResult);
        assertObjectEquals(['fakeResult'], apiCallResult);
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_FiltersInLockScreen', makeTest(
      userProfileType, inLockScreenState,
      function(loginStateHook) {
        var hook = loginStateHook.getRequestSuccessHook();

        var apiCallResult = ['fakeResult'];
        hook('getDevices', apiCallResult);
        assertObjectEquals([], apiCallResult);

        apiCallResult = ['fakeResult'];
        hook('getConfigurations', apiCallResult);
        assertObjectEquals([], apiCallResult);

        //only getDevices and getConfigurations should get filtered
        apiCallResult = ['fakeResult'];
        hook('listInterfaces', apiCallResult);
        assertObjectEquals(['fakeResult'], apiCallResult);
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_DoesNotFilterForSignInProfile', makeTest(
      signInProfileType, inSessionState,
      function(loginStateHook) {
        var hook = loginStateHook.getRequestSuccessHook();
        var apiCallResult = ['fakeResult'];
        hook('getDevices', apiCallResult);
        assertObjectEquals(['fakeResult'], apiCallResult);
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_ChangeToLockScreen', makeTest(
      userProfileType, inSessionState,
      function(loginStateHook) {
        notifySessionStateListeners(inLockScreenState);
        var hook = loginStateHook.getRequestSuccessHook();
        var apiCallResult = ['fakeResult'];
        hook('getDevices', apiCallResult);
        assertObjectEquals([], apiCallResult);
      }
    ));

goog.exportSymbol(
    'test_ChromeLoginStateHook_ChangeToSession', makeTest(
      userProfileType, inLockScreenState,
      function(loginStateHook) {
        notifySessionStateListeners(inSessionState);
        var hook = loginStateHook.getRequestSuccessHook();
        var apiCallResult = ['fakeResult'];
        hook('getDevices', apiCallResult);
        assertObjectEquals(['fakeResult'], apiCallResult);
      }
    ));

});  // goog.scope
