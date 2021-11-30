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
 * @fileoverview This file contains definitions for making the debug dumps of
 * the JavaScript values.
 */

goog.provide('GoogleSmartCard.DebugDump');

goog.require('goog.Disposable');
goog.require('goog.array');
goog.require('goog.functions');
goog.require('goog.iter');
goog.require('goog.json');
goog.require('goog.math');
goog.require('goog.object');
goog.require('goog.string');

goog.scope(function() {

const GSC = GoogleSmartCard;

/**
 * @param {?} value
 * @return {string}
 */
function encodeJson(value) {
  return goog.json.serialize(value) || 'null';
}

/**
 * @param {number} value
 * @return {number}
 */
function guessIntegerBitLength(value) {
  const BIT_LENGTHS = [8, 32, 64];
  let result = null;
  goog.array.forEach(BIT_LENGTHS, function(bitLength) {
    const signedBegin = -Math.pow(2, bitLength - 1);
    const unsignedEnd = Math.pow(2, bitLength);
    if (signedBegin <= value && value < unsignedEnd && result === null)
      result = bitLength;
  });
  return result;
}

/**
 * @param {!Array|!Uint8Array} value
 * @return {string}
 */
function dumpSequenceItems(value) {
  const dumpedItems = goog.iter.map(value, GSC.DebugDump.dump);
  return goog.iter.join(dumpedItems, ', ');
}

/**
 * @param {!Array} items Array of two-element arrays - pairs of key and value.
 * @return {string}
 */
function dumpMappingItems(items) {
  const dumpedItems = goog.array.map(items, function(item) {
    return GSC.DebugDump.dump(item[0]) + ': ' + GSC.DebugDump.dump(item[1]);
  });
  return goog.iter.join(dumpedItems, ', ');
}

/**
 * @param {number} value
 * @return {string}
 */
function dumpNumber(value) {
  // Format things like "NaN" and "Infinity" textually
  if (!isFinite(value))
    return value.toString();

  // Format real numbers by the default JSON formatter
  if (!goog.math.isInt(value))
    return encodeJson(value);

  const bitLength = guessIntegerBitLength(value);

  // Format huge integers by the default JSON formatter
  if (!bitLength)
    return encodeJson(value);

  // Format negative integers, which are not too huge, in the unsigned
  // representation (with the guessed bit length)
  if (value < 0)
    value += Math.pow(2, bitLength);

  // Format non-negative integers, which are not too huge, in the hexadecimal
  // system
  const hexValue = value.toString(16).toUpperCase();
  const expectedHexLength = bitLength / 4;
  return '0x' + goog.string.repeat('0', expectedHexLength - hexValue.length) +
      hexValue;
}

/**
 * An optimized version of `dumpNumber()` for the case of an unsigned
 * single-byte integer.
 * @param {number} value
 * @return {string}
 */
function dumpByte(value) {
  const HEX_LENGTH_OF_BYTE = 2;
  const hexValue = value.toString(16).toUpperCase();
  const zeroPadding = goog.string.repeat('0', HEX_LENGTH_OF_BYTE - hexValue.length);
  return `0x${zeroPadding}${hexValue}`;
}

/**
 * @param {string} value
 * @return {string}
 */
function dumpString(value) {
  return '"' + value + '"';
}

/**
 * @param {!Array|!Uint8Array} value
 * @return {string}
 */
function dumpArray(value) {
  return '[' + dumpSequenceItems(value) + ']';
}

/**
 * @param {function(...)} value
 * @return {string}
 */
function dumpFunction(value) {
  return '<Function>';
}

/**
 * @param {!ArrayBuffer} value
 * @return {string}
 */
function dumpArrayBuffer(value) {
  const bytes = new Uint8Array(value);
  // Not using `dumpSequenceItems()` or `dumpNumber()` here, since we can avoid
  // a lot of their overhead due to knowing the item type and boundaries.
  const dumpedBytes = goog.iter.map(bytes, byte => dumpByte(byte));
  return 'ArrayBuffer[' + goog.iter.join(dumpedBytes, ', ') + ']';
}

/**
 * @param {!Map} value
 * @return {string}
 */
function dumpMap(value) {
  return 'Map{' + dumpMappingItems(Array.from(value.entries())) + '}';
}

/**
 * @param {!Set} value
 * @return {string}
 */
function dumpSet(value) {
  return 'Set{' + dumpSequenceItems(Array.from(value)) + '}';
}

/**
 * @param {!Object} value
 * @return {string}
 */
function dumpObject(value) {
  const items = [];
  goog.object.forEach(value, function(itemValue, itemKey) {
    items.push([itemKey, itemValue]);
  });
  return '{' + dumpMappingItems(items) + '}';
}

/**
 * @param {?} value
 * @return {string}
 */
function dump(value) {
  // First, exit fast for DOM-related types, which contain cyclic references
  // breaking the code below, and for which in any case a meaningful debug dump
  // is difficult to be produced.
  //
  // Note that goog.dom.is* methods are not used because many of them may
  // produce thorny false positives.
  //
  // TODO(emaxx): Think about a proper solution that deals with cyclic
  // references.
  if (Document && value instanceof Document)
    return '<Document>';
  if (Window && value instanceof Window)
    return '<Window>';
  if (NodeList && value instanceof NodeList)
    return '<NodeList>';
  if (HTMLCollection && value instanceof HTMLCollection)
    return '<HTMLCollection>';
  if (Node && value instanceof Node) {
    // Note that this branch should go after other branches checking for
    // DOM-related types.
    return '<Node>';
  }

  // Fast exit for types that are likely non-struct classes, in order to avoid
  // cyclic references or huge meaningless debug dumps.
  if (value instanceof goog.Disposable)
    return '<Class>';

  if (value === undefined)
    return 'undefined';
  if (value === null)
    return 'null';
  if (typeof value === 'number')
    return dumpNumber(value);
  if (typeof value === 'string')
    return dumpString(value);
  if (Array.isArray(value))
    return dumpArray(value);
  if (goog.functions.isFunction(value))
    return dumpFunction(value);
  if (value instanceof ArrayBuffer)
    return dumpArrayBuffer(value);
  if (value instanceof Map)
    return dumpMap(value);
  if (value instanceof Set)
    return dumpSet(value);
  if (goog.isObject(value))
    return dumpObject(value);
  return encodeJson(value);
}

/**
 * Generates a debug textual representation of the passed value.
 *
 * Note that in the cases when the passed value may contain privacy-sensitive
 * data, the GoogleSmartCard.DebugDump.debugDump method should be used instead.
 * @param {?} value
 * @return {string}
 */
GSC.DebugDump.dump = function(value) {
  if (goog.DEBUG) {
    return dump(value);
  } else {
    // In Release mode, ensure that the debug dump generation cannot result in a
    // thrown exception.
    /** @preserveTry */
    try {
      return dump(value);
    } catch (exc) {
      return '<failed to dump the value>';
    }
  }
};

/**
 * Generates debug textual representation of the passed value in the Debug mode,
 * or the stub string in the Release mode (for the privacy reasons).
 * @param {*} value
 * @return {string}
 */
GSC.DebugDump.debugDump = function(value) {
  if (goog.DEBUG)
    return GSC.DebugDump.dump(value);
  else
    return '<stripped value>';
};
});  // goog.scope
