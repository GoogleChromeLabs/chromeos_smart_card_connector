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

/**
 * @fileoverview This file contains helper definitions for operating with the
 * system clipboard.
 */

goog.provide('GoogleSmartCard.Clipboard');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.dom');
goog.require('goog.log');
goog.require('goog.log.Logger');

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {!goog.log.Logger} */
const logger = GSC.Logging.getScopedLogger('Clipboard');

/**
 * Copies the passed text to the clipboard.
 * NOTE: This function only works when called from a foreground window and only
 * in response to an explicit user gesture.
 *
 * The text may contain the \n characters as a delimiter for multiple lines.
 * @param {string} text
 * @return {boolean}
 */
GSC.Clipboard.copyToClipboard = function(text) {
  // Create a DOM node with the text.
  const element = goog.dom.createDom('textarea');
  element.textContent = text;
  document.body.appendChild(element);

  // Select and copy the text.
  element.select();
  const success = document.execCommand('copy');

  // Clean up the DOM node. Deleting the text isn't strictly necessary, but in
  // practice helps reduce the RAM consumption quickly.
  element.textContent = '';
  document.body.removeChild(element);

  if (!success) {
    goog.log.error(
        logger,
        'Failed to copy the text of length ' + text.length + ' to clipboard');
    return false;
  }
  goog.log.fine(
      logger, 'Copied text of length ' + text.length + ' to clipboard');
  return true;
};
});  // goog.scope
