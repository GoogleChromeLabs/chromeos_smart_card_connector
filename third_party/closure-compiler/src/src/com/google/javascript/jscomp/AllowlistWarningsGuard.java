/*
 * Copyright 2011 The Closure Compiler Authors.
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

import static com.google.common.base.Preconditions.checkNotNull;
import static java.nio.charset.StandardCharsets.UTF_8;

import com.google.common.annotations.GwtIncompatible;
import com.google.common.base.Joiner;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Iterables;
import com.google.common.collect.Multimap;
import com.google.common.collect.TreeMultimap;
import com.google.common.io.CharSource;
import com.google.common.io.CharStreams;
import com.google.common.io.Files;
import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.io.Reader;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Pattern;

/**
 * An extension of {@code WarningsGuard} that provides functionality to maintain a list of warnings
 * (allowlist). It is subclasses' responsibility to decide what to do with the allowlist by
 * implementing the {@code level} function. Warnings are defined by the name of the JS file and the
 * first line of warnings description.
 */
@GwtIncompatible("java.io, java.util.regex")
public class AllowlistWarningsGuard extends WarningsGuard {
  private static final Splitter LINE_SPLITTER = Splitter.on('\n');

  /** The set of allowlisted warnings, same format as {@code formatWarning}. */
  private final Set<String> allowlist;

  /** Pattern to match line number in error descriptions. */
  private static final Pattern LINE_NUMBER = Pattern.compile(":-?\\d+");

  public AllowlistWarningsGuard() {
    this(ImmutableSet.of());
  }

  /**
   * This class depends on an input set that contains the allowlist. The format of each allowlist
   * string is: {@code <file-name>:<line-number>? <warning-description>} {@code #
   * <optional-comment>}
   *
   * @param allowlist The set of JS-warnings that are allowlisted. This is expected to have similar
   *     format as {@code formatWarning(JSError)}.
   */
  public AllowlistWarningsGuard(Set<String> allowlist) {
    checkNotNull(allowlist);
    this.allowlist = normalizeAllowlist(allowlist);
  }

  /**
   * Loads legacy warnings list from the set of strings. During development line numbers are changed
   * very often - we just cut them and compare without ones.
   *
   * @return known legacy warnings without line numbers.
   */
  protected Set<String> normalizeAllowlist(Set<String> allowlist) {
    Set<String> result = new HashSet<>();
    for (String line : allowlist) {
      String trimmed = line.trim();
      if (trimmed.isEmpty() || trimmed.charAt(0) == '#') {
        // strip out empty lines and comments.
        continue;
      }

      // Strip line number for matching.
      result.add(LINE_NUMBER.matcher(trimmed).replaceFirst(":"));
    }
    return ImmutableSet.copyOf(result);
  }

  @Override
  public CheckLevel level(JSError error) {
    if (error.getDefaultLevel().equals(CheckLevel.ERROR)) {
      return null;
    }
    if (containWarning(formatWarning(error))) {
      // If the message matches the guard we use WARNING, so that it
      // - Shows up on stderr, and
      // - Gets caught by the AllowlistBuilder downstream in the pipeline
      return CheckLevel.WARNING;
    }
    return null;
  }
  /**
   * Determines whether a given warning is included in the allowlist.
   *
   * @param formattedWarning the warning formatted by {@code formattedWarning}
   * @return whether the given warning is allowlisted or not.
   */
  protected boolean containWarning(String formattedWarning) {
    return allowlist.contains(formattedWarning);
  }

  @Override
  public int getPriority() {
    return WarningsGuard.Priority.SUPPRESS_BY_ALLOWLIST.getValue();
  }

  /** Creates a warnings guard from a file. */
  public static AllowlistWarningsGuard fromFile(File file) {
    return new AllowlistWarningsGuard(loadAllowlistedJsWarnings(file));
  }

  /**
   * Loads legacy warnings list from the file.
   *
   * @return The lines of the file.
   */
  public static Set<String> loadAllowlistedJsWarnings(File file) {
    return loadAllowlistedJsWarnings(Files.asCharSource(file, UTF_8));
  }

