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

goog.require('GoogleSmartCard.LibusbToWebusbAdaptor');
goog.require('goog.object');
goog.require('goog.testing.PropertyReplacer');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const deepClone = goog.object.unsafeClone;

const propertyReplacer = new goog.testing.PropertyReplacer();
let libusbToWebusbAdaptor;

const FAILING_ASYNC_METHOD = async () => {
  fail('Unexpected method call');
};
const FAKE_WEBUSB_INTERFACE = {
  'interfaceNumber': 12,
  'alternates': [{
    'interfaceClass': 0x56,
    'interfaceSubclass': 0x78,
    'interfaceProtocol': 0xAB,
    'endpoints': [
      {
        'endpointNumber': 1,
        'direction': 'in',
        'type': 'bulk',
        'packetSize': 100,
      },
      {
        'endpointNumber': 0x2,
        'direction': 'out',
        'type': 'interrupt',
        'packetSize': 100,
      },
    ]
  }]
};
const FAKE_WEBUSB_OTHER_INTERFACE = {
  'interfaceNumber': 21,
  'alternates': [{
    'interfaceClass': 0x65,
    'interfaceSubclass': 0x87,
    'interfaceProtocol': 0xBA,
    'endpoints': [
      {
        'endpointNumber': 0x3,
        'direction': 'out',
        'type': 'isochronous',
        'packetSize': 1000,
      },
    ]
  }]
};
const FAKE_WEBUSB_CONFIGURATION = {
  'configurationValue': 0x12,
  'interfaces': [FAKE_WEBUSB_INTERFACE, FAKE_WEBUSB_OTHER_INTERFACE]
};
const FAKE_WEBUSB_DEVICE = {
  'vendorId': 0x1234,
  'productId': 0xfedc,
  'deviceVersionMajor': 1,
  'deviceVersionMinor': 2,
  'deviceVersionSubminor': 3,
  'productName': 'some name',
  'manufacturerName': 'some company',
  'serialNumber': '98765',
  'configuration': deepClone(
      FAKE_WEBUSB_CONFIGURATION),  // this follows Chrome's WebUSB behavior,
                                   // which gives different objects in
                                   // configuration and configurations
  'configurations': [FAKE_WEBUSB_CONFIGURATION],
  'open': FAILING_ASYNC_METHOD,
  'close': FAILING_ASYNC_METHOD,
};
const FAKE_WEBUSB_OTHER_DEVICE = {
  'vendorId': 0x4321,
  'productId': 0xcdef,
  'deviceVersionMajor': 1,
  'deviceVersionMinor': 0,
  'deviceVersionSubminor': 0,
  'configurations': [],
  'open': FAILING_ASYNC_METHOD,
  'close': FAILING_ASYNC_METHOD,
};

