/**
 * @license
 * Copyright The Closure Library Authors.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @fileoverview DOM pattern to match the end of a tag.
 */

goog.provide('goog.dom.pattern.EndTag');

goog.require('goog.dom.TagWalkType');
goog.require('goog.dom.pattern.Tag');



/**
 * Pattern object that matches a closing tag.
 *
 * @param {string|RegExp} tag Name of the tag.  Also will accept a regular
 *     expression to match against the tag name.
 * @param {Object=} opt_attrs Optional map of attribute names to desired values.
 *     This pattern will only match when all attributes are present and match
 *     the string or regular expression value provided here.
 * @param {Object=} opt_styles Optional map of CSS style names to desired
 *     values. This pattern will only match when all styles are present and
 *     match the string or regular expression value provided here.
 * @param {Function=} opt_test Optional function that takes the element as a
 *     parameter and returns true if this pattern should match it.
 * @constructor
 * @extends {goog.dom.pattern.Tag}
 * @final
 */
goog.dom.pattern.EndTag = function(tag, opt_attrs, opt_styles, opt_test) {
  goog.dom.pattern.Tag.call(
      this, tag, goog.dom.TagWalkType.END_TAG, opt_attrs, opt_styles, opt_test);
};
goog.inherits(goog.dom.pattern.EndTag, goog.dom.pattern.Tag);
