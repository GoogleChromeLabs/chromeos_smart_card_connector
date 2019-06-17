/*
 * Copyright 2016 The Closure Compiler Authors.
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

goog.module('jscomp.runtime_tests.polyfill_tests.array_values_test');
goog.setTestOnly();

const testSuite = goog.require('goog.testing.testSuite');
const testing = goog.require('jscomp.runtime_tests.polyfill_tests.testing');
const userAgent = goog.require('goog.userAgent');

const assertIteratorContents = testing.assertIteratorContents;
const noCheck = testing.noCheck;

testSuite({
  shouldRunTests() {
    // Disable tests for IE8 and below.
    return !userAgent.IE || userAgent.isVersionOrHigher(9);
  },

  testValues() {
    assertIteratorContents(['x', 'c'].values(), 'x', 'c');

    const arr = new Array();
    arr[3] = 42;
    assertIteratorContents(arr.values(), void 0, void 0, void 0, 42);

    assertIteratorContents(
        [].values.call(noCheck({length: 3, 1: 'x'})), void 0, 'x', void 0);

    assertIteratorContents(
        Array.prototype.values.call(noCheck('yq')), 'y', 'q');
  },
});
