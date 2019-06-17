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
package com.google.javascript.refactoring.testing;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.io.Files;
import com.google.common.truth.Correspondence;
import com.google.javascript.jscomp.CommandLineRunner;
import com.google.javascript.jscomp.CompilerOptions;
import com.google.javascript.refactoring.ApplySuggestedFixes;
import com.google.javascript.refactoring.RefactoringDriver;
import com.google.javascript.refactoring.RefasterJsScanner;
import com.google.javascript.refactoring.SuggestedFix;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.List;

/** Utilities for testing RefasterJs templates. */
public final class RefasterJsTestUtils {

  /**
   * Performs refactoring using a RefasterJs template and asserts that result is as expected.
   *
   * @param refasterJsTemplate path of the file or resource containing the RefasterJs template to
   *     apply
   * @param testDataPathPrefix path prefix of the directory from which input and expected-output
   *     file will be read
   * @param originalFile file name of the JavaScript source file to apply the refaster template to
   * @param additionalSourceFiles list of additional source files to provide to the compiler (e.g.
   *     dependencies)
   * @param expectedFileChoices the expected result options of applying the specified template to
   *     {@code originalFile}
   * @throws IOException
   */
  public static void assertFileRefactoring(
      String refasterJsTemplate,
      String testDataPathPrefix,
      String originalFile,
      List<String> additionalSourceFiles,
      String... expectedFileChoices)
      throws IOException {
    RefasterJsScanner scanner = new RefasterJsScanner();
    scanner.loadRefasterJsTemplate(refasterJsTemplate);

    final String originalFilePath = testDataPathPrefix + File.separator + originalFile;

    ImmutableList.Builder<String> expectedCodeBuilder = ImmutableList.builder();
    for (String expectedFile : expectedFileChoices) {
      expectedCodeBuilder.add(slurpFile(testDataPathPrefix + File.separator + expectedFile));
    }
    final ImmutableList<String> expectedCode = expectedCodeBuilder.build();

    RefactoringDriver.Builder driverBuilder =
        new RefactoringDriver.Builder()
            .addExterns(CommandLineRunner.getBuiltinExterns(CompilerOptions.Environment.BROWSER));

    for (String additionalSource : additionalSourceFiles) {
      driverBuilder.addInputsFromFile(testDataPathPrefix + File.separator + additionalSource);
    }
    RefactoringDriver driver = driverBuilder.addInputsFromFile(originalFilePath).build();

    List<SuggestedFix> fixes = driver.drive(scanner);
    assertThat(driver.getCompiler().getErrors()).isEmpty();
    assertThat(driver.getCompiler().getWarnings()).isEmpty();

    ImmutableList<String> newCode =
        ApplySuggestedFixes.applyAllSuggestedFixChoicesToCode(
                fixes, ImmutableMap.of(originalFilePath, slurpFile(originalFilePath)))
            .stream()
            .map(m -> m.get(originalFilePath))
            .collect(ImmutableList.toImmutableList());
    assertThat(newCode)
        .comparingElementsUsing(IGNORING_WHITESPACE_CORRESPONDENCE)
        .containsExactlyElementsIn(expectedCode);
  }

  private static String slurpFile(String originalFile) throws IOException {
    return Files.asCharSource(new File(originalFile), StandardCharsets.UTF_8).read();
  }

  private RefasterJsTestUtils() {}

  private static final Correspondence<String, String> IGNORING_WHITESPACE_CORRESPONDENCE =
      Correspondence.transforming(
          RefasterJsTestUtils::replaceTrailingWhitespace,
          RefasterJsTestUtils::replaceTrailingWhitespace,
          "equals (except for whitespace)");

  private static String replaceTrailingWhitespace(String contents) {
    return contents.replaceAll("[ \t]*\n", "\n");
  }
}
