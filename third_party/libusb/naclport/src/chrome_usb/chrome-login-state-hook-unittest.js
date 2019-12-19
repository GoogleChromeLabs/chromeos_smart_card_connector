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
var ChromeLoginStateHook = GSC.Libusb.ChromeLoginStateHook;
/** @const */
var RemoteCallMessage = GSC.RemoteCallMessage;
/** @const */
var ignoreArgument = goog.testing.mockmatchers.ignoreArgument;
/** @const */
var REQUESTER_NAME = 'libusb_chrome_usb';

/** @const */
var userProfileType = chrome.loginState.ProfileType.USER_PROFILE;
/** @const */
var signinProfileType = chrome.loginState.ProfileType.SIGNIN_PROFILE;
/** @const */
var inSessionState = chrome.loginState.SessionState.IN_SESSION;
/** @const */
var inLockScreenState = chrome.loginState.SessionState.IN_LOCK_SCREEN;


var sessionStateListeners = [];

/** @const */
var FAKE_DEVICES = [{"device": 0x00, "manufacturerName": "ACS", 
    "productId": 0x000090CC, "productName": "CCID USB Reader", 
    "serialNumber": "", "vendorId": 0x0000072F, "version": 0x00000100}];
/** @const */
/*
var FAKE_CONFIGS = [{"active": true, "configurationValue": 0x01, 
    "extra_data": ArrayBuffer, "interfaces": [
      {"alternateSetting": 0x00, "endpoints": [
        {"address": 0x81, "direction": "in", "extra_data": ArrayBuffer,
        "maximumPacketSize": 0x08, "pollingInterval": 0x10, 
        "type": "interrupt", "usage": "periodic"}, 
        {"address": 0x02, "direction": "out", "extra_data": ArrayBuffer, 
        "maximumPacketSize": 0x40, "pollingInterval": 0x00, "type": "bulk"}, 
        {"address": 0x82, "direction": "in", "extra_data": ArrayBuffer, 
        "maximumPacketSize": 0x40, "pollingInterval": 0x00, "type": "bulk"}],
      "extra_data": ArrayBuffer[0x36, 0x21, 0x00, 0x01, 0x00, 0x07, 0x03, 
        0x00, 0x00, 0x00, 0xA0, 0x0F, 0x00, 0x00, 0xA0, 0x0F, 0x00, 0x00, 
        0x00, 0x00, 0x2A, 0x00, 0x00, 0x24, 0x40, 0x05, 0x00, 0x00, 0xF7,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x30, 0x00, 0x01, 0x00, 0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x01], 
      "interfaceClass": 0x0B, "interfaceNumber": 0x00, 
      "interfaceProtocol": 0x00, "interfaceSubclass": 0x00}], 
    "maxPower": 0x32, "remoteWakeup": false, "selfPowered": false}];
    */
/** @const */
var FAKE_INTERFACES = [{'interface': 'fakeInterface'}];

/**
 * Sets up mocks for the chrome.usb Extensions API.
 * @param {!goog.testing.MockControl} mockControl
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 * @param {!Object} fakeDevices Fake data to be returned from getDevices().
 * @param {!Object} fakeConfigurations Fake data to be returned from
 * getConfigurations().
 * @param {!Object} fakeInterfaces Fake data to be returned from 
 * listInterfaces().
 */
function setUpChromeUsbMock(
    mockControl, propertyReplacer, fakeDevices, fakeConfigurations, 
    fakeInterfaces) {
  propertyReplacer.set(
      chrome, 'usb',
      {getDevices: mockControl.createFunctionMock('chrome.usb.getDevices'),
       getConfigurations: mockControl.createFunctionMock(
         'chrome.usb.getConfigurations'),
       listInterfaces: mockControl.createFunctionMock('chrome.usb.listInterfaces')});

  // Suppress compiler's signature verifications locally, to be able to use
  // mock-specific functionality.
  /** @type {?} */ chrome.usb.getDevices;
  /** @type {?} */ chrome.usb.getConfigurations;
  /** @type {?} */ chrome.usb.listInterfaces;

  chrome.usb.getDevices(ignoreArgument, ignoreArgument).$does(
      function(options, callback) {
        callback(fakeDevices);
      });

  chrome.usb.getConfigurations(ignoreArgument, ignoreArgument).$does(
      function(options, callback) {
        callback(fakeConfigurations);
      });

  chrome.usb.listInterfaces(ignoreArgument, ignoreArgument).$does(
      function(options, callback) {
        callback(fakeInterfaces);
      });
}

/**
 * Sets up mocks for the chrome.loginState Extensions API.
 * @param {!goog.testing.MockControl} mockControl
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 * @param {!chrome.loginState.ProfileType} fakeProfileType Fake data to be 
 * returned from getProfileType().
 * @param {!chrome.loginState.SessionState} fakeSessionState Fake data to be 
 * returned from getSessionState().
 */
