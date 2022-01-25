/*
 * Copyright 2015 The Closure Compiler Authors.
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


import com.google.javascript.rhino.Node;
import com.google.javascript.rhino.StaticRef;
import com.google.javascript.rhino.StaticSlot;
import javax.annotation.Nullable;

/**
 * Used by {@code Scope} to store information about variables.
 */
public class Var extends AbstractVar<Scope, Var> implements StaticSlot, StaticRef {

  static final String ARGUMENTS = "arguments";
  static final String EXPORTS = "exports";

  Var(
      String name,
      Node nameNode,
      Scope scope,
      int index,
      CompilerInput input,
      @Nullable Node implicitGoogNamespaceDefinition) {
    super(name, nameNode, scope, index, input, implicitGoogNamespaceDefinition);
    if (nameNode != null) {
      switch (nameNode.getToken()) {
        case MODULE_BODY:
        case NAME:
        case IMPORT_STAR:
          break;
        default:
          throw new IllegalArgumentException("Invalid name node " + nameNode);
      }
    }
  }

  static Var createImplicitGoogNamespace(String name, Scope scope, Node definition) {
    return new Var(name, /* nameNode= */ null, scope, -1, /* input= */ null, definition);
  }

  @Override
  public String toString() {
    return "Var " + getName() + " @ " + getNameNode();
  }
}
