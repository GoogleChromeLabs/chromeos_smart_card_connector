/*
 * Copyright 2014 The Closure Compiler Authors.
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

package com.google.javascript.jscomp;

import com.google.common.collect.ImmutableMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test case for {@link Es6RewriteBlockScopedDeclaration}. */
@RunWith(JUnit4.class)
public final class Es6RewriteBlockScopedDeclarationTest extends CompilerTestCase {

  private boolean runVarCheck = false;

  public Es6RewriteBlockScopedDeclarationTest() {
    super(DEFAULT_EXTERNS);
  }

  @Override
  protected CompilerOptions getOptions() {
    CompilerOptions options = super.getOptions();
    options.setWarningLevel(DiagnosticGroups.CHECK_VARIABLES, CheckLevel.OFF);
    return options;
  }

  @Before
  public void customSetUp() throws Exception {
    allowExternsChanges();
    enableTypeCheck();
    enableTypeInfoValidation();
    replaceTypesWithColors();
    enableMultistageCompilation();
  }

  @Override
  protected CompilerPass getProcessor(Compiler compiler) {
    return (externs, root) -> {
      if (runVarCheck) {
        // Adds a Synthetic definition of $jscomp$lookupPolyfilledValue
        new VarCheck(compiler).process(externs, root);
      }
      new Es6RewriteBlockScopedDeclaration(compiler).process(externs, root);
    };
  }

  private void loopClosureTest(Sources srcs, Expected originalExpected) {
    loopClosureTest(externs(""), srcs, originalExpected);
  }

  private void loopClosureTest(Externs externs, Sources srcs, Expected originalExpected) {
    Expected modifiedExpected =
        expected(
            UnitTestUtils.updateGenericVarNamesInExpectedFiles(
                (FlatSources) srcs, originalExpected, ImmutableMap.of("LOOP", "$jscomp$loop$")));
    test(externs, srcs, modifiedExpected);
  }

  @Test
  public void testVarNameCollisionWithExterns() {
    Externs externs = externs("var url;");
    Sources srcs = srcs("export {}; { const url = ''; alert(url);}");
    Expected expected =
        expected("export {}; { /** @const */ var url$jscomp$1 = ''; alert(url$jscomp$1); }");
    test(externs, srcs, expected);

    externs = externs("var url;" + DEFAULT_EXTERNS);
    srcs = srcs("goog.module('main'); { const url = ''; alert(url);}");
    expected =
        expected(
            "goog.module('main'); { /** @const */ var url$jscomp$1 = ''; alert(url$jscomp$1); }");
    test(externs, srcs, expected);

    test(
        externs("var url;"),
        srcs(
            lines(
                "export {};", //
                "function foo() {",
                "  const url = '';",
                "  alert(url);",
                "}")),
        expected(
            lines(
                "export {};", //
                "function foo() {",
                "  /** @const */ var url$jscomp$1 = '';",
                "  alert(url$jscomp$1);",
                "}")));

    test(
        externs("var url;" + DEFAULT_EXTERNS),
        srcs(
            lines(
                "goog.module('main');", //
                "function foo() {",
                "  const url = '';",
                "  alert(url);",
                "}")),
        expected(
            lines(
                "goog.module('main');", //
                "function foo() {",
                "  /** @const */",
                "  var url$jscomp$1 = '';",
                "  alert(url$jscomp$1);",
                "}")));

    test(
        externs("var url;" + DEFAULT_EXTERNS),
        srcs(
            lines(
                "function foo() {", //
                "  const url = '';",
                "  alert(url);",
                "}")),
        expected(
            lines(
                "function foo() {", //
                "  /** @const */",
                "  var url$jscomp$1 = '';",
                "  alert(url$jscomp$1);",
                "}")));
  }

  @Test
  public void testSimple() {
    test("let x = 3;", "var x = 3;");
    test("const x = 3;", "/** @const */ var x = 3;");
    test("const x = 1, y = 2;", "/** @const */ var x = 1; /** @const */ var y = 2;");
    test("const a = 0; a;", "/** @const */ var a = 0; a;");
    test(
        "if (a) { let x; }", //
        "if (a) { var x; }");
    test("function f() { const x = 3; }", "function f() { /** @const */ var x = 3; }");
  }

  @Test
  public void testLetShadowing() {
    Sources srcs =
        srcs(
            lines(
                "function f() {",
                "  var x = 1;",
                "  if (a) {",
                "    let x = 2;",
                "    x = function() { return x; };",
                "  }",
                "  return x;",
                "}"));
    Expected expected =
        expected(
            lines(
                "function f() {",
                "  var x = 1;",
                "  if (a) {",
                "    var x$jscomp$1 = 2;",
                "    x$jscomp$1 = function() { return x$jscomp$1; };",
                "  }",
                "  return x;",
                "}"));
    test(srcs, expected);

    srcs =
        srcs(lines("function f() {", "  const x = 3;", "  if (true) {", "    let x;", "  }", "}"));
    expected =
        expected(
            lines(
                "function f() {",
                "  /** @const */ var x = 3;",
                "  if (true) {",
                "    var x$jscomp$1;",
                "  }",
                "}"));
    test(srcs, expected);

    srcs =
        srcs(
            lines(
                "function f() {",
                "  var x = 1;",
                "  if (a) {",
                "    var g = function() { return x; };",
                "    let x = 2;",
                "    return g();",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "function f() {",
                "  var x = 1;",
                "  if (a) {",
                "    var g = function() { return x$jscomp$1; };",
                "    var x$jscomp$1 = 2;",
                "    return g();",
                "  }",
                "}"));
    test(srcs, expected);

    srcs =
        srcs(
            lines(
                "var x = 2;", //
                "function f() {",
                "  x = 1;",
                "  if (a) {",
                "    let x = 2;",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "var x = 2;",
                "function f() {",
                "  x = 1;",
                "  if (a) {",
                "    var x$jscomp$1 = 2;",
                "  }",
                "}"));
    test(srcs, expected);
  }