function setUpChromeLoginStateMock(
    mockControl, propertyReplacer, fakeProfileType, fakeSessionState) {
  propertyReplacer.set(
      chrome, 'loginState',
      {getProfileType: mockControl.createFunctionMock(
        'chrome.loginState.getProfileType'),
      getSessionState: mockControl.createFunctionMock(
        'chrome.loginState.getSessionState'),
      onSessionStateChanged: {
          addListener: mockControl.createFunctionMock(
            'chrome.loginState.onSessionStateChanged.addListener')
      }});

  // Suppress compiler's signature verifications locally, to be able to use
  // mock-specific functionality.
  /** @type {?} */ chrome.loginState.getProfileType;
  /** @type {?} */ chrome.loginState.getSessionState;
  /** @type {?} */ chrome.loginState.onSessionStateChanged.addListener;

  chrome.loginState.getProfileType(ignoreArgument).$does(
      function(callback) {
        callback(fakeProfileType);
      });

  chrome.loginState.getSessionState(ignoreArgument).$does(
      function(callback) {
        callback(fakeSessionState);
      });

  chrome.loginState.onSessionStateChanged.addListener(ignoreArgument).$does(
      function(callback) {
        sessionStateListeners.push(callback);
      });
}

/**
 * Wraps the given test function, providing the necessary setup and teardown.
 * @param {!chrome.loginState.ProfileType} fakeProfileType Fake data to be 
 * returned from chrome.loginState.getProfileType().
 * @param {!chrome.loginState.SessionState} fakeSessionState Fake data to be 
 * returned from chrome.loginState.getSessionState().
 * @param {!Object} fakeDevices Fake data to be returned from
 * chrome.usb.getDevices().
 * @param {!Object} fakeConfigurations Fake data to be returned from
 * chrome.usb.getConfigurations().
 * @param {!Object} fakeInterfaces Fake data to be returned from 
 * chrome.usb.listInterfaces().
 * @param {function(!GSC.Requester):!goog.Promise} testCallback The test
 * function to be run after the needed setup; must return a promise of the test
 * result.
 * @return {function():!goog.Promise} The wrapped test function, which returns a
 * promise of the test result and the result of teardown.
 */
function makeTest(
  fakeProfileType, fakeSessionState, 
  fakeDevices, fakeConfigurations, fakeInterfaces, testCallback) {
  return function() {
    var mockControl = new goog.testing.MockControl;
    var propertyReplacer = new goog.testing.PropertyReplacer;

    function cleanup() {
      mockControl.$tearDown();
      mockControl.$resetAll();
      propertyReplacer.reset();
    }

    function verifyAndCleanup() {
      /** @preserveTry */
      try {
        mockControl.$verifyAll();
      } finally {
        // Cleanup regardless of the mock verification outcome.
        cleanup();
      }
    }
    
    setUpChromeUsbMock(
        mockControl, propertyReplacer,
        fakeDevices, fakeConfigurations, fakeInterfaces);
    setUpChromeLoginStateMock(
        mockControl, propertyReplacer, fakeProfileType, fakeSessionState);

    mockControl.$replayAll();

    var channel = new GSC.SingleMessageBasedChannel('loginStateHookTest');
    var requester = new GSC.Requester(REQUESTER_NAME, channel);
    var libusbChromeUsbBackend = 
        new GSC.Libusb.ChromeUsbBackend(channel);
    new GSC.Libusb.ChromeLoginStateHook(libusbChromeUsbBackend);

    /** @preserveTry */
    try {
      var testPromise = testCallback(requester);
    } catch (exc) {
      // Cleanup after the test fatally failed synchronously.
      cleanup();
      throw exc;
    }
    // Verify mocks and cleanup after the test completes or fails. Note that
    // it's crucial that the returned promise is a child promise which is tied
    // with the success of verification and cleanup.
    return testPromise.then(verifyAndCleanup, verifyAndCleanup);
  };
}

/**
 * Notifies listeners on chrome.loginState.sessionStateChanged with the new
 * session state. Does not change the value returned by 
 * chrome.loginState.getSessionState() !
 * @param {!chrome.loginState.SessionState} fakeNewSessionState
 */
function notifySessionStateListeners(fakeNewSessionState) {
  goog.array.forEach(sessionStateListeners, function(listener) {
      listener(fakeNewSessionState);
      });
}

/**
 * Calls a chrome.usb function through libusbChromeUsbBackend.
 * @param {!GSC.Requester} requester
 * @param {string} functionName
 * @param {!Array.<*>} functionArguments
 * @return {!goog.Promise}
 */
function sendRequest(requester, functionName, functionArguments) {
  var remoteCallMessage = new RemoteCallMessage(functionName, functionArguments);
  return requester.postRequest(remoteCallMessage.makeRequestPayload());
}

// try to call the private handleRequest_ on the chrome usb backend directly
// in that case - also do not use the message channel
// if not possible - look more at channels

goog.exportSymbol('test_ChromeLoginStateHook', function() {
    assertObjectEquals({}, {});
    //With filtering on:
      //should filter getDevices
      //should filter getConfigurations
      //should not filter listInterfaces
    //With filtering off:
      //should not filter anything
    //should register hook for user profile
    //should not register hook for non-user profile
    //session starts in lock screen
    //session starts logged in
  });

goog.exportSymbol(
    'test_ChromeLoginStateHook_UserProfileRegistersHook', makeTest(
      userProfileType, inSessionState, {}, {}, {}, 
      function(requester) {
        var testPromise = goog.Promise.withResolver();
        var promise = sendRequest(requester, 'getDevices', [{}]);
        promise.then(function(resultArgs) {
          assertObjectEquals(resultArgs, [[]]);
          testPromise.resolve();
        },function (error) {testPromise.reject(error);});
        return testPromise.promise;
      }
    ));

});  // goog.scope
