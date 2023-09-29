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

goog.provide('GoogleSmartCard.TestingLibusbSmartCardSimulationConstants');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @const */
GSC.TestingLibusbSmartCardSimulationConstants.CPP_HELPER_NAME =
    'SmartCardConnectorApplicationTestHelper';

// Names of the device types defined by TestingSmartCardSimulation.
// These strings need to match the string description of DeviceType values
// defined in `testing_smart_card_simulation.cc`.
/** @const */
GSC.TestingLibusbSmartCardSimulationConstants.GEMALTO_DEVICE_TYPE =
    'gemaltoPcTwinReader';
/** @const */
GSC.TestingLibusbSmartCardSimulationConstants.DELL_DEVICE_TYPE =
    'dellSmartCardReaderKeyboard';

// Name of the card type defined by TestingSmartCardSimulation.
// This string needs to match the string description of CardType values defined
// in `testing_smart_card_simulation.cc`.
/** @const */
GSC.TestingLibusbSmartCardSimulationConstants.COSMO_CARD_TYPE = 'cosmoId70';

// Name of the card profile type defined by TestingSmartCardSimulation.
// This string needs to match the string description of CardProfile values
// defined in `testing_smart_card_simulation.cc`.
/** @const */
GSC.TestingLibusbSmartCardSimulationConstants.CHARISMATHICS_PIV_TYPE =
    'charismathicsPiv';

// Names of simulated smart card readers as they appear in the PC/SC-Lite API.
// The "0" suffix corresponds to the "00 00" part that contains nonzeroes in
// case there are multiple devices with the same name.
/** @const */
GSC.TestingLibusbSmartCardSimulationConstants
    .GEMALTO_PC_TWIN_READER_PCSC_NAME0 = 'Gemalto PC Twin Reader 00 00';

// Card ATRs. The values are mirrored from testing_smart_card_simulation.cc.
/** @const */
GSC.TestingLibusbSmartCardSimulationConstants.COSMO_ID_70_ATR =
    (new Uint8Array([
      0x3B, 0xDB, 0x96, 0x00, 0x80, 0xB1, 0xFE, 0x45, 0x1F, 0x83, 0x00,
      0x31, 0xC0, 0x64, 0xC7, 0xFC, 0x10, 0x00, 0x01, 0x90, 0x00, 0x74,
    ])).buffer;

// SELECT command (the format is per NIST 800-73-4).
/**@const */
GSC.TestingLibusbSmartCardSimulationConstants.SELECT_COMMAND =
    new Uint8Array([
      0x00, 0xA4, 0x04, 0x00, 0x09, 0xA0, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00,
      0x10, 0x00, 0x00
    ]).buffer;

// Hardcoded application identifier that testing_smart_card_simulation.cc uses
// for the CardProfile::kCharismathicsPiv.
/**@const */
GSC.TestingLibusbSmartCardSimulationConstants
    .APPLICATION_IDENTIFIER_CHARISMATHICS_PIV = new Uint8Array([
  0x61, 0x5C, 0x4F, 0x0B, 0xA0, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x10,
  0x00, 0x01, 0x00, 0x79, 0x07, 0x4F, 0x05, 0xA0, 0x00, 0x00, 0x03, 0x08,
  0x50, 0x27, 0x50, 0x65, 0x72, 0x73, 0x6F, 0x6E, 0x61, 0x6C, 0x5F, 0x49,
  0x64, 0x65, 0x6E, 0x74, 0x69, 0x74, 0x79, 0x5F, 0x61, 0x6E, 0x64, 0x5F,
  0x56, 0x65, 0x72, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E,
  0x5F, 0x43, 0x61, 0x72, 0x64, 0x5F, 0x50, 0x1A, 0x68, 0x74, 0x74, 0x70,
  0x3A, 0x2F, 0x2F, 0x63, 0x73, 0x72, 0x63, 0x2E, 0x6E, 0x69, 0x73, 0x74,
  0x2E, 0x67, 0x6F, 0x76, 0x2F, 0x6E, 0x70, 0x69, 0x76, 0x70
]);

// The `PIN_PROPERTIES_STRUCTURE` struct as encoded blob, with the value
// that's expected for a standard reader.
/** @const */
GSC.TestingLibusbSmartCardSimulationConstants.PIN_PROPERTIES_STRUCTURE =
    new Uint8Array([0x00, 0x00, 0x07, 0x00]).buffer;
});