  @Test
  public void testLetShadowingUndeclaredWithVarCheck() {
    this.runVarCheck = true;
    Sources srcs =
        srcs(
            lines(
                "function use(x) {}", //
                "function f() {",
                "  {",
                "    let inner$jscomp$1 = 2;",
                "  }",
                "  use(inner)",
                "}"));
    Expected expected =
        expected(
            lines(
                "function use(x) {}", //
                "function f() {",
                "  {",
                "    var inner$jscomp$1 = 2;",
                "  }",
                "  use(inner)",
                "}"));
    test(srcs, expected);
  }

  @Test
  public void testLetShadowingTranspileOnly() {
    Sources srcs =
        srcs(
            lines(
                "function use(x) {}", //
                "function f() {",
                "  {",
                "    let inner = 2;",
                "  }",
                "  use(inner)",
                "}"));
    Expected expected =
        expected(
            lines(
                "function use(x) {}", //
                "function f() {",
                "  {",
                // Notice that without VarCheck running MakeDeclaredNamesUnique fails to
                // rename this variable to avoid conflicting with the undeclared global variable
                // used after this block in `use(inner)`.
                // This probably indicates a flaw in MakeDeclaredNamesUnique when an undeclared
                // variable comes after the scope of a local with the same name.
                // However, fixing this is not a high priority because:
                // 1. VarCheck always runs unless we are in transpile-only mode.
                // 2. Undeclared variables are already a known hazard with lots of warnings in the
                //    compiler to push you away from having them in your code.
                "    var inner = 2;",
                "  }",
                "  use(inner)",
                "}"));
    test(srcs, expected);
  }

  @Test
  public void testLetShadowingWithMultivariateDeclaration() {
    Sources srcs = srcs("var x, y; for (let x, y;;) {}");
    Expected expected =
        expected("var x, y; for (var x$jscomp$1 = void 0, y$jscomp$1 = void 0;;) {}");
    test(srcs, expected);
  }

  @Test
  public void testNonUniqueLet() {
    Sources srcs =
        srcs(
            lines(
                "function f() {",
                "  var x = 1;",
                "  if (a) {",
                "    let x = 2;",
                "    assert(x === 2);",
                "  }",
                "  if (b) {",
                "    let x;",
                "    assert(x === void 0);",
                "  }",
                "  assert(x === 1);",
                "}"));
    Expected expected =
        expected(
            lines(
                "function f() {",
                "  var x = 1;",
                "  if (a) {",
                "    var x$jscomp$1 = 2;",
                "    assert(x$jscomp$1 === 2);",
                "  }",
                "  if (b) {",
                "    var x$jscomp$2;",
                "    assert(x$jscomp$2 === void 0);",
                "  }",
                "  assert(x === 1);",
                "}"));
    test(srcs, expected);

    srcs =
        srcs(
            lines(
                "function f() {",
                "  if (a) {",
                "    let x = 2;",
                "    assert(x === 2);",
                "    if (b) {",
                "      let x;",
                "      assert(x === void 0);",
                "    }",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "function f() {",
                "  if (a) {",
                "    var x = 2;",
                "    assert(x === 2);",
                "    if (b) {",
                "      var x$jscomp$1;",
                "      assert(x$jscomp$1 === void 0);",
                "    }",
                "  }",
                "}"));
    test(srcs, expected);
  }

  @Test
  public void testRenameConflict() {
    Sources srcs =
        srcs(
            lines(
                "function f() {",
                "  let x = 1;",
                "  let x$0 = 2;",
                "  {",
                "    let x = 3;",
                "  }",
                "}"));
    Expected expected =
        expected(
            lines(
                "function f() {",
                "  var x = 1;",
                "  var x$0 = 2;",
                "  {",
                "    var x$jscomp$1 = 3;",
                "  }",
                "}"));
    test(srcs, expected);
  }

