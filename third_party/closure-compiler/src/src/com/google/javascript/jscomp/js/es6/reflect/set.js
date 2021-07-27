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

'require es6/reflect/isextensible';
'require es6/reflect/reflect';
'require util/finddescriptor';
'require util/polyfill';


$jscomp.polyfill('Reflect.set', function(orig) {
  if (orig) return orig;

  /**
   * Polyfill for Reflect.set() method:
   * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Reflect/set
   *
   * Applies the 'setprop' operator as a function.
   *
   * @param {!Object} target Target on which to get the property.
   * @param {string} propertyKey Name of the property to get.
   * @param {*} value Value to set.
   * @param {!Object=} opt_receiver An optional 'this' to use for a setter.
   * @return {boolean} Whether setting was successful.
   */
  var polyfill = function(target, propertyKey, value, opt_receiver) {
    var property = $jscomp.findDescriptor(target, propertyKey);
    if (!property) {
      if (Reflect.isExtensible(target)) {
        target[propertyKey] = value;
        return true;
      }
      return false;
    }
    if (property.set) {
      property.set.call(arguments.length > 3 ? opt_receiver : target, value);
      return true;
    } else if (property.writable && !Object.isFrozen(target)) {
      target[propertyKey] = value;
      return true;
    }
    return false;
  };
  return polyfill;
}, 'es6', 'es5'); // ES5: findDescriptor requires getPrototypeOf
