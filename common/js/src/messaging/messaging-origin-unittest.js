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

goog.require('GoogleSmartCard.MessagingOrigin');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const getFromChromeMessageSender =
    GSC.MessagingOrigin.getFromChromeMessageSender;
const getFromExtensionId = GSC.MessagingOrigin.getFromExtensionId;
const extractExtensionId = GSC.MessagingOrigin.extractExtensionId;

goog.exportSymbol('testGetFromChromeMessageSender', function() {
  assertNull(getFromChromeMessageSender(undefined));

  const messageSender = /** @type {!MessageSender} */ ({});
  assertNull(getFromChromeMessageSender(messageSender));

  messageSender.id = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
  assertEquals(
      getFromChromeMessageSender(messageSender),
      'chrome-extension://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa');

  delete messageSender.id;
  messageSender.url = 'https://example.org/index.html';
  assertEquals(
      getFromChromeMessageSender(messageSender), 'https://example.org');

  messageSender.url = 'http://example.org:8080/#foo=bar';
  assertEquals(
      getFromChromeMessageSender(messageSender), 'http://example.org:8080');

  messageSender.url = 'abacaba';
  assertNull(getFromChromeMessageSender(messageSender));
});

goog.exportSymbol('testGetFromExtensionId', function() {
  assertEquals(
      getFromExtensionId('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'),
      'chrome-extension://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa');
});

goog.exportSymbol('testExtractExtensionId', function() {
  assertEquals(
      extractExtensionId('chrome-extension://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'),
      'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa');

  assertNull(extractExtensionId('https://example.org'));
  assertNull(extractExtensionId('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'));
  assertNull(extractExtensionId('https://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'));
});
});  // goog.scope