  /**
   * Loads legacy warnings list from the file.
   *
   * @return The lines of the file.
   */
  protected static Set<String> loadAllowlistedJsWarnings(CharSource supplier) {
    try {
      return loadAllowlistedJsWarnings(supplier.openStream());
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  /**
   * Loads legacy warnings list from the file.
   *
   * @return The lines of the file.
   */
  // TODO(nicksantos): This is a weird API.
  static Set<String> loadAllowlistedJsWarnings(Reader reader) throws IOException {
    checkNotNull(reader);
    Set<String> result = new HashSet<>();

    result.addAll(CharStreams.readLines(reader));

    return result;
  }

  /**
   * If subclasses want to modify the formatting, they should override
   * #formatWarning(JSError, boolean), not this method.
   */
  protected String formatWarning(JSError error) {
    return formatWarning(error, false);
  }

  /**
   * @param withMetaData If true, include metadata that's useful to humans
   *     This metadata won't be used for matching the warning.
   */
  protected String formatWarning(JSError error, boolean withMetaData) {
    StringBuilder sb = new StringBuilder();
    sb.append(error.getSourceName()).append(":");
    if (withMetaData) {
      sb.append(error.getLineNumber());
    }
    List<String> lines = LINE_SPLITTER.splitToList(error.getDescription());
    sb.append("  ").append(lines.get(0));

    // Add the rest of the message as a comment.
    if (withMetaData) {
      for (int i = 1; i < lines.size(); i++) {
        sb.append("\n# ").append(lines.get(i));
      }
      sb.append("\n");
    }

    return sb.toString();
  }

  public static String getFirstLine(String warning) {
    int lineLength = warning.indexOf('\n');
    if (lineLength > 0) {
      warning = warning.substring(0, lineLength);
    }
    return warning;
  }

  /** Allowlist builder */
  public class AllowlistBuilder implements ErrorHandler {
    private final Set<JSError> warnings = new LinkedHashSet<>();
    private String productName = null;
    private String generatorTarget = null;
    private String headerNote = null;

    /** Fill in your product name to get a fun message! */
    public AllowlistBuilder setProductName(String name) {
      this.productName = name;
      return this;
    }

    /** Fill in instructions on how to generate this allowlist. */
    public AllowlistBuilder setGeneratorTarget(String name) {
      this.generatorTarget = name;
      return this;
    }

    /** A note to include at the top of the allowlist file. */
    public AllowlistBuilder setNote(String note) {
      this.headerNote  = note;
      return this;
    }

    @Override
    public void report(CheckLevel level, JSError error) {
      if (error.getDefaultLevel().equals(CheckLevel.ERROR)) {
        // ERROR-level diagnostics are ignored by AllowlistWarningsGuard (c.f. above getLevel).
        return;
      }
      warnings.add(error);
    }

    /**
     * Writes the warnings collected in a format that the AllowlistWarningsGuard can read back
     * later.
     */
    public void writeAllowlist(File out) throws IOException {
      try (PrintStream stream = new PrintStream(out)) {
        appendAllowlist(stream);
      }
    }

    /**
     * Writes the warnings collected in a format that the AllowlistWarningsGuard can read back
     * later.
     */
    public void appendAllowlist(PrintStream out) {
      out.append(
          "# This is a list of legacy warnings that have yet to be fixed.\n");

      if (productName != null && !productName.isEmpty() && !warnings.isEmpty()) {
        out.append("# Please find some time and fix at least one of them "
            + "and it will be the happiest day for " + productName + ".\n");
      }

      if (generatorTarget != null && !generatorTarget.isEmpty()) {
        out.append("# When you fix any of these warnings, run "
            + generatorTarget + " task.\n");
      }

      if (headerNote != null) {
        out.append("#" + Joiner.on("\n# ").join(Splitter.on('\n').split(headerNote)) + "\n");
      }

      Multimap<DiagnosticType, String> warningsByType = TreeMultimap.create();
      for (JSError warning : warnings) {
        warningsByType.put(
            warning.getType(),
            formatWarning(warning, true /* withLineNumber */));
      }

      for (DiagnosticType type : warningsByType.keySet()) {
        if (DiagnosticGroups.DEPRECATED.matches(type)) {
          // Deprecation warnings are not raisable to error, so we don't need them in allowlists.
          continue;
        }
        out.append("\n# Warning ")
            .append(type.key)
            .append(": ")
            .println(Iterables.get(LINE_SPLITTER.split(type.format), 0));

        for (String warning : warningsByType.get(type)) {
          out.println(warning);
        }
      }
      out.flush();
    }
  }
}
