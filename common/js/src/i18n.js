/** @license
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

/**
 * @fileoverview This file contains common functionality for
 * internationalization.
 */

goog.provide('GoogleSmartCard.I18n');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.dom');
goog.require('goog.log.Logger');

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/**
 * @type {!goog.log.Logger}
 * @const
 */
var logger = GSC.Logging.getScopedLogger('I18n');

/** @const */
var I18N_DATA_ATTRIBUTE = 'data-i18n';

/** @const */
var I18N_DATA_ARIA_LABEL_ATTRIBUTE = 'data-i18n-aria-label';

/**
 * @param {!Element} element
 * @param {string} attribute
 * @param {function(!Element,string)} transformFunction
 */
function transformElement(element, attribute, transformFunction) {
  var i18nId = element.getAttribute(attribute);
  GSC.Logging.checkWithLogger(
      logger,
      i18nId,
      'Failed to get attribute [' + attribute +
      '] for element: ' + element.outerHTML);

  var translatedText = chrome.i18n.getMessage(i18nId);
  GSC.Logging.checkWithLogger(
      logger,
      translatedText,
      'Failed to get translation for text with id: ' + i18nId);

  transformFunction(element, translatedText);
}

/**
 * @param {string} attribute
 * @param {function(!Element,string)} transformFunction
 */
function transformAllElements(attribute, transformFunction) {
  var selector = '[' + attribute + ']';
  for (let element of document.querySelectorAll(selector)) {
    transformElement(element, attribute, transformFunction);
  }
}

function transformFunctionI18nDataAttribute(element, translatedText) {
  logger.fine('Translating element.textContent [' + element.outerHTML +
              '] to "' + translatedText + '"');
  element.textContent = translatedText;
}

function transformFunctionI18nDataAriaLabelAttribute(element, translatedText) {
  logger.fine('Translating element.aria-label [' + element.outerHTML +
              '] to "' + translatedText + '"');
  element.setAttribute('aria-label', translatedText);
}

/** 
 * Takes the single element passed, replacing element.textContent with translation
 * if it conatins I18N_DATA_ATTRIBUTE, and setting aria-label to translation
 * if it contains I18N_DATA_ARIA_LABEL_ATTRIBUTE.
*/
GSC.I18n.adjustElementTranslation = function(element) {
  if (goog.dom.dataset.has(element, 'i18n')) {
    transformElement(
      element, I18N_DATA_ATTRIBUTE, 
      transformFunctionI18nDataAttribute);
  }

  if(goog.dom.dataset.has(element, 'i18nAriaLabel')) {
    transformElement(
      element, I18N_DATA_ARIA_LABEL_ATTRIBUTE, 
      transformFunctionI18nDataAriaLabelAttribute);
  }
};

/**
 * Goes through all the HTML elements in the current window, replacing
 * element.textContent with translation for elements containing attribute
 * I18N_DATA_ATTRIBUTE, and setting attribute aria-label to translation for
 * elements containing attribute I18N_DATA_ARIA_LABEL_ATTRIBUTE. Used for
 * translation that plays nice with Chromevox (accessibility tool).
 */
GSC.I18n.adjustAllElementsTranslation = function() {
  transformAllElements(
      I18N_DATA_ATTRIBUTE,
      transformFunctionI18nDataAttribute);

  transformAllElements(
      I18N_DATA_ARIA_LABEL_ATTRIBUTE,
      transformFunctionI18nDataAriaLabelAttribute);
};

});  // goog.scope