goog.exportSymbol('testLibusbToWebusbAdaptor', {
  'setUp': function() {
    libusbToWebusbAdaptor = new GSC.LibusbToWebusbAdaptor();
    // We're not replacing the `navigator.usb` object, since Chrome throws an
    // exception if the code is in the 'use strict' mode.
  },

  'tearDown': function() {
    propertyReplacer.reset();
  },

  // Test the `listDevices()` method when there's no device.
  'testListDevices_empty': async function() {
    // Arrange:
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [];
    });

    // Act:
    const devices = await libusbToWebusbAdaptor.listDevices();

    // Assert:
    assertObjectEquals([], devices);
  },

  // Test the `listDevices()` method when there are devices available.
  'testListDevices_two': async function() {
    // Arrange:
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [
        FAKE_WEBUSB_DEVICE,
        FAKE_WEBUSB_OTHER_DEVICE,
      ];
    });

    // Act:
    const devices = await libusbToWebusbAdaptor.listDevices();

    // Assert:
    assertObjectEquals(
        [
          {
            'deviceId': 1,
            'vendorId': 0x1234,
            'productId': 0xfedc,
            'version': 1 * 256 + 2 * 16 + 3,
            'productName': 'some name',
            'manufacturerName': 'some company',
            'serialNumber': '98765',
          },
          {
            'deviceId': 2,
            'vendorId': 0x4321,
            'productId': 0xcdef,
            'version': 256,
          }
        ],
        devices);
  },

  // Test the `listDevices()` method when a device gets added dynamically.
  'testListDevices_appears': async function() {
    // Arrange:
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [];
    });

    // Act:
    await libusbToWebusbAdaptor.listDevices();
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [FAKE_WEBUSB_DEVICE];
    });
    const devices = await libusbToWebusbAdaptor.listDevices();

    // Assert:
    assertEquals(1, devices.length);
    assertEquals(1, devices[0]['deviceId']);
  },

  // Test the `listDevices()` method when the device gets removed.
  'testListDevices_disappears': async function() {
    // Arrange:
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [FAKE_WEBUSB_DEVICE];
    });

    // Act:
    await libusbToWebusbAdaptor.listDevices();
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [];
    });
    const devices = await libusbToWebusbAdaptor.listDevices();

    // Assert:
    assertEquals(0, devices.length);
  },

  // Test the `listDevices()` method that the deviceId's aren't reused between
  // different devices.
  'testListDevices_deviceIds': async function() {
    // Arrange:
    // Add two devices.
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [FAKE_WEBUSB_DEVICE, FAKE_WEBUSB_OTHER_DEVICE];
    });

    // Act:
    await libusbToWebusbAdaptor.listDevices();
    // Remove both devices.
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [];
    });
    await libusbToWebusbAdaptor.listDevices();
    // Add two devices again (they're treated by the tested code as new ones).
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [FAKE_WEBUSB_DEVICE, FAKE_WEBUSB_OTHER_DEVICE];
    });
    const devices = await libusbToWebusbAdaptor.listDevices();

    // Assert:
    assertEquals(2, devices.length);
    assertEquals(3, devices[0]['deviceId']);
    assertEquals(4, devices[1]['deviceId']);
  },

  // Test the `getConfigurations()` method when the device has no
  // configurations.
  'testGetConfigurations_successEmpty': async function() {
    // Arrange:
    const device = deepClone(FAKE_WEBUSB_DEVICE);
    // Clear the fake device's
    device['configurations'] = [];
    delete device['configuration'];
    device['open'] = async () => {
      // In this test we don't want to check the extraData parsing
      // functionality, so suppress it by aborting open() calls.
      throw new Error('Intentional error');
    };
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [device];
    });

    // Act:
    await libusbToWebusbAdaptor.listDevices();
    const configurations =
        await libusbToWebusbAdaptor.getConfigurations(/*deviceId=*/ 1);

    // Assert:
    assertObjectEquals([], configurations);
  },

  // Test the `getConfigurations()` method when the device has one
  // configuration, with a few interfaces and endpoints in it.
  'testGetConfigurations_successOne': async function() {
    // Arrange:
    const device = deepClone(FAKE_WEBUSB_DEVICE);
    device['open'] = async () => {
      // In this test we don't want to check the extraData parsing
      // functionality, so suppress it by aborting open() calls.
      throw new Error('Intentional error');
    };
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [device];
    });

    // Act:
    await libusbToWebusbAdaptor.listDevices();
    const configurations =
        await libusbToWebusbAdaptor.getConfigurations(/*deviceId=*/ 1);

    // Assert:
    console.log(JSON.stringify(configurations));
    assertObjectEquals(
        [{
          'active': true,
          'configurationValue': 0x12,
          'interfaces': [
            {
              'interfaceNumber': 12,
              'interfaceClass': 0x56,
              'interfaceSubclass': 0x78,
              'interfaceProtocol': 0xAB,
              'endpoints': [
                {
                  'endpointAddress': 1 + 128,
                  'direction': 'in',
                  'type': 'bulk',
                  'maxPacketSize': 100
                },
                {
                  'endpointAddress': 2,
                  'direction': 'out',
                  'type': 'interrupt',
                  'maxPacketSize': 100
                }
              ]
            },
            {
              'interfaceNumber': 21,
              'interfaceClass': 0x65,
              'interfaceSubclass': 0x87,
              'interfaceProtocol': 0xBA,
              'endpoints': [{
                'endpointAddress': 3,
                'direction': 'out',
                'type': 'isochronous',
                'maxPacketSize': 1000
              }]
            }
          ]
        }],
        configurations);
  },

});
});  // goog.scope