  @Test
  public void testForLoop() {
    Sources srcs =
        srcs(
            lines(
                "function use(x) {}",
                "function f() {",
                "  const y = 0;",
                "  for (let x = 0; x < 10; x++) {",
                "    const y = x * 2;",
                "    const z = y;",
                "  }",
                "  use(y);",
                "}"));
    Expected expected =
        expected(
            lines(
                "function use(x) {}",
                "function f() {",
                "  /** @const */ var y = 0;",
                "  for (var x$jscomp$1 = 0; x$jscomp$1 < 10; x$jscomp$1++) {",
                "    /** @const */ var y$jscomp$1 = x$jscomp$1 * 2;",
                "    /** @const */ var z = y$jscomp$1;",
                "  }",
                "  use(y);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "for (let i in [0, 1]) {",
                "  let f = function() {",
                "    let i = 0;",
                "    if (true) {",
                "      let i = 1;",
                "    }",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "for (var i in [0, 1]) {",
                "  var f = function() {",
                "    var i$jscomp$1 = 0;",
                "    if (true) {",
                "      var i$jscomp$2 = 1;",
                "    }",
                "  }",
                "}"));
    loopClosureTest(srcs, expected);

    srcs = srcs("for (let i = 0;;) { let i; }");
    expected = expected("for (var i = 0;;) { var i$jscomp$1 = void 0; }");
    loopClosureTest(srcs, expected);

    srcs = srcs("for (let i = 0;;) {} let i;");
    expected = expected("for (var i$jscomp$1 = 0;;) {} var i;");
    loopClosureTest(srcs, expected);

    test(
        lines(
            "for (var x in y) {", //
            "  /** @type {number} */",
            "  let i;",
            "}"),
        lines(
            "for (var x in y) {", //
            "  var i = void 0;",
            "}"));

    srcs =
        srcs(
            lines(
                "for (const i in [0, 1]) {",
                "  let f = function() {",
                "    let i = 0;",
                "    if (true) {",
                "      let i = 1;",
                "    }",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "for (/** @const */ var i in [0, 1]) {",
                "  var f = function() {",
                "    var i$jscomp$1 = 0;",
                "    if (true) {",
                "      var i$jscomp$2 = 1;",
                "    }",
                "  }",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLoopClosure() {
    Sources srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i = 0; i < 10; i++) {",
                "  arr.push(function() { return i; });",
                "}"));
    Expected expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = {",
                "      i: LOOP$0.i",
                "    },",
                "    LOOP$0.i++) {",
                "  arr.push((function(LOOP$0) {",
                "      return function() { return LOOP$0.i; };",
                "  })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i = 0; i < 10; i++) {",
                "  let y = i;",
                "  arr.push(function() { return y; });",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 10; ",
                "     LOOP$0 = {y: LOOP$0.y},",
                "     i++) {",
                "  LOOP$0.y = i;",
                "  arr.push((function(LOOP$0) {",
                "      return function() { return LOOP$0.y; };",
                "  })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "const arr = [];",
                "while (true) {",
                "  let i = 0;",
                "  arr.push(function() { return i; });",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {}",
                "while (true) {",
                "  LOOP$0 = {",
                "    i: LOOP$0.i",
                "  };",
                "  LOOP$0.i = 0;",
                "  arr.push(",
                "      (function(LOOP$0) {",
                "        return function() {",
                "          return LOOP$0.i;",
                "        };",
                "       })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i = 0; i < 10; i++) {",
                "  let y = i;",
                "  arr.push(function() { return y + i; });",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = {y:" + " LOOP$0.y, ",
                "        i: LOOP$0.i},",
                "        LOOP$0.i++) {",
                "  LOOP$0.y =" + " LOOP$0.i;",
                "  arr.push((function(LOOP$0) {",
                "    return function() {",
                "      return LOOP$0.y",
                "        + LOOP$0.i;",
                "    };",
                "  }(LOOP$0)));",
                "}"));
    loopClosureTest(srcs, expected);

    // Renamed inner i
    srcs =
        srcs(
            lines(
                "const arr = [];",
                "let x = 0",
                "for (let i = 0; i < 10; i++) {",
                "  let i = x + 1;",
                "  arr.push(function() { return i + i; });",
                "  x++;",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var x = 0",
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 10; ",
                "    LOOP$0 = {",
                "      i$jscomp$1: LOOP$0.i$jscomp$1",
                "    },",
                "    i++) {",
                "  LOOP$0.i$jscomp$1 = x + 1;",
                "  arr.push((function(LOOP$0) {",
                "      return function() {",
                "          return LOOP$0.i$jscomp$1 + LOOP$0.i$jscomp$1;",
                "      };",
                "  }(LOOP$0)));",
                "  x++;",
                "}"));
    loopClosureTest(srcs, expected);

    // Renamed, but both closures reference the inner i
    srcs =
        srcs(
            lines(
                "const arr = [];",
                "let x = 0",
                "for (let i = 0; i < 10; i++) {",
                "  arr.push(function() { return i + i; });",
                "  let i = x + 1;",
                "  arr.push(function() { return i + i; });",
                "  x++;",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var x = 0",
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 10;",
                "    LOOP$0 = {",
                "      i$jscomp$1: LOOP$0.i$jscomp$1",
                "    }, i++) {",
                "  arr.push((function(LOOP$0) {",
                "      return function() {",
                "          return LOOP$0.i$jscomp$1 + LOOP$0.i$jscomp$1;",
                "      };",
                "  }(LOOP$0)));",
                "  LOOP$0.i$jscomp$1 = x + 1;",
                "  arr.push((function(LOOP$0) {",
                "      return function() {",
                "          return LOOP$0.i$jscomp$1 + LOOP$0.i$jscomp$1;",
                "      };",
                "  }(LOOP$0)));",
                "  x++;",
                "}"));
    loopClosureTest(srcs, expected);

    // Renamed distinct captured variables
    srcs =
        srcs(
            lines(
                "for (let i = 0; i < 10; i++) {",
                "  if (true) {",
                "    let i = x - 1;",
                "    arr.push(function() { return i + i; });",
                "  }",
                "  let i = x + 1;",
                "  arr.push(function() { return i + i; });",
                "  x++;",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 10;",
                "    LOOP$0 = {",
                "      i$jscomp$2: LOOP$0.i$jscomp$2,",
                "      i$jscomp$1: LOOP$0.i$jscomp$1",
                "    }, i++) {",
                "  if (true) {",
                "    LOOP$0.i$jscomp$2 = x - 1;",
                "    arr.push(",
                "      (function(LOOP$0) {",
                "        return function() {",
                "          return LOOP$0.i$jscomp$2 + LOOP$0.i$jscomp$2;",
                "        };",
                "      })(LOOP$0));",
                "  }",
                "  LOOP$0.i$jscomp$1 = x + 1;",
                "  arr.push((function(LOOP$0) {",
                "      return function() {",
                "         return LOOP$0.i$jscomp$1 + LOOP$0.i$jscomp$1;",
                "      };",
                "  })(LOOP$0));",
                "  x++;",
                "}"));
    loopClosureTest(srcs, expected);

    srcs = srcs("for (;;) { /** @type {number} */ let x = 3; var f = function() { return x; } }");
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "for (;;LOOP$0 = {x: LOOP$0.x}) {",
                "  LOOP$0.x = 3;",
                "  var f = function(LOOP$0) {",
                "    return function() { return LOOP$0.x; }",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs = srcs("for (;;) { let /** number */ x = 3; var f = function() { return x; } }");
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "for (;; LOOP$0 = { x: LOOP$0.x }) {",
                "  LOOP$0.x = 3;",
                "  var f = function(LOOP$0) {",
                "    return function() { return LOOP$0.x; }",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    // Preserve constancy
    srcs = srcs("for (;;) { const /** number */ x = 3; var f = function() { return x; } }");
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "for (;; LOOP$0 = { x: LOOP$0.x}) {",
                "  /** @const */ LOOP$0.x = 3;",
                "  var f = function(LOOP$0) {",
                "    return function() { return LOOP$0.x}",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "for (;;) { let /** number */ x = 3, /** number */ y = 4;",
                "var f = function() { return x + y; } }"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "for (;;LOOP$0 = {x: LOOP$0.x, y: LOOP$0.y}) {",
                "  LOOP$0.x = 3;",
                "  LOOP$0.y = 4;",
                "  var f = function(LOOP$0) {",
                "    return function() { return LOOP$0.x + LOOP$0.y; };",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    // Preserve constancy on declaration lists
    srcs =
        srcs(
            lines(
                "for (;;) {",
                "  const /** number */ x = 3, /** number */ y = 4;",
                "  var f = function() { return x + y; };",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "for (;;LOOP$0 = { x: LOOP$0.x, y: LOOP$0.y })" + " {",
                "  /** @const */ LOOP$0.x = 3;",
                "  /** @const */ LOOP$0.y = 4;",
                "  var f = function(LOOP$0) {",
                "    return function() {",
                "        return LOOP$0.x + LOOP$0.y;",
                "    }",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    // No-op, vars don't need transpilation
    testSame("for (;;) { var x = 3; var f = function() { return x; } }");

    srcs =
        srcs(
            lines(
                "var i;", "for (i = 0;;) {", "  let x = 0;", "  var f = function() { x; };", "}"));
    expected =
        expected(
            lines(
                "var i;",
                "var LOOP$0={};",
                "i = 0;",
                "for(;; LOOP$0 = { x: LOOP$0.x }) {",
                "  LOOP$0.x = 0;",
                "  var f = (function(LOOP$0) {",
                "    return function() { LOOP$0.x; };",
                "  })(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "for (foo();;) {", //
                "  let x = 0;",
                "  var f = function() { x; };",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0={};",
                "foo();",
                "for(;; LOOP$0 = { x: LOOP$0.x }) {",
                "  LOOP$0.x = 0;",
                "  var f = (function(LOOP$0) {",
                "    return function() { LOOP$0.x; };",
                "  })(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "for (function foo() {};;) {", //
                "  let x = 0;",
                "  var f = function() { x; };",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0={};",
                "(function foo() {});",
                "for(;; LOOP$0 = { x: LOOP$0.x }) {",
                "  LOOP$0.x = 0;",
                "  var f = (function(LOOP$0) {",
                "    return function() { LOOP$0.x; };",
                "  })(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "for (;;) {", //
                "  let x;",
                "  foo(function() { return x; });",
                "  x = 5;",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "for(;; LOOP$0 = {x: LOOP$0.x}) {",
                "  LOOP$0.x = void 0;",
                "  foo(function(LOOP$0) {",
                "    return function() {",
                "      return LOOP$0.x;",
                "    };",
                "  }(LOOP$0));",
                "  LOOP$0.x=5;",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLoopClosureWithContinue() {
    // We must add a labeled block and convert continue statements to breaks to ensure that the
    // loop variable gets updated on every loop iteration of all loop types other than vanilla for.
    // For vanilla for(;;) loops we place the loop variable update in the loop update expression at
    // the top of the loop.

    // for-in case
    Sources srcs =
        srcs(
            lines(
                "const obj = {a: 1, b: 2, c: 3, skipMe: 4};",
                "const arr = [];",
                "for (const p in obj) {",
                "  if (p == 'skipMe') {",
                "    continue;",
                "  }",
                "  arr.push(function() { return obj[p]; });",
                "}",
                ""));
    Expected expected =
        expected(
            lines(
                "/** @const */ var obj = {a: 1, b: 2, c: 3, skipMe: 4};",
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "for (/** @const */ var p in obj) {",
                "  LOOP$0 = {",
                "    p: LOOP$0.p",
                "  };",
                "  LOOP$0.p = p;",
                "  if (LOOP$0.p == 'skipMe') {",
                "    continue;",
                "  }",
                "  arr.push(",
                "      (function(LOOP$0) {",
                "        return function() {",
                "          return obj[LOOP$0.p];",
                "        };",
                "      })(LOOP$0));",
                "}",
                ""));
    loopClosureTest(srcs, expected);

    // while case
    srcs =
        srcs(
            lines(
                "const values = ['a', 'b', 'c', 'skipMe'];",
                "const arr = [];",
                "while (values.length > 0) {",
                "  const v = values.shift();",
                "  if (v == 'skipMe') {",
                "    continue;",
                "  }",
                "  arr.push(function() { return v; });",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var values = ['a', 'b', 'c', 'skipMe'];",
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "while (values.length > 0) {",
                "  LOOP$0 = {",
                "    v: LOOP$0.v",
                "  };",
                "  /** @const */ LOOP$0.v = values.shift();",
                "  if (LOOP$0.v == 'skipMe') {",
                "    continue",
                "  }",
                "  arr.push(",
                "    (function(LOOP$0) {",
                "      return function() {",
                "        return LOOP$0.v;",
                "      };",
                "    })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);

    // do-while case
    srcs =
        srcs(
            lines(
                "const values = ['a', 'b', 'c', 'skipMe'];",
                "const arr = [];",
                "do {",
                "  const v = values.shift();",
                "  if (v == 'skipMe') {",
                "    continue;",
                "  }",
                "  arr.push(function() { return v; });",
                "} while (values.length > 0);",
                ""));
    expected =
        expected(
            lines(
                "/** @const */ var values = ['a', 'b', 'c', 'skipMe'];",
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "do {",
                "  LOOP$0 = {",
                "    v: LOOP$0.v",
                "  };",
                "  /** @const */ LOOP$0.v = values.shift();",
                "  if (LOOP$0.v == 'skipMe') {",
                "    continue",
                "  }",
                "  arr.push(",
                "      (function(LOOP$0) {",
                "        return function() {",
                "          return LOOP$0.v;",
                "        };",
                "      })(LOOP$0));",
                "} while (values.length > 0);",
                ""));
    loopClosureTest(srcs, expected);

    // labeled continue case
    srcs =
        srcs(
            lines(
                "const values = ['a', 'b', 'c', 'skipMe'];",
                "const arr = [];",
                "LABEL: while (values.length > 0) {",
                "  const v = values.shift();",
                "  if (v == 'skipMe') {",
                "    continue LABEL;",
                "  }",
                "  arr.push(function() { return v; });",
                "}",
                ""));
    expected =
        expected(
            lines(
                "/** @const */ var values = ['a', 'b', 'c', 'skipMe'];",
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "LABEL: while (values.length > 0) {",
                "  LOOP$0 = {",
                "    v: LOOP$0.v",
                "  };",
                "  /** @const */ LOOP$0.v = values.shift();",
                "  if (LOOP$0.v == 'skipMe') {",
                "    continue LABEL;",
                "  }",
                "  arr.push(",
                "      (function(LOOP$0) {",
                "        return function() {",
                "          return LOOP$0.v;",
                "        };",
                "      })(LOOP$0));",
                "}",
                ""));
    loopClosureTest(srcs, expected);

    // nested labeled continue case
    srcs =
        srcs(
            lines(
                "const values = ['abc', 'def', 'ghi', 'jkl'];",
                "const words = [];",
                "const letters = [];",
                "OUTER: while (values.length > 0) {",
                "  const v = values.shift();",
                "  let i = 0;",
                "  while (i < v.length) {",
                "    const c = v.charAt(i);",
                "    if (c == 'a') {",
                "      i++;",
                "      continue;",
                "    } else if (c == 'i') {",
                "      continue OUTER;",
                "    } else {",
                "      letters.push(function() { return c; });",
                "    }",
                "    i++;",
                "  }",
                "  words.push(function() { return v; });",
                "}",
                ""));
    expected =
        expected(
            lines(
                "/** @const */ var values = ['abc', 'def', 'ghi', 'jkl'];",
                "/** @const */ var words = [];",
                "/** @const */ var letters = [];",
                "var LOOP$1 = {};",
                "OUTER: while (values.length > 0) {",
                "  LOOP$1 = {",
                "    v: LOOP$1.v",
                "  };",
                "  /** @const */ LOOP$1.v = values.shift();",
                "  var i = 0;",
                "  var LOOP$0 = {};",
                "  while (i < LOOP$1.v.length) {",
                "      LOOP$0 = {",
                "        c: LOOP$0.c",
                "      };",
                "      /** @const */ LOOP$0.c = LOOP$1.v.charAt(i);",
                "      if (LOOP$0.c == 'a') {",
                "        i++;",
                "        continue;",
                "      } else if (LOOP$0.c == 'i') {",
                "        continue OUTER;",
                "      } else {",
                "        letters.push(",
                "            (function(LOOP$0) {",
                "              return function() {",
                "                return LOOP$0.c;",
                "              };",
                "            })(LOOP$0));",
                "      }",
                "      i++;",
                "  }",
                "  words.push(",
                "      (function(LOOP$1) {",
                "        return function() { return LOOP$1.v; };",
                "      })(LOOP$1));",
                "}",
                ""));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLoopClosureCommaInBody() {
    Sources srcs =
        srcs(
            lines(
                "const foobar = [];",
                "let j = 0;",
                "for (let i = 0; i < 10; i++) {",
                "  let i, j = 0;",
                "  foobar.push(function() { return i + j; });",
                "}"));
    Expected expected =
        expected(
            lines(
                "/** @const */",
                "var foobar = [];",
                "var j = 0;",
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 10;",
                "    LOOP$0 = {",
                "      i$jscomp$1: LOOP$0.i$jscomp$1,",
                "      j$jscomp$1: LOOP$0.j$jscomp$1",
                "    },",
                "    i++) {",
                "  LOOP$0.i$jscomp$1 = void 0;",
                "  LOOP$0.j$jscomp$1 = 0;",
                "  foobar.push(",
                "      function(LOOP$0) {",
                "        return function() {",
                "          return LOOP$0.i$jscomp$1 + LOOP$0.j$jscomp$1;",
                "        };",
                "      }(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLoopClosureCommaInIncrement() {
    Sources srcs =
        srcs(
            lines(
                "const arr = [];",
                "let j = 0;",
                "for (let i = 0; i < 10; i++, j++) {",
                "  arr.push(function() { return i + j; });",
                "}",
                ""));
    Expected expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var j = 0;",
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = {",
                "      i:LOOP$0.i",
                "    }, (LOOP$0.i++, j++)) {",
                "  arr.push((function(LOOP$0) {",
                "    return function() {",
                "      return LOOP$0.i + j;",
                "    };",
                "  })(LOOP$0));",
                "}",
                ""));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLoopClosureCommaInInitializerAndIncrement() {
    Sources srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i = 0, j = 0; i < 10; i++, j++) {",
                "  arr.push(function() { return i + j; });",
                "}"));
    Expected expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "LOOP$0.j = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = {",
                "      i: LOOP$0.i,",
                "      j: LOOP$0.j",
                "    },",
                "    (LOOP$0.i++, LOOP$0.j++)) {",
                "  arr.push((function(LOOP$0) {",
                "      return function() { ",
                "         return LOOP$0.i + LOOP$0.j;",
                "    };",
                "  })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i = 0, j = 0; i < 10; i++, j++) {",
                "  arr.push(function() { return j; });",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "var i = 0;",
                "LOOP$0.j = 0;",
                "for (; i < 10; ",
                "    LOOP$0 = {j :",
                "    LOOP$0.j},",
                "    (i++, LOOP$0.j++)) {",
                "  arr.push((function(LOOP$0) {",
                "      return function() { return LOOP$0.j; };",
                "  })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLoopClosureMutated() {
    Sources srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i = 0; i < 10; i++) {",
                "  arr.push(function() { return ++i; });",
                "}"));
    Expected expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = {i: LOOP$0.i},",
                "    LOOP$0.i++) {",
                "  arr.push((function(LOOP$0) {",
                "      return function() {",
                "          return ++LOOP$0.i;",
                "      };",
                "  }(LOOP$0)));",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i = 0; i < 10; i++) {",
                "  arr.push(function() { return i; });",
                "  i += 100;",
                "}"));
    expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = {i: LOOP$0.i},",
                "    LOOP$0.i++) {",
                "  arr.push((function(LOOP$0) {",
                "      return function() {",
                "          return LOOP$0.i;",
                "      };",
                "  }(LOOP$0)));",
                "  LOOP$0.i += 100;",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLoopClosureWithNestedInnerFunctions() {
    Sources srcs =
        srcs(
            lines(
                "for (let i = 0; i < 10; i++) {",
                "  later(function(ctr) {",
                "    (function() { return use(i); })();",
                "  });",
                "}"));
    Expected expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = { i: LOOP$0.i },",
                "    LOOP$0.i++) {",
                "  later((function(LOOP$0) {",
                "    return function(ctr) {",
                "      (function() { return use(LOOP$0.i); })();",
                "    };",
                "  })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "for (let i = 0; i < 10; i++) {",
                "  var f = function() {",
                "    return function() {",
                "      return i;",
                "    };",
                "  };",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "LOOP$0.i = 0;",
                "for (; LOOP$0.i < 10;",
                "    LOOP$0 = {i:" + " LOOP$0.i},",
                "    LOOP$0.i++) {",
                "  var f = function(LOOP$0) {",
                "    return function() {",
                "      return function() {",
                "        return LOOP$0.i;",
                "      };",
                "    };",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "use(function() {",
                "  later(function(ctr) {",
                "    for (let i = 0; i < 10; i++) {",
                "      (function() { return use(i); })();",
                "    }",
                "  });",
                "});"));
    expected =
        expected(
            lines(
                "use(function() {",
                "  later(function(ctr) {",
                "    var LOOP$0 = {};",
                "    LOOP$0.i = 0;",
                "    for (; LOOP$0.i < 10;",
                "        LOOP$0 = { i: LOOP$0.i },",
                "        LOOP$0.i++) {",
                "      (function(LOOP$0) {",
                "        return function() { return use(LOOP$0.i); }",
                "      })(LOOP$0)();",
                "    }",
                "  });",
                "});"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testNestedLoop() {
    Sources srcs =
        srcs(
            lines(
                "function f() {",
                "  let array = [];",
                "  for (let i = 0; i < 10; i++) {",
                "    for (let j = 0; j < 10; j++) {",
                "      array.push(function() { return j++ + i++; });",
                "      array.push(function() { return j++ + i++; });",
                "    }",
                "  }",
                "}"));
    Expected expected =
        expected(
            lines(
                "function f() {",
                "  var array = [];",
                "  var LOOP$1 = {};",
                "  LOOP$1.i = 0;",
                "  for (; LOOP$1.i < 10;",
                "      LOOP$1 = {",
                "        i: LOOP$1.i",
                "      },",
                "      LOOP$1.i++) {",
                "    var LOOP$0 = {};",
                "    LOOP$0.j = 0;",
                "    for (; LOOP$0.j < 10;",
                "        LOOP$0 = {",
                "          j: LOOP$0.j",
                "        },",
                "        LOOP$0.j++) {",
                "      array.push((function(LOOP$0, LOOP$1) {",
                "          return function() {",
                "              return LOOP$0.j++ + LOOP$1.i++;",
                "          };",
                "      }(LOOP$0, LOOP$1)));",
                "      array.push((function(LOOP$0, LOOP$1) {",
                "          return function() {",
                "              return LOOP$0.j++ + LOOP$1.i++;",
                "          };",
                "      }(LOOP$0, LOOP$1)));",
                "    }",
                "  }",
                "}"));
    loopClosureTest(srcs, expected);

    // Renamed inner i
    srcs =
        srcs(
            lines(
                "function f() {",
                "  let array = [];",
                "  for (let i = 0; i < 10; i++) {",
                "    array.push(function() { return i++ + i++; });",
                "    for (let i = 0; i < 10; i++) {",
                "      array.push(function() { return i++ + i++; });",
                "    }",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "function f() {",
                "  var array = [];",
                "  var LOOP$0 = {};",
                "  LOOP$0.i = 0;",
                "  for (; LOOP$0.i < 10;",
                "      LOOP$0 = { i: LOOP$0.i },",
                "      LOOP$0.i++) {",
                "    array.push(",
                "        (function(LOOP$0) {",
                "          return function() {",
                "            return LOOP$0.i++ + LOOP$0.i++;",
                "          };",
                "        }(LOOP$0)));",
                "    var LOOP$1 = {};",
                "    LOOP$1.i$jscomp$1 = 0;",
                "    for (; LOOP$1.i$jscomp$1 < 10;",
                "        LOOP$1 = { i$jscomp$1: LOOP$1.i$jscomp$1 },",
                "        LOOP$1.i$jscomp$1++) {",
                "      array.push(",
                "          (function(LOOP$1) {",
                "            return function() {",
                "              return LOOP$1.i$jscomp$1++ + LOOP$1.i$jscomp$1++;",
                "            };",
                "          }(LOOP$1)));",
                "    }",
                "  }",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testLabeledLoop() {
    Sources srcs =
        srcs(
            lines(
                "label1:",
                "label2:",
                "for (let x = 1;;) {",
                "  let f = function() {",
                "    return x;",
                "  }",
                "}"));
    Expected expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "LOOP$0.x = 1;",
                "label1:",
                "label2:",
                "for (;; LOOP$0 = { x: LOOP$0.x }) {",
                "  var f = function(LOOP$0) {",
                "    return function() {",
                "      return LOOP$0.x;",
                "    }",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testForIn() {
    Sources srcs =
        srcs(
            lines(
                "const arr = [];",
                "for (let i in [0, 1]) {",
                "  arr.push(function() { return i; });",
                "}"));
    Expected expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$0 = {};",
                "for (var i in [0, 1]) {",
                "  LOOP$0 = {",
                "    i: LOOP$0.i",
                "  };",
                "  LOOP$0.i = i;",
                "  arr.push((function(LOOP$0) {",
                "      return function() { return LOOP$0.i; };",
                "  })(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "for (;;) {",
                "  let a = getArray();",
                "  f = function() {",
                "    for (var x in use(a)) {",
                "      f(a);",
                "      a.push(x);",
                "      return x;",
                "    }",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "for (;; LOOP$0 = { a: LOOP$0.a }) {",
                "  LOOP$0.a = getArray();",
                "  f = (function(LOOP$0) {",
                "    return function() {",
                "      for (var x in use(LOOP$0.a)) {",
                "        f(LOOP$0.a);",
                "        LOOP$0.a.push(x);",
                "        return x;",
                "      }",
                "    };",
                "  }(LOOP$0));",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testDoWhileForInCapturedLet() {
    Sources srcs =
        srcs(
            lines(
                "const arr = [];",
                "do {",
                "  let special = 99;",
                "  const obj = { 0: 0, 1: 1, 2: special, 3: 3, 4: 4, 5: 5 };",
                "  for (let i in obj) {",
                "    i = Number(i);",
                "    arr.push(function() { return i++; });",
                "    arr.push(function() { return i + special; });",
                "  }",
                "} while (false);"));
    Expected expected =
        expected(
            lines(
                "/** @const */ var arr = [];",
                "var LOOP$1 = {};",
                "do {",
                "  LOOP$1 = {",
                "    special: LOOP$1.special",
                "  };",
                "  LOOP$1.special = 99;",
                "  /** @const */",
                "  var obj = ",
                "    {",
                "      0: 0,",
                "      1: 1,",
                "      2: LOOP$1.special,",
                "      3: 3,",
                "      4: 4,",
                "      5: 5",
                "    };",
                "  var LOOP$0 = {};",
                "  for (var i in obj) {",
                "    LOOP$0 = {",
                "      i: LOOP$0.i",
                "    };",
                "    LOOP$0.i = i",
                "    LOOP$0.i = Number(LOOP$0.i);",
                "    arr.push(",
                "        (function(LOOP$0) {",
                "          return function() {",
                "            return LOOP$0.i++;",
                "          };",
                "        }(LOOP$0)));",
                "    arr.push(",
                "        (function(LOOP$0, LOOP$1) {",
                "          return function() { ",
                "            return LOOP$0.i + LOOP$1.special;",
                "          };",
                "        }(LOOP$0, LOOP$1)));",
                "  }",
                "} while (false);"));
    loopClosureTest(srcs, expected);
  }

  // https://github.com/google/closure-compiler/issues/1124
  @Test
  public void testFunctionsInLoop() {
    Sources srcs =
        srcs(
            lines(
                "while (true) {",
                "  let x = null;",
                "  var f = function() {",
                "    x();",
                "  }",
                "}"));
    Expected expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "while (true) {",
                "  LOOP$0 =",
                "      {",
                "        x: LOOP$0.x",
                "      };",
                "  LOOP$0.x = null;",
                "  var f = function(LOOP$0) {",
                "    return function() {",
                "      (0,LOOP$0.x)();",
                "    };",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "while (true) {",
                "  let x = null;",
                "  let f = function() {",
                "    x();",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "while (true) {",
                "  LOOP$0 =",
                "      { x: LOOP$0.x };",
                "  LOOP$0.x = null;",
                "  var f = function(LOOP$0) {",
                "    return function() {",
                "      (0,LOOP$0.x)();",
                "    };",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "while (true) {",
                "  let x = null;",
                "  (function() {",
                "    x();",
                "  })();",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "while (true) {",
                "  LOOP$0 =",
                "      { x: LOOP$0.x };",
                "  LOOP$0.x = null;",
                "  (function(LOOP$0) {",
                "    return function() {",
                "      (0,LOOP$0.x)();",
                "    };",
                "  })(LOOP$0)();",
                "}"));
    loopClosureTest(srcs, expected);
  }

  // https://github.com/google/closure-compiler/issues/1557
  @Test
  public void testNormalizeDeclarations() {
    Sources srcs =
        srcs(
            lines(
                "while(true) {",
                "  let x, y;",
                "  let f = function() {",
                "    x = 1;",
                "    y = 2;",
                "  }",
                "}"));
    Expected expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "while (true) {",
                "  LOOP$0 = {",
                "    x: LOOP$0.x,",
                "    y: LOOP$0.y",
                "  };",
                "  LOOP$0.x = void 0;",
                "  LOOP$0.y = void 0;",
                "  var f = function(LOOP$0) {",
                "    return function() {",
                "      LOOP$0.x = 1;",
                "      LOOP$0.y = 2;",
                "    }",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);

    srcs =
        srcs(
            lines(
                "while(true) {",
                "  let x, y;",
                "  let f = function() {",
                "    y = 2;",
                "    x = 1;",
                "  }",
                "}"));
    expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "while (true) {",
                "  LOOP$0 = {",
                "    y: LOOP$0.y,",
                "    x: LOOP$0.x",
                "  };",
                "  LOOP$0.x = void 0;",
                "  LOOP$0.y = void 0;",
                "  var f = function(LOOP$0) {",
                "    return function() {",
                "      LOOP$0.y = 2;",
                "      LOOP$0.x = 1;",
                "    }",
                "  }(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testTypeAnnotationsOnLetConst() {
    Diagnostic mismatch = warning(TypeValidator.TYPE_MISMATCH_WARNING);

    test(srcs("/** @type {number} */ let x = 5; x = 'str';"), mismatch);
    test(srcs("let /** number */ x = 5; x = 'str';"), mismatch);
    test(srcs("let /** @type {number} */ x = 5; x = 'str';"), mismatch);

    test(srcs("/** @type {number} */ const x = 'str';"), mismatch);
    test(srcs("const /** number */ x = 'str';"), mismatch);
    test(srcs("const /** @type {number} */ x = 'str';"), mismatch);
    test(srcs("const /** @type {string} */ x = 3, /** @type {number} */ y = 3;"), mismatch);
    test(srcs("const /** @type {string} */ x = 'str', /** @type {string} */ y = 3;"), mismatch);
  }

  @Test
  public void testDoWhileForOfCapturedLetAnnotated() {
    test(
        lines(
            "while (true) {",
            "  /** @type {number} */ let x = 5;",
            "  (function() { x++; })();",
            "  x = 7;",
            "}"),
        null);

    test(
        lines(
            "for (/** @type {number} */ let x = 5;;) {",
            "  (function() { x++; })();",
            "  x = 7;",
            "}"),
        null);

    // TODO(sdh): NTI does not detect the type mismatch in the transpiled code,
    // since the LOOP$0 object does not have its type inferred until after
    // the mistyped assignment.
    test(
        srcs(
            lines(
                "while (true) {",
                "  /** @type {number} */ let x = 5;",
                "  (function() { x++; })();",
                "  x = 'str';",
                "}")),
        warning(TypeValidator.TYPE_MISMATCH_WARNING));

    test(
        srcs(
            lines(
                "for (/** @type {number} */ let x = 5;;) {",
                "  (function() { x++; })();",
                "  x = 'str';",
                "}")),
        warning(TypeValidator.TYPE_MISMATCH_WARNING));
  }

  @Test
  public void testLetForInitializers() {
    Sources srcs =
        srcs(
            lines(
                "{",
                "  let l = [];",
                "  for (var vx = 1, vy = 2, vz = 3; vx < 10; vx++) {",
                "    let lx = vx, ly = vy, lz = vz;",
                "    l.push(function() { return [ lx, ly, lz ]; });",
                "  }",
                "}"));
    Expected expected =
        expected(
            lines(
                "{",
                "  var l = [];",
                "  var LOOP$0 = {};",
                "  var vx = 1, vy = 2, vz = 3;",
                "  for (; vx < 10; ",
                "      LOOP$0 = {",
                "        lx: LOOP$0.lx,",
                "        ly: LOOP$0.ly,",
                "        lz: LOOP$0.lz",
                "      },",
                "      vx++){",
                "    LOOP$0.lx = vx;",
                "    LOOP$0.ly = vy;",
                "    LOOP$0.lz = vz;",
                "    l.push(function(LOOP$0) {",
                "        return function() {",
                "            return [ ",
                "                LOOP$0.lx,",
                "                LOOP$0.ly,",
                "                LOOP$0.lz ];",
                "        };",
                "    }(LOOP$0));",
                "  }",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testCatch() {
    Sources srcs = srcs("function f(e) { try {} catch (e) { throw e; } }");
    Expected expected =
        expected("function f(e) { try {} catch (e$jscomp$1) { throw e$jscomp$1; } }");
    test(srcs, expected);

    srcs =
        srcs(
            lines(
                "function f(e) {",
                "  try {",
                "    let f = function(e) {",
                "      try {} catch (e) { e++; }",
                "    }",
                "  } catch (e) { e--; }",
                "}"));
    expected =
        expected(
            lines(
                "function f(e) {",
                "  try {",
                "    var f$jscomp$1 = function(e$jscomp$1) {",
                "      try {} catch (e$jscomp$2) { e$jscomp$2++; }",
                "    }",
                "  } catch (e$jscomp$3) { e$jscomp$3--; }",
                "}"));
    test(srcs, expected);
  }

  // Regression test for https://github.com/google/closure-compiler/issues/3599
  @Test
  public void testReferenceToLoopScopedLetInObjectGetterAndSetter() {
    Sources srcs =
        srcs(
            lines(
                "for (let i = 0; i < 2; i++) {",
                "   let bar = 42;",
                "   let a = {",
                "     get foo() {",
                "      return bar",
                "     },",
                "     set foo(x) {",
                "      use(bar);",
                "     },",
                "     prop: bar",
                "   };",
                "   bar = 43;",
                "   use(a);",
                "}"));
    Expected expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 2;",
                "    LOOP$0 = { bar:LOOP$0.bar }, i++) {",
                "  LOOP$0.bar = 42;",
                // Note that we wrap the entire object literal in an IIFE, because that's simpler
                // than trying to individually wrap the getter and setter methods defined in it.
                "  var a =",
                "      (function(LOOP$0) {",
                "        return {",
                "          get foo() {",
                "            return LOOP$0.bar;",
                "          },",
                "          set foo(x) {",
                "            use(LOOP$0.bar);",
                "          },",
                "          prop: LOOP$0.bar",
                "      };",
                "  })(LOOP$0);",
                "  LOOP$0.bar = 43;",
                "  use(a);",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testReferenceToMultipleLoopScopedLetConstInObjectWithGetter() {
    Sources srcs =
        srcs(
            lines(
                "for (let i = 0; i < 2; i++) {",
                "   let bar = 42;",
                "   const baz = 43;",
                "   let a = {",
                "     get foo() {",
                "      return bar + baz",
                "     },",
                "   };",
                "}"));
    Expected expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 2;",
                "    LOOP$0 = { bar: LOOP$0.bar, baz:" + " LOOP$0.baz },",
                "    i++) {",
                "  LOOP$0.bar = 42;",
                "  /** @const */",
                "  LOOP$0.baz = 43;",
                "  var a = (function(LOOP$0) {",
                "   return {",
                "    get foo() {",
                "       return LOOP$0.bar +",
                "           LOOP$0.baz;",
                "    }",
                "  };",
                " })(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testReferenceToMultipleLoopScopedLetsInObjectWithSetter() {
    Sources srcs =
        srcs(
            lines(
                "for (let i = 0; i < 2; i++) {",
                "   let bar = 42;",
                "   let baz = 43;",
                "   let a = {",
                "     set foo(x =  bar) {",
                "      return x + baz",
                "     },",
                "   };",
                "}"));
    Expected expected =
        expected(
            lines(
                "var LOOP$0 = {};",
                "var i = 0;",
                "for (; i < 2;",
                "    LOOP$0 = {",
                "       bar: LOOP$0.bar,",
                "       baz: LOOP$0.baz",
                "    },",
                "    i++) {",
                "  LOOP$0.bar = 42;",
                "  LOOP$0.baz = 43;",
                "  var a =",
                "      (function(LOOP$0) {",
                "        return {",
                "          set foo(x = LOOP$0.bar) {",
                "            return x + LOOP$0.baz;",
                "          }",
                "        };",
                "      })(LOOP$0);",
                "}"));
    loopClosureTest(srcs, expected);
  }

  @Test
  public void testUniqueIdAcrossMultipleFiles() {
    Sources srcs =
        srcs(
            lines(
                "for (const i in [0, 1]) {",
                "  let f = function() {",
                "    let i = 0;",
                "    if (true) {",
                "      let i = 1;",
                "    }",
                "  }",
                "}"),
            lines(
                "for (const b in [0, 1]) {",
                "  let f = function() {",
                "    let b = 0;",
                "    if (true) {",
                "      let b = 1;",
                "    }",
                "  }",
                "}"));
    Expected expected =
        expected(
            lines(
                "for (/** @const */ var i in [0, 1]) {",
                "  var f = function() {",
                "    var i$jscomp$1 = 0;",
                "    if (true) {",
                "      var i$jscomp$2 = 1;",
                "    }",
                "  }",
                "}"),
            lines(
                "for (/** @const */",
                "var b in[0, 1]) {",
                "  var f$jscomp$1 = function() {",
                "    var b$jscomp$1 = 0;",
                "    if (true) {",
                "      var b$jscomp$2 = 1;",
                "    }",
                "  };",
                "}"));
    test(srcs, expected);
  }
}
