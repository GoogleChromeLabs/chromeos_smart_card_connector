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

/** @const */
var I18N_TITLE_ATTRIBUTE = 'title';

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

/**
 * @param {!Element} element 
 * @param {string} translatedText 
 */
function applyContentTranslation(element, translatedText) {
  logger.fine('Translating element.textContent [' + element.outerHTML +
              '] to "' + translatedText + '"');
  element.textContent = translatedText;
}

/**
 * @param {!Element} element 
 * @param {string} translatedText 
 */
function applyAriaLabelTranslation(element, translatedText) {
  logger.fine('Translating element.aria-label [' + element.outerHTML +
              '] to "' + translatedText + '"');
  element.setAttribute('aria-label', translatedText);
}

/**
 * @param {!Element} element 
 * @param {string} translatedText 
 */
function applyTitleTranslation(element, translatedText) {
  logger.fine('Translating element.title [' + element.outerHTML +
              '] to "' + translatedText + '"');
  element.setAttribute('title', translatedText);
}

/** 
 * Takes the element passed, replacing element.textContent with 
 * translation if it contains I18N_DATA_ATTRIBUTE, setting aria-label to 
 * translation if it contains I18N_DATA_ARIA_LABEL_ATTRIBUTE, and setting
 * title to translation if it contains I18N_TITLE_ATTRIBUTE.
 * @param {!Element} element
 */
GSC.I18n.adjustElementTranslation = function(element) {
  if (element.hasAttribute(I18N_DATA_ATTRIBUTE)) {
    transformElement(
        element, I18N_DATA_ATTRIBUTE, applyContentTranslation);
  }

  if (element.hasAttribute(I18N_DATA_ARIA_LABEL_ATTRIBUTE)) {
    transformElement(
        element, I18N_DATA_ARIA_LABEL_ATTRIBUTE, applyAriaLabelTranslation);
  }

  if (element.hasAttribute(I18N_TITLE_ATTRIBUTE)) {
    transformElement(
        element, I18N_TITLE_ATTRIBUTE, applyTitleTranslation);
  }
};

/**
 * Applies adjustElementTranslation() to all HTML elements in the current 
 * document. Used for translation that plays nice with Chromevox (accessibility
 * tool).
 */
GSC.I18n.adjustAllElementsTranslation = function() {
  transformAllElements(
      I18N_DATA_ATTRIBUTE, applyContentTranslation);

  transformAllElements(
      I18N_DATA_ARIA_LABEL_ATTRIBUTE, applyAriaLabelTranslation);

  transformAllElements(
      I18N_TITLE_ATTRIBUTE, applyTitleTranslation);
};

});  // goog.scope
