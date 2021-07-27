/**
 * @license
 * Copyright 2016 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.FixedSizeInteger');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const castToInt32 = GSC.FixedSizeInteger.castToInt32;

goog.exportSymbol('testCastToInt32', function() {
  assertEquals(0, castToInt32(0));
  assertEquals(1, castToInt32(1));
  assertEquals(-1, castToInt32(-1));
  assertEquals(-1, castToInt32(0xFFFFFFFF));
  assertEquals(0x7FFFFFFF, castToInt32(0x7FFFFFFF));
  assertEquals(-0x80000000, castToInt32(0x80000000));
  assertEquals(-0x80000000, castToInt32(-0x80000000));
  assertEquals(-0x7FFFFFFF, castToInt32(0x80000001));
});
});  // goog.scope
