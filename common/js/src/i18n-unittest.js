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
    case 'someString':
      return 'Some string.';
    case 'anotherString':
      return 'Another string.';
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
    pNode1.dataset.i18n = 'someString';
    pNode1.dataset.i18nAriaLabel = 'anotherString';
    pNode2.dataset.i18nAriaLabel = 'someString';
    spanNode.dataset.i18n = 'anotherString';

    GSC.I18n.adjustAllElementsTranslation();

    assertEquals(pNode1.textContent, 'Some string.');
    assertEquals(pNode1.getAttribute('aria-label'), 'Another string.');
    assertEquals(pNode2.textContent, '');
    assertEquals(pNode2.getAttribute('aria-label'), 'Some string.');
    assertEquals(spanNode.textContent, 'Another string.');
    assertFalse(spanNode.hasAttribute('aria-label'));
  },
});

});  // goog.scope
