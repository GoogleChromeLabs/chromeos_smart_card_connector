/** @license
 * Copyright 2020 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.I18n');
goog.require('goog.dom');
goog.require('goog.testing');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const propertyReplacer = new goog.testing.PropertyReplacer;
/** @type {Element?} */
let pNode1;
/** @type {Element?} */
let pNode2;
/** @type {Element?} */
let divNode;
/** @type {Element?} */
let spanNode;

/**
 * Stub implementation of the chrome.i18n.getMessage() Extensions API.
 * @param {string} messageName
 * @return {string}
 */
function stubChromeI18nGetMessage(messageName) {
  switch (messageName) {
    case 'firstString':
      return 'First string.';
    case 'secondString':
      return 'Second string.';
    case 'thirdString':
      return 'Third string.';
    default:
      fail(`Unexpected messageName=${messageName}`);
      return '';
  }
}

function createTestElements() {
  pNode1 = document.createElement('p');
  document.body.appendChild(pNode1);

  pNode2 = document.createElement('p');
  document.body.appendChild(pNode2);

  divNode = document.createElement('div');
  document.body.appendChild(divNode);

  spanNode = document.createElement('span');
  divNode.appendChild(spanNode);
}

function removeTestElements() {
  for (const element of [pNode1, pNode2, divNode, spanNode])
    goog.dom.removeNode(element);
}

function checkPNode1Translated() {
  assertEquals(pNode1.textContent, 'First string.');
  assertEquals(pNode1.getAttribute('aria-label'), 'Second string.');
  assertFalse(pNode1.hasAttribute('title'));
}

function checkPNode2Translated() {
  assertEquals(pNode2.textContent, '');
  assertEquals(pNode2.getAttribute('aria-label'), 'Second string.');
  assertEquals(pNode2.getAttribute('title'), 'Third string.');
}

function checkSpanNodeTranslated() {
  assertEquals(spanNode.textContent, 'First string.');
  assertFalse(spanNode.hasAttribute('aria-label'));
  assertEquals(spanNode.getAttribute('title'), 'Third string.');
}

function checkNodeNotTranslated(element) {
  assertEquals(element.textContent, '');
  assertFalse(element.hasAttribute('aria-label'));
  assertFalse(element.hasAttribute('title'));
}

goog.exportSymbol('testI18n', {
  setUp: function() {
    createTestElements();
    propertyReplacer.set(
        chrome, 'i18n', {getMessage: stubChromeI18nGetMessage});
  },

  tearDown: function() {
    propertyReplacer.reset();
    removeTestElements();
  },

  testBulkTranslation: function() {
    pNode1.dataset.i18n = 'firstString';
    pNode1.dataset.i18nAriaLabel = 'secondString';
    pNode2.dataset.i18nAriaLabel = 'secondString';
    pNode2.dataset.title = 'thirdString';
    spanNode.dataset.i18n = 'firstString';
    spanNode.dataset.title = 'thirdString';

    GSC.I18n.adjustAllElementsTranslation();

    checkPNode1Translated();
    checkPNode2Translated();
    checkSpanNodeTranslated();
  },

  testElementTranslation: function() {
    pNode1.dataset.i18n = 'firstString';
    pNode1.dataset.i18nAriaLabel = 'secondString';
    pNode2.dataset.i18nAriaLabel = 'secondString';
    pNode2.dataset.title = 'thirdString';
    spanNode.dataset.i18n = 'firstString';
    spanNode.dataset.title = 'thirdString';

    GSC.I18n.adjustElementTranslation(pNode1);

    checkPNode1Translated();
    checkNodeNotTranslated(pNode2);
    checkNodeNotTranslated(spanNode);

    GSC.I18n.adjustElementTranslation(pNode2);

    checkPNode1Translated();
    checkPNode2Translated();
    checkNodeNotTranslated(spanNode);

    GSC.I18n.adjustElementTranslation(spanNode);

    checkPNode1Translated();
    checkPNode2Translated();
    checkSpanNodeTranslated();
  },
});

});  // goog.scope
