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

/**
 * @fileoverview
 * Tests dynamic dispatch of functions after transpilation
 * from ES6 to ES3.
 *
 * @author mattloring@google.com (Matthew Loring)
 */
goog.provide('baz.Bar');
goog.provide('baz.Foo');

goog.require('goog.testing.jsunit');

var DynamicDispSup = class {
  g(str) {
    return this.f(str);
  }
  f(str) {
    return str + 'super';
  }
};
var DynamicDispSub = class extends DynamicDispSup {
  f(str) {
    return str + 'sub';
  }
};

function testDynamicDispatch() {
  var dds = new DynamicDispSub();
  assertEquals('hisub', dds.g('hi'));
}

var SuperCallSup = class {
  foo(b) {
    return b + 'sup';
  }
};
var SuperCallSub = class extends SuperCallSup {
  foo(bar) {
    return super.foo(bar);
  }
};

function testSuperCall() {
  var scs = new SuperCallSub();
  assertEquals('hsup', scs.foo('h'));
}

var ConstructorNoArgs = class {
  constructor() {
    this.a = 1729;
  }
};

function testConstructorEmpty() {
  assertEquals(1729, (new ConstructorNoArgs()).a);
}

var ConstructorArgs = class {
  /** @param {number} b */
  constructor(b) {
    this.a = b;
  }
};

function testConstructorArgs() {
  assertEquals(1729, (new ConstructorArgs(1729)).a);
}

var SuperConstructorSup = class {
  constructor() {
    this.m = 1729;
  }
  getM() {
    return this.m;
  }
  method() {
    class SuperConstructorSub extends SuperConstructorSup {
      constructor() {
        super();
      }
    }
    var d = new SuperConstructorSub();
    return d.getM();
  }
};

function testSuperConstructor() {
  var scsup = new SuperConstructorSup();
  assertEquals(1729, scsup.method());
}

/** @export */
baz.Foo = class {

  constructor(i) {
    this.i = i;
  }

  f() {
    return this.i;
  }

};

baz.Bar = class extends baz.Foo {
  constructor() {
    super(1729);
  }
};

function testQualifiedNames() {
  assertEquals(1729, (new baz.Bar()).f());
}

function testExport() {
  assertEquals(5, new window['baz']['Foo'](5).f());
}

var A = class {
  constructor() {
    this.x = 1234;
  }
  init() {}
  /** @nocollapse */
  static create() {
    var newThis = new this();
    newThis.init.apply(newThis, arguments);
    return newThis;
  }
};

var B = class extends A {
  init() {
    super.init();
  }
};

function testImplicitSuperConstructorCall() {
  assertEquals(1234, B.create().x);
}

/**
 * Does the current environment set `stack` on Error objects either when
 * they are created or when they are thrown?
 * @returns {boolean}
 */
function thrownErrorHasStack() {
  const e = new Error();
  try {
    throw e;
  } finally {
    return 'stack' in e;
  }
}

function testExtendsError() {
  class MyError extends Error {
    constructor() {
      super('my message');
    }
  }
  try {
    throw new MyError();
  } catch (e) {
    // IE11 and earlier don't set the stack field until the error is actually
    // thrown. IE8 doesn't set it at all.
    assertTrue(e instanceof MyError);
    assertEquals('my message', e.message);
    if (thrownErrorHasStack()) {
      assertNonEmptyString(e.stack);
    }
  }
}

function testExtendsErrorSuperReturnsThis() {
  class MyError extends Error {
    constructor() {
      const superResult = super('my message');
      this.superResult = superResult;
    }
  }
  const e = new MyError();
  assertEquals(e, e.superResult);
}

function testExtendsObject() {
  // When super() returns an object, it is supposed to replace `this`,
  // but Object() always returns a new object, so it must be handled specially.
  // Make sure that the compiler does NOT do that replacement.
  class NoConstructor extends Object {}
  assertTrue(new NoConstructor('junk') instanceof NoConstructor);

  // super() should have `this` as its return value, not a new object created
  // by calling Object().
  class WithConstructor extends Object {
    constructor(message) {
      const superResult = super();
      this.superResult = superResult;
      this.message = message;
    }
  }
  const withConstructor = new WithConstructor('message');
  assertEquals(withConstructor, withConstructor.superResult);
}

function testSuperInStaticMethod() {
  class C1 {
    static foo() { return 42; }
  }
  class D extends C1 {
    static foo() { return super.foo() + 1; }
  }
  assertEquals(43, D.foo());
}

function testSuperInDifferentStaticMethod() {
  class C2 {
    static foo() { return 12; }
  }
  class D extends C2 {
    static foo() { throw new Error('unexpected'); }
    static bar() { return super.foo() + 2; }
  }
  assertEquals(14, D.bar());
}

// Confirm that extension of native classes works.
// Use Map, because it's a class users are likely to want to extend.
function testExtendMapClass() {
  /**
   * @template K
   * @extends {Map<K, number>}
   */
  class CounterMap extends Map {
    /**
     * @param {!Iterable<!Array<K|number>>} keyCountPairs
     */
    constructor(keyCountPairs) {
      super(keyCountPairs);  // test calling super() with arguments.
    }

    /** @override */
    get(key) {
      // test overriding a method
      return this.has(key) ? super.get(key) : 0;
    }

    /**
     * Increment the counter for `key`.
     *
     * @param {K} key
     * @return {number} count after incrementing
     */
    increment(key) {
      const newCount = this.get(key) + 1;
      this.set(key, newCount);
      return newCount;
    }
  }

  /** @type {!CounterMap<string>} */
  const counterMap = new CounterMap([['a', 1], ['b', 2]]);

  assertEquals(1, counterMap.get('a'));
  assertEquals(2, counterMap.get('b'));
  assertEquals(0, counterMap.get('not-there'));

  assertEquals(2, counterMap.increment('a'));
  assertEquals(3, counterMap.increment('b'));
  assertEquals(1, counterMap.increment('add-me'));
}
