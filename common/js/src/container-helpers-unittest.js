/**
 * @license
 * Copyright 2018 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.ContainerHelpers');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;
const buildObjectFromMap = GSC.ContainerHelpers.buildObjectFromMap;
const substituteArrayBufferLikeObjectsRecursively =
    GSC.ContainerHelpers.substituteArrayBufferLikeObjectsRecursively;

goog.exportSymbol('testBuildObjectFromMap', function() {
  assertObjectEquals(buildObjectFromMap(new Map()), {});
  assertObjectEquals(
      buildObjectFromMap(new Map([['key', 'value']])), {'key': 'value'});
  assertObjectEquals(
      buildObjectFromMap(new Map([['key1', 'value1'], ['key2', 'value2']])),
      {'key1': 'value1', 'key2': 'value2'});
  // Test cases where keys and/or values are not strings.
  assertObjectEquals(buildObjectFromMap(new Map([['key', 123]])), {'key': 123});
  assertObjectEquals(
      buildObjectFromMap(new Map([[123, 'value']])), {123: 'value'});
  assertObjectEquals(
      buildObjectFromMap(new Map([['key 1', {'key 2': 'value'}]])),
      {'key 1': {'key 2': 'value'}});
  assertThrows(function() {
    buildObjectFromMap(new Map([[{}, 'value']]));
  });
});

goog.exportSymbol('testSubstituteArrayBuffersRecursively', function() {
  const buffer12 = (new Uint8Array([1, 2])).buffer;

  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(new ArrayBuffer(0)), []);
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(new ArrayBuffer(3)),
      [0, 0, 0]);
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(buffer12), [1, 2]);

  assertEquals(
      substituteArrayBufferLikeObjectsRecursively(undefined), undefined);
  assertEquals(substituteArrayBufferLikeObjectsRecursively(null), null);
  assertEquals(substituteArrayBufferLikeObjectsRecursively(false), false);
  assertEquals(substituteArrayBufferLikeObjectsRecursively(true), true);
  assertEquals(substituteArrayBufferLikeObjectsRecursively(123), 123);
  assertEquals(substituteArrayBufferLikeObjectsRecursively('foo'), 'foo');

  assertObjectEquals(substituteArrayBufferLikeObjectsRecursively([]), []);
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively([-1, 1000]), [-1, 1000]);
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(['a', buffer12]),
      ['a', [1, 2]]);
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively([[buffer12]]), [[[1, 2]]]);

  assertObjectEquals(substituteArrayBufferLikeObjectsRecursively({}), {});
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively({foo: {bar: 1, baz: null}}),
      {foo: {bar: 1, baz: null}});
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively({foo: buffer12}),
      {foo: [1, 2]});

  const uint8Array = new Uint8Array([1, 2, 255]);
  assertEquals(
      substituteArrayBufferLikeObjectsRecursively(uint8Array), uint8Array);

  // A function/class isn't a sensible input, but verify such cases anyway.
  assertEquals(substituteArrayBufferLikeObjectsRecursively(parseInt), parseInt);
  assertEquals(substituteArrayBufferLikeObjectsRecursively(Array), Array);
});


goog.exportSymbol('testSubstituteDataViewsRecursively', function() {
  const emptyView = new DataView(new ArrayBuffer(/*length=*/ 0));
  const buffer1234 = (new Uint8Array([1, 2, 3, 4])).buffer;
  const view1234 = new DataView(buffer1234);
  const view23 = new DataView(buffer1234, /*byteOffset=*/ 1, /*byteLength=*/ 2);

  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(emptyView), []);
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(view1234), [1, 2, 3, 4]);
  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(view23), [2, 3]);

  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively(['a', view1234]),
      ['a', [1, 2, 3, 4]]);

  assertObjectEquals(
      substituteArrayBufferLikeObjectsRecursively({foo: view23}),
      {foo: [2, 3]});
});
});  // goog.scope
