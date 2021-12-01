/**
 * @license
 * Copyright 2017 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('goog.testing.jsunit');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const dump = GSC.DebugDump.dump;

goog.exportSymbol('testDebugDump', function() {
  assertEquals('undefined', dump(undefined));

  assertEquals('null', dump(null));

  assertEquals('0x00', dump(0));
  assertEquals('0xFE', dump(254));
  assertEquals('0x00000100', dump(256));
  assertEquals('0xFFFFFFFF', dump(Math.pow(2, 32) - 1));
  assertEquals('0x0000000100000000', dump(Math.pow(2, 32)));
  assertEquals('0x4000000000000000', dump(Math.pow(2, 62)));
  assertEquals('0xFF', dump(-1));
  assertEquals('0x80000000', dump(-Math.pow(2, 31)));
  assertEquals('1.5', dump(1.5));

  assertEquals('"foo bar"', dump('foo bar'));

  assertEquals('[0x01, "foo", [0x02, 0x03], []]', dump([1, 'foo', [2, 3], []]));

  assertEquals('ArrayBuffer[]', dump(new ArrayBuffer(0)));
  assertEquals(
      'ArrayBuffer[0x01FF]', dump(new Uint8Array([1, 255]).buffer));

  assertEquals(
      'Map{0x01: 0x02, 0x03: "foo", "bar": {}, {}: 0x04}',
      dump(new Map([[1, 2], [3, 'foo'], ['bar', {}], [{}, 4]])));

  assertEquals('Set{0x01, 0x02}', dump(new Set([1, 2])));

  assertEquals('{}', dump({}));
  assertEquals('{"a": "b", "c": [0x01, "d"]}', dump({'a': 'b', 'c': [1, 'd']}));

  assertEquals('<Function>', dump(function() {}));

  assertEquals('<Document>', dump(document));
  assertEquals('<Window>', dump(window));
  assertEquals('<Node>', dump(document.body));
  assertEquals('<NodeList>', dump(document.body.childNodes));
  assertEquals('<HTMLCollection>', dump(document.body.children));
});

// Test the `dump()` method against container types with circular references: it
// should detect the loops and avoid infinite recursion.
goog.exportSymbol('testDebugDumpWithCircularRefs', function() {
  const circularInsideArray = [];
  circularInsideArray.push(circularInsideArray);
  assertEquals('[<circular>]', dump(circularInsideArray));

  const circularInObject = {'a': 'b'};
  circularInObject['c'] = circularInObject;
  assertEquals('{"a": "b", "c": <circular>}', dump(circularInObject));

  const circularInMap = new Map();
  circularInMap.set('a', 'b');
  circularInMap.set('c', circularInMap);
  assertEquals('Map{"a": "b", "c": <circular>}', dump(circularInMap));

  const circularInSet = new Set();
  circularInSet.add('foo');
  circularInSet.add(circularInSet);
  assertEquals('Set{"foo", <circular>}', dump(circularInSet));

  const nestedCircular = {};
  nestedCircular['foo'] = {'bar': nestedCircular};
  assertEquals('{"foo": {"bar": <circular>}}', dump(nestedCircular));
});

// Test that the `dump()` method correctly handles the case when an object is
// linked from two different places: it shouldn't be confused with a circular
// reference.
goog.exportSymbol('testDebugDumpWithDuplicateRefs', function() {
  const object = {'a': 'b'};
  const arrayWithDuplicates = [object, object];
  assertEquals('[{"a": "b"}, {"a": "b"}]', dump(arrayWithDuplicates));

  const array = [];
  const objectWithDuplicates = {'foo': array, 'foobar': array};
  assertEquals('{"foo": [], "foobar": []}', dump(objectWithDuplicates));

  const diamond = {'x': {'a': array}, 'y': {'b': array}};
  assertEquals('{"x": {"a": []}, "y": {"b": []}}', dump(diamond));
});
});  // goog.scope
