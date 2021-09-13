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

goog.provide('GoogleSmartCard.Packaging');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Enumeration containing possible packaging modes for the resulting
 * application.
 * @enum {string}
 */
GSC.Packaging.Mode = {
  APP: 'app',
  EXTENSION: 'extension',
};

/**
 * @define {string} The packaging mode to be used for the resulting application.
 */
const MODE = goog.define('GoogleSmartCard.Packaging.MODE', '');
GSC.Logging.check(
    Object.values(GSC.Packaging.Mode).includes(MODE),
    `Unexpected value of GoogleSmartCard.Packaging.MODE: ${MODE}`);

/**
 * The packaging mode that is used for building the resulting application. This
 * constant is coming from the build scripts, which take it from the "PACKAGING"
 * environment variable.
 * @type {!GSC.Packaging.Mode}
 * @const
 */
GSC.Packaging.MODE = /** @type {!GSC.Packaging.Mode} */ (MODE);

});  // goog.scope
