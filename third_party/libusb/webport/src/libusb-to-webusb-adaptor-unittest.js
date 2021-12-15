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
goog.require('goog.testing.PropertyReplacer');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const propertyReplacer = new goog.testing.PropertyReplacer();
let libusbToWebusbAdaptor;

goog.exportSymbol('testLibusbToWebusbAdaptor', {
  'setUp': function() {
    libusbToWebusbAdaptor = new GSC.LibusbToWebusbAdaptor();
    // We're not replacing the `navigator.usb` object, since Chrome throws an
    // exception if the code is in the 'use strict' mode.
  },

  'tearDown': function() {
    propertyReplacer.reset();
  },

  'testListDevices_empty': async function() {
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [];
    });
    assertObjectEquals(await libusbToWebusbAdaptor.listDevices(), []);
  },

  'testListDevices_two': async function() {
    propertyReplacer.set(navigator['usb'], 'getDevices', async () => {
      return [
        {
          'vendorId': 0x1234,
          'productId': 0xfedc,
          'deviceVersionMajor': 1,
          'deviceVersionMinor': 2,
          'deviceVersionSubminor': 3,
        },
        {
          'vendorId': 0x4321,
          'productId': 0xcdef,
          'deviceVersionMajor': 1,
          'deviceVersionMinor': 0,
          'deviceVersionSubminor': 0,
          'productName': 'some name',
          'manufacturerName': 'some company',
          'serialNumber': '98765',
        }
      ];
    });
    assertObjectEquals(await libusbToWebusbAdaptor.listDevices(), [
      {
        'deviceId': 1,
        'vendorId': 0x1234,
        'productId': 0xfedc,
        'version': 1 * 256 + 2 * 16 + 3,
      },
      {
        'deviceId': 2,
        'vendorId': 0x4321,
        'productId': 0xcdef,
        'version': 256,
        'productName': 'some name',
        'manufacturerName': 'some company',
        'serialNumber': '98765',
      }
    ]);
  },

});
});  // goog.scope
