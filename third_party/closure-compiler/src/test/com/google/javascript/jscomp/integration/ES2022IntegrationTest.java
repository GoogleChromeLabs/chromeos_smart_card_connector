/*
 * Copyright 2020 The Closure Compiler Authors.
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

package com.google.javascript.jscomp.integration;

import static com.google.javascript.jscomp.base.JSCompStrings.lines;

import com.google.common.collect.ImmutableList;
import com.google.javascript.jscomp.CompilationLevel;
import com.google.javascript.jscomp.CompilerOptions;
import com.google.javascript.jscomp.CompilerOptions.DevMode;
import com.google.javascript.jscomp.CompilerOptions.LanguageMode;
import com.google.javascript.jscomp.DiagnosticGroup;
import com.google.javascript.jscomp.DiagnosticGroups;
import com.google.javascript.jscomp.GoogleCodingConvention;
import com.google.javascript.jscomp.WarningLevel;
import com.google.javascript.jscomp.testing.TestExternsBuilder;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Integration tests related to ES2022 features like class fields */
@RunWith(JUnit4.class)
public final class ES2022IntegrationTest extends IntegrationTestCase {

  /** Creates a CompilerOptions object with google coding conventions. */
  protected CompilerOptions createCompilerOptions() {
    CompilerOptions options = new CompilerOptions();
    options.setLanguageIn(LanguageMode.ECMASCRIPT_NEXT_IN);
    options.setDevMode(DevMode.EVERY_PASS);
    options.setCodingConvention(new GoogleCodingConvention());
    return options;
  }

  private CompilerOptions checksOnlyCompilerOptions() {
    CompilerOptions options = createCompilerOptions();
    options.setLanguageOut(LanguageMode.NO_TRANSPILE);
    options.setChecksOnly(true);
    WarningLevel.VERBOSE.setOptionsForWarningLevel(options);

    return options;
  }

  private CompilerOptions fullyOptimizedCompilerOptions() {
    CompilerOptions options = createCompilerOptions();
    CompilationLevel.ADVANCED_OPTIMIZATIONS.setOptionsForCompilationLevel(options);
    CompilationLevel.ADVANCED_OPTIMIZATIONS.setTypeBasedOptimizationOptions(options);

    options.setLanguageOut(LanguageMode.ECMASCRIPT_NEXT);
    WarningLevel.VERBOSE.setOptionsForWarningLevel(options);

    return options;
  }

  @Test
  public void publicClassFields_supportedInChecksOnlyMode() {
    CompilerOptions options = checksOnlyCompilerOptions();
    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    testNoWarnings(
        options,
        lines(
            "class MyClass {", //
            "  /** @type {number} */",
            "  x = 2;",
            "  y;",
            "}",
            "console.log(new MyClass().x);"));
  }

  @Test
  public void publicClassFields_supportedInChecksOnlyMode2() {
    CompilerOptions options = checksOnlyCompilerOptions();
    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    testNoWarnings(
        options,
        lines(
            "class MyClass {", //
            "  x = '';",
            "  y;",
            "}",
            "console.log(new MyClass().x);"));
  }

  @Test
  public void publicClassFields_supportedInChecksOnlyMode3() {
    CompilerOptions options = checksOnlyCompilerOptions();

    test(
        options,
        new String[] {
          lines(
              "class MyClass {", //
              "  /** @type {string} */",
              "  x = 2;",
              "}")
        },
        /* compiled= */ null,
        new DiagnosticGroup[] {DiagnosticGroups.CHECK_TYPES});
  }

  @Test
  public void computedPublicClassFields_supportedInChecksOnlyMode() {
    CompilerOptions options = checksOnlyCompilerOptions();
    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    testNoWarnings(
        options,
        lines(
            "/** @dict */", //
            "class MyClass {",
            "  [3 + 4] = 5;",
            "  [6];",
            "  'x' = 2;",
            "}",
            "console.log(new MyClass()[6]);"));
  }

  @Test
  public void computedPublicClassFields_supportedInChecksOnlyMode2() {
    CompilerOptions options = checksOnlyCompilerOptions();
    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    testNoWarnings(
        options,
        lines(
            "/** @dict */", //
            "class MyClass {",
            "  ['x'] = 5;",
            "}",
            "console.log(new MyClass()['x']);"));
  }

  // deprecated warnings aren't given on computed fields:
  // users should be careful about tags on computed fields
  @Test
  public void computedPublicClassFields_supportedInChecksOnlyMode3() {
    CompilerOptions options = checksOnlyCompilerOptions();

    testNoWarnings(
        options,
        lines(
            "/** @unrestricted */", //
            "class MyClass {",
            "  /** @deprecated */",
            "  ['x'] = 5;",
            "  baz() { return this['x']; }",
            "}"));
  }

