/**
 * @license
 * Copyright 2023 Google Inc. All Rights Reserved.
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
 * @fileoverview This file contains tests for the PC/SC API exposed by Smart
 * Card Connector.
 *
 * TODO(emaxx): This is currently just a boilerplate. Real tests will be added
 * after needed helpers are implemented.
 */

goog.require('GoogleSmartCard.IntegrationTestController');
goog.require('goog.testing');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {GSC.IntegrationTestController?} */
let testController;

goog.exportSymbol('testPcscApi', {
  'setUp': async function() {
    testController = new GSC.IntegrationTestController();
    await testController.initAsync();
  },

  'tearDown': async function() {
    try {
      await testController.disposeAsync();
    } finally {
      testController = null;
    }
  },

  'testSmoke': async function() {
    // TODO(emaxx): The test currently does nothing. Add functional tests after
    // implementing needed C++ helpers.
  },
});
});  // goog.scope
