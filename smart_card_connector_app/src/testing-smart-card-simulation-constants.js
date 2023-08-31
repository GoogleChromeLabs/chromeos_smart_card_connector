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
});
