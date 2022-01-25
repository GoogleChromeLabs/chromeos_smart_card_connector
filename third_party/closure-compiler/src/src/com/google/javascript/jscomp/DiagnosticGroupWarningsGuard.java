/*
 * Copyright 2008 The Closure Compiler Authors.
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

import com.google.javascript.jscomp.base.Tri;
import javax.annotation.Nullable;

/** Sets the level for a particular DiagnosticGroup. */
public final class DiagnosticGroupWarningsGuard extends WarningsGuard {
  private static final long serialVersionUID = 1L;

  private final DiagnosticGroup group;
  private final CheckLevel level;

  public DiagnosticGroupWarningsGuard(
      DiagnosticGroup group, CheckLevel level) {
    this.group = group;
    this.level = level;
  }

  @Override
  @Nullable
  public CheckLevel level(JSError error) {
    return this.group.matches(error) ? this.level : null;
  }

  @Override
  public Tri mustRunChecks(DiagnosticGroup otherGroup) {
    if (this.level.isOn()) {
      return otherGroup.getTypes().stream().anyMatch(this.group::matches) ? Tri.TRUE : Tri.UNKNOWN;
    } else {
      return otherGroup.getTypes().stream().allMatch(this.group::matches) ? Tri.FALSE : Tri.UNKNOWN;
    }
  }

  @Override
  public String toString() {
    return group + "(" + level + ")";
  }
}