  @Test
  public void computedPublicClassFields_supportedInChecksOnlyMode4() {
    CompilerOptions options = checksOnlyCompilerOptions();

    // test will give JSC_ILLEGAL_PROPERTY_ACCESS error because @dict or @restricted is missing
    test(
        options,
        new String[] {
          lines(
              "class MyClass {", //
              "  [3 + 4] = 5;",
              "}"),
        },
        /* compiled= */ null,
        new DiagnosticGroup[] {DiagnosticGroups.CHECK_TYPES});
  }

  @Test
  public void publicMixedClassFields_supportedInChecksOnlyMode() {
    CompilerOptions options = checksOnlyCompilerOptions();

    testNoWarnings(
        options,
        lines(
            "/** @unrestricted */",
            "class MyClass {", //
            "  a = 2;",
            "  ['b'] = 'hi';",
            "  'c' = 5;",
            "  2 = 4;",
            "  d;",
            "  ['e'];",
            "}"));
  }

  @Test
  public void publicClassFields_supportedInOptimizationsMode() {
    CompilerOptions options = fullyOptimizedCompilerOptions();
    options.setPrintSourceAfterEachPass(true);

    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    test(
        options,
        lines(
            "class C {",
            "  f1 = 1;",
            "static f2 = 3;",
            "  m1() {return this.f1}",
            "}",
            "console.log(new C().f1);",
            "console.log(C.f2);",
            "console.log(new C().m1());"),
        lines("console.log(1);", "console.log(3);", "console.log(1);"));
  }

  @Test
  public void publicClassFields_supportedInOptimizationsMode1() {
    CompilerOptions options = fullyOptimizedCompilerOptions();
    options.setPrintSourceAfterEachPass(true);

    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    test(
        options,
        lines(
            "class MyClass {", //
            "  /** @type {number} */",
            "  f1 = 2;",
            "  /** @type {string} */",
            "  f2 = 'hi';",
            "  f3 = function() { return this.f1 };",
            "  m1() { return this.f2; }",
            "}",
            "console.log(new MyClass().f1);",
            "console.log(new MyClass().f2);",
            "console.log(new MyClass().f3());",
            "console.log(new MyClass().m1());"),
        lines("console.log(2);", "console.log('hi');", "console.log(2);", "console.log('hi');"));
  }

  @Test
  public void publicClassFields_supportedInOptimizationsMode2() {
    CompilerOptions options = fullyOptimizedCompilerOptions();
    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    test(
        options,
        lines(
            "/** @dict */",
            "class MyClass {", //
            "  ['f1'] = 2;",
            "  'f2' = 'hi';",
            "  2 = 4;",
            "  ['m1']() { return this['f1']; }",
            "}",
            "console.log(new MyClass()['f1']);",
            "console.log(new MyClass()[2]);",
            "console.log(new MyClass()['m1']());"),
        lines(
            "class a {", //
            "  f1 = 2;",
            "  f2= 'hi';",
            "  2 = 4;",
            "  m1() { return this.f1; }",
            "}",
            "console.log((new a).f1);",
            "console.log((new a)[2]);",
            "console.log((new a).m1());"));
  }

  @Test
  public void publicClassFields_supportedInOptimizationsMode3() {
    CompilerOptions options = fullyOptimizedCompilerOptions();
    externs = ImmutableList.of(new TestExternsBuilder().addConsole().buildExternsFile("externs"));

    test(
        options,
        lines(
            "/** @unrestricted */",
            "class MyClass {", //
            "  f1 = 1;",
            "  ['a'] = 'hi';",
            "  ['f3'] = function() { return this.f1; };",
            "}",
            "console.log(new MyClass().f1);",
            "console.log(new MyClass()['a']);",
            "console.log(new MyClass()['f3']());"),
        lines(
            "class a {", //
            "  b = 1;",
            "  a = 'hi';",
            "  f3 = function() { return this.b; };",
            "}",
            "console.log(1);",
            "console.log(new a().a);",
            "console.log(new a().f3());"));
  }

  @Test
  public void publicClassFields_supportedInOptimizationsMode4() {
    CompilerOptions options = fullyOptimizedCompilerOptions();

    test(
        options,
        lines(
            "/** @unrestricted */",
            "class MyClass {", //
            "  static f1 = alert(2);",
            "  ['f2'] = 'hi';",
            "  'f3' = 5;",
            "  4 = 4;",
            "  f5;",
            "  ['f6'] = alert(1);",
            "}"),
        lines(
            "/** @unrestricted */",
            "class a {", //
            "  static a = alert(2);",
            "  f2 = 'hi';",
            "  f3 = 5;",
            "  4 = 4;",
            "  f6 = alert(1);",
            "}"));
  }
}
