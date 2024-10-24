/**
 * @license
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

/**
 * @fileoverview This file contains an abstract interface that can be used for
 * loading and interacting with a binary executable module (e.g., an
 * Emscripten/WebAssembly module).
 */

goog.provide('GoogleSmartCard.ExecutableModule');

goog.require('GoogleSmartCard.Logging');
goog.require('goog.log.Logger');
goog.require('goog.messaging.AbstractChannel');
goog.require('goog.Disposable');
goog.require('goog.Thenable');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * Type of the toolchain used for building the binary executable module.
 * @enum {string}
 */
const Toolchain = {
  EMSCRIPTEN: 'emscripten',
};

/** @define {string} */
const TOOLCHAIN_STR =
    goog.define('GoogleSmartCard.ExecutableModule.TOOLCHAIN', '');
GSC.Logging.check(
    Object.values(Toolchain).includes(TOOLCHAIN_STR),
    `Unexpected value of GoogleSmartCard.ExecutableModule.TOOLCHAIN: ${
        TOOLCHAIN_STR}`);

/**
 * The toolchain that is used for building the executable module. This constant
 * is coming from the build scripts, which take it from the "TOOLCHAIN"
 * environment variable.
 * @type {!Toolchain}
 * @const
 */
const TOOLCHAIN = /** @type {!Toolchain} */ (TOOLCHAIN_STR);

/**
 * An interface that can be used for loading and interacting with a binary
 * executable module.
 * The object gets disposed of when the module fails to load or crashes.
 * @abstract
 */
GSC.ExecutableModule = class extends goog.Disposable {
  /**
   * @abstract
   * @return {!goog.log.Logger}
   */
  getLogger() {}

  /**
   * Begins loading and running the executable module.
   * Implemented as a separate method rather than the constructor's
   * responsibility, so that the consumer code can set up event and message
   * channel listeners.
   * @abstract
   */
  startLoading() {}

  /**
   * Returns the promise that gets fulfilled when the module loading completes.
   * The promise will be rejected in case the loading failed.
   * @abstract
   * @return {!goog.Thenable<void>}
   */
  getLoadPromise() {}

  /**
   * Returns the message channel for sending/receiving messages to/from the
   * module.
   * @abstract
   * @return {!goog.messaging.AbstractChannel}
   */
  getMessageChannel() {}
};

// Expose static attributes.
GSC.ExecutableModule.Toolchain = Toolchain;
GSC.ExecutableModule.TOOLCHAIN = TOOLCHAIN;
});  // goog.scope
