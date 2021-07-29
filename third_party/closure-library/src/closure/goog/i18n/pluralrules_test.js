/**
 * @license
 * Copyright The Closure Library Authors.
 * SPDX-License-Identifier: Apache-2.0
 */

goog.module('goog.i18n.pluralRulesTest');
goog.setTestOnly();

const pluralRules = goog.require('goog.i18n.pluralRules');
const testSuite = goog.require('goog.testing.testSuite');

/** @suppress {missingRequire} */
const Keyword = pluralRules.Keyword;

testSuite({
  testSimpleSelectEn() {
    /** @suppress {visibility} suppression added to enable type checking */
    const funcSelect = pluralRules.enSelect_;

    assertEquals(Keyword.OTHER, funcSelect(0));  // 0 dollars
    assertEquals(Keyword.ONE, funcSelect(1));    // 1 dollar
    assertEquals(Keyword.OTHER, funcSelect(2));  // 2 dollars

    assertEquals(Keyword.OTHER, funcSelect(0, 2));  // 0.00 dollars
    assertEquals(Keyword.OTHER, funcSelect(1, 2));  // 1.00 dollars
    assertEquals(Keyword.OTHER, funcSelect(2, 2));  // 2.00 dollars
  },

  testSimpleSelectRo() {
    /** @suppress {visibility} suppression added to enable type checking */
    const funcSelect = pluralRules.roSelect_;

    assertEquals(Keyword.FEW, funcSelect(0));       // 0 dolari
    assertEquals(Keyword.ONE, funcSelect(1));       // 1 dolar
    assertEquals(Keyword.FEW, funcSelect(2));       // 2 dolari
    assertEquals(Keyword.FEW, funcSelect(12));      // 12 dolari
    assertEquals(Keyword.OTHER, funcSelect(23));    // 23 de dolari
    assertEquals(Keyword.FEW, funcSelect(1212));    // 1212 dolari
    assertEquals(Keyword.OTHER, funcSelect(1223));  // 1223 de dolari

    assertEquals(Keyword.FEW, funcSelect(0, 2));     // 0.00 dolari
    assertEquals(Keyword.FEW, funcSelect(1, 2));     // 1.00 dolari
    assertEquals(Keyword.FEW, funcSelect(2, 2));     // 2.00 dolari
    assertEquals(Keyword.FEW, funcSelect(12, 2));    // 12.00 dolari
    assertEquals(Keyword.FEW, funcSelect(23, 2));    // 23.00  dolari
    assertEquals(Keyword.FEW, funcSelect(1212, 2));  // 1212.00  dolari
    assertEquals(Keyword.FEW, funcSelect(1223, 2));  // 1223.00 dolari
  },

  testSimpleSelectSr() {
    /** @suppress {visibility} suppression added to enable type checking */
    const funcSelect = pluralRules.srSelect_;  // Serbian

    assertEquals(Keyword.ONE, funcSelect(1));
    assertEquals(Keyword.ONE, funcSelect(31));
    assertEquals(Keyword.ONE, funcSelect(0.1));
    assertEquals(Keyword.ONE, funcSelect(1.1));
    assertEquals(Keyword.ONE, funcSelect(2.1));

    assertEquals(Keyword.FEW, funcSelect(3));
    assertEquals(Keyword.FEW, funcSelect(33));
    assertEquals(Keyword.FEW, funcSelect(0.2));
    assertEquals(Keyword.FEW, funcSelect(0.3));
    assertEquals(Keyword.FEW, funcSelect(0.4));
    assertEquals(Keyword.FEW, funcSelect(2.2));

    assertEquals(Keyword.OTHER, funcSelect(2.11));
    assertEquals(Keyword.OTHER, funcSelect(2.12));
    assertEquals(Keyword.OTHER, funcSelect(2.13));
    assertEquals(Keyword.OTHER, funcSelect(2.14));
    assertEquals(Keyword.OTHER, funcSelect(2.15));

    assertEquals(Keyword.OTHER, funcSelect(0));
    assertEquals(Keyword.OTHER, funcSelect(5));
    assertEquals(Keyword.OTHER, funcSelect(10));
    assertEquals(Keyword.OTHER, funcSelect(35));
    assertEquals(Keyword.OTHER, funcSelect(37));
    assertEquals(Keyword.OTHER, funcSelect(40));
    assertEquals(Keyword.OTHER, funcSelect(0.0, 1));
    assertEquals(Keyword.OTHER, funcSelect(0.5));
    assertEquals(Keyword.OTHER, funcSelect(0.6));

    assertEquals(Keyword.FEW, funcSelect(2));
    assertEquals(Keyword.ONE, funcSelect(2.1));
    assertEquals(Keyword.FEW, funcSelect(2.2));
    assertEquals(Keyword.FEW, funcSelect(2.3));
    assertEquals(Keyword.FEW, funcSelect(2.4));
    assertEquals(Keyword.OTHER, funcSelect(2.5));

    assertEquals(Keyword.OTHER, funcSelect(20));
    assertEquals(Keyword.ONE, funcSelect(21));
    assertEquals(Keyword.FEW, funcSelect(22));
    assertEquals(Keyword.FEW, funcSelect(23));
    assertEquals(Keyword.FEW, funcSelect(24));
    assertEquals(Keyword.OTHER, funcSelect(25));
  },
});
