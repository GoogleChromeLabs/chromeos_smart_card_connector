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

package com.google.javascript.jscomp.parsing.parser.util;

import com.google.javascript.jscomp.base.format.SimpleFormat;

/**
 * A range of positions in a source string.
 * Start is inclusive.
 * End is exclusive.
 */
public class SourceRange {

  public final SourcePosition start;
  public final SourcePosition end;

  public SourceRange(SourcePosition start, SourcePosition end) {
    this.start = start;
    this.end = end;
  }

  @Override
  public String toString() {
    return SimpleFormat.format("<%s - %s>", start, end);
  }
}
