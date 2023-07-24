/**
 * @license
 * Copyright 2023 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.LibusbProxyDataModel');
goog.require('GoogleSmartCard.LibusbToJsApiAdaptor');
goog.require('GoogleSmartCard.SmartCardFilterLibusbHook');
goog.require('goog.asserts');
goog.require('goog.testing');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const LibusbJsConfigurationDescriptor =
    GSC.LibusbProxyDataModel.LibusbJsConfigurationDescriptor;

const FAKE_DEVICE_ID = 123;

class FakeLibusbProxyHookDelegate extends GSC.LibusbToJsApiAdaptor {
  constructor() {
    super();
    /** @type {!Array<!LibusbJsConfigurationDescriptor>} */
    this.fakeConfigurations_ = [];
  }

  /**
   * @param {!Array<!LibusbJsConfigurationDescriptor>} fakeConfigurations
   */
  setFakeConfigurations(fakeConfigurations) {
    this.fakeConfigurations_ = fakeConfigurations;
  }

  /** @override */
  async listDevices() {
    fail('Unexpected call');
    goog.asserts.fail();
  }
  /** @override */
  async getConfigurations(deviceId) {
    return this.fakeConfigurations_;
  }
  /** @override */
  async openDeviceHandle(deviceId) {
    fail('Unexpected call');
    goog.asserts.fail();
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
};

/** @type {!FakeLibusbProxyHookDelegate|undefined} */
let fakeDelegate;
/** @type {!GSC.SmartCardFilterLibusbHook|undefined} */
let hook;

goog.exportSymbol('test_SmartCardFilterLibusbHook', {
  'setUp': function() {
    fakeDelegate = new FakeLibusbProxyHookDelegate();
    hook = new GSC.SmartCardFilterLibusbHook();
    hook.setDelegate(fakeDelegate);
  },

  'tearDown': function() {
    hook = undefined;
    fakeDelegate = undefined;
  },

  'testEmpty': async function() {
    const got = await hook.getConfigurations(FAKE_DEVICE_ID);

    assertObjectEquals([], got);
  },

  // A simple device with the Smart Card USB interface class is passed as-is.
  'testSingleInterface_SmartCardClass': async function() {
    const CONFIGS = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [{
        'interfaceNumber': 123,
        'interfaceClass': 0xB,
        'interfaceSubclass': 0,
        'interfaceProtocol': 0,
        'endpoints': [
          {
            'endpointNumber': 1,
            'direction': 'out',
            'type': 'bulk',
            'packetSize': 64,
          },
        ],
      }],
    }];
    fakeDelegate.setFakeConfigurations(CONFIGS);

    const got = await hook.getConfigurations(FAKE_DEVICE_ID);

    assertObjectEquals(CONFIGS, got);
  },

  // A simple device with an unexpected USB interface class is filtered out.
  'testSingleInterface_OtherClass': async function() {
    const CONFIGS = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [{
        'interfaceNumber': 123,
        'interfaceClass': 0x3,
        'interfaceSubclass': 0,
        'interfaceProtocol': 0,
        'endpoints': [
          {
            'endpointNumber': 1,
            'direction': 'out',
            'type': 'bulk',
            'packetSize': 64,
          },
        ],
      }],
    }];
    fakeDelegate.setFakeConfigurations(CONFIGS);

    const got = await hook.getConfigurations(FAKE_DEVICE_ID);

    const EXPECTED_RESULT = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [],
    }];
    assertObjectEquals(EXPECTED_RESULT, got);
  },

  // A simple device with the Vendor Specific USB interface class and the
  // specific "extra data" (54-byte long, conforming to the CCID specs) is
  // passed as-is.
  'testSingleInterface_VendorClassLikeSmartCard': async function() {
    const CONFIGS = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [{
        'interfaceNumber': 123,
        'interfaceClass': 0xFF,
        'interfaceSubclass': 0,
        'interfaceProtocol': 0,
        'extraData': new ArrayBuffer(54),
        'endpoints': [
          {
            'endpointNumber': 1,
            'direction': 'out',
            'type': 'bulk',
            'packetSize': 64,
          },
        ],
      }],
    }];
    fakeDelegate.setFakeConfigurations(CONFIGS);

    const got = await hook.getConfigurations(FAKE_DEVICE_ID);

    assertObjectEquals(CONFIGS, got);
  },

  // A simple device with the Vendor Specific USB interface class and without
  // "extra data" is filtered out.
  'testSingleInterface_VendorClassWithoutExtraData': async function() {
    // Unlike the test above, the "extraData" key is missing.
    const CONFIGS = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [{
        'interfaceNumber': 123,
        'interfaceClass': 0xFF,
        'interfaceSubclass': 0,
        'interfaceProtocol': 0,
        'endpoints': [
          {
            'endpointNumber': 1,
            'direction': 'out',
            'type': 'bulk',
            'packetSize': 64,
          },
        ],
      }],
    }];
    fakeDelegate.setFakeConfigurations(CONFIGS);

    const got = await hook.getConfigurations(FAKE_DEVICE_ID);

    const EXPECTED_RESULT = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [],
    }];
    assertObjectEquals(EXPECTED_RESULT, got);
  },

  // A simple device with the Vendor Specific USB interface class and with
  // "extra data" of unexpected length is filtered out.
  'testSingleInterface_VendorClassDifferentExtraDataLength': async function() {
    // Unlike the successful test above, the "extraData" key is missing.
    const CONFIGS = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [{
        'interfaceNumber': 123,
        'interfaceClass': 0xFF,
        'interfaceSubclass': 0,
        'interfaceProtocol': 0,
        'extraData': new ArrayBuffer(10),
        'endpoints': [
          {
            'endpointNumber': 1,
            'direction': 'out',
            'type': 'bulk',
            'packetSize': 64,
          },
        ],
      }],
    }];
    fakeDelegate.setFakeConfigurations(CONFIGS);

    const got = await hook.getConfigurations(FAKE_DEVICE_ID);

    const EXPECTED_RESULT = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [],
    }];
    assertObjectEquals(EXPECTED_RESULT, got);
  },

  // A composite device with both interfaces (one having the Smart Card class
  // and another having a different class) is passed only in half.
  'testSingleInterface_Composite_SmartCardAndOtherClass': async function() {
    const SMART_CARD_INTERFACE = {
      'interfaceNumber': 123,
      'interfaceClass': 0xB,
      'interfaceSubclass': 0,
      'interfaceProtocol': 0,
      'endpoints': [
        {
          'endpointNumber': 1,
          'direction': 'out',
          'type': 'bulk',
          'packetSize': 64,
        },
      ],
    };
    const HUMAN_INTERFACE_DEVICE_INTERFACE = {
      'interfaceNumber': 234,
      'interfaceClass': 0x3,
      'interfaceSubclass': 0,
      'interfaceProtocol': 0,
      'endpoints': [
        {
          'endpointNumber': 1,
          'direction': 'out',
          'type': 'bulk',
          'packetSize': 64,
        },
      ],
    };
    const CONFIGS = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [SMART_CARD_INTERFACE, HUMAN_INTERFACE_DEVICE_INTERFACE],
    }];
    fakeDelegate.setFakeConfigurations(CONFIGS);

    const got = await hook.getConfigurations(FAKE_DEVICE_ID);

    const EXPECTED_RESULT = [{
      'active': true,
      'configurationValue': 1,
      'interfaces': [SMART_CARD_INTERFACE],
    }];
    assertObjectEquals(EXPECTED_RESULT, got);
  },
});
});
