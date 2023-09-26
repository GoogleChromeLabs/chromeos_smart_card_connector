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

goog.provide('GoogleSmartCard.TestingConstants');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

// Corresponds to `IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE` in the CCID
// sources.
/** @const */
GSC.TestingConstants.IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE = 0x42000001;

// Corresponds to `IOCTL_FEATURE_IFD_PIN_PROPERTIES` in the CCID sources.
/** @const */
GSC.TestingConstants.IOCTL_FEATURE_IFD_PIN_PROPERTIES = 0x4233000A;
});
