/**
 * @license
 * Copyright The Closure Library Authors.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @fileoverview Provides an implementation for getInstance() methods.
 */

goog.module('goog.singleton');
goog.module.declareLegacyNamespace();

const {assert} = goog.require('goog.asserts');

/** @type {!Array<function(new: ?): ?>} */
const instantiatedSingletons = [];

/**
 * @template T
 * @record
 */
class Singleton {
  constructor() {
    /** @type {!T} */
    this.instance_;
  }
}

/**
 * Used as the implementation body for a static getInstance method.
 *
 * ```
 * class Foo {
 *   static getInstance() {
 *     return getInstance(Foo);
 *   }
 * }
 * ```
 * @param {function(new: T)} ctor
 * @return {T}
 * @template T
 * @deprecated Singleton patterns are discouraged. Use dependency injection
 *     instead.
 */
exports.getInstance = (ctor) => {
  assert(
      !Object.isSealed(ctor),
      'Cannot use getInstance() with a sealed constructor.');
  const ctorWithInstance = /** @type {!Singleton} */ (ctor);
  if (ctorWithInstance.instance_) {
    return ctorWithInstance.instance_;
  }
  if (goog.DEBUG) {
    // Used to reset singletons in test code.
    instantiatedSingletons.push(ctor);
  }
  const instance = new ctor();
  ctorWithInstance.instance_ = instance;
  return instance;
};

exports.instantiatedSingletons = instantiatedSingletons;
