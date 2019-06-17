/*
 * Copyright 2013 The Closure Compiler Authors.
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
import static com.google.common.base.Preconditions.checkState;

import com.google.common.annotations.VisibleForTesting;
import com.google.javascript.jscomp.NodeTraversal.AbstractPostOrderCallback;
import com.google.javascript.rhino.Node;

/**
 * Enforces that invocations of the method {@code goog.string.Const.from} are
 * done with an argument which is a string literal.
 *
 * <p>This function parameter checker enforces that for all invocations of
 * method {@code goog.string.Const.from} the actual argument satisfies one of
 * the following conditions:
 * <ol>
 * <li>The argument is an expression that is a string literal or concatenation
 * thereof, or
 * <li>The argument is a constant variable previously assigned from a string
 * literal or concatenation thereof.
 * </ol>
 */
class ConstParamCheck extends AbstractPostOrderCallback implements CompilerPass {

  private static final String CONST_FUNCTION_NAME = "goog.string.Const.from";
  private static final String CONST_FUNCTION_NAME_COLLAPSED = "goog$string$Const$from";

  @VisibleForTesting
  static final DiagnosticType CONST_NOT_STRING_LITERAL_ERROR =
      DiagnosticType.error("JSC_CONSTANT_NOT_STRING_LITERAL_ERROR",
          "Function argument is not a string literal or a constant assigned "
          + "from a string literal or a concatenation of these.");

  private final AbstractCompiler compiler;

  public ConstParamCheck(AbstractCompiler compiler) {
    this.compiler = checkNotNull(compiler);
  }

  @Override
  public void process(Node externs, Node root) {
    checkState(compiler.getLifeCycleStage().isNormalized());
    NodeTraversal.traverse(compiler, root, this);
  }

  /**
   * Callback to visit a node and check method call arguments of
   * {@code goog.string.Const.from}.
   *
   * @param traversal The node traversal object that supplies context, such as
   *        the scope chain to use in name lookups as well as error reporting.
   * @param node The node being visited.
   * @param parent The parent of the node.
   */
  @Override
  public void visit(NodeTraversal traversal, Node node, Node parent) {
    if (node.isCall()) {
      Node name = node.getFirstChild();
      Node argument = name.getNext();
      if (argument == null) {
        return;
      }

      // Detect calls to an aliased goog.string.Const.
      if (name.isName() && !name.matchesQualifiedName(CONST_FUNCTION_NAME_COLLAPSED)) {
        Scope scope = traversal.getScope();
        Var var = scope.getVar(name.getString());
        if (var == null) {
          return;
        }
        name = var.getInitialValue();
        if (name == null) {
          return;
        }
      }

      if (name.matchesQualifiedName(CONST_FUNCTION_NAME)
          || name.matchesQualifiedName(CONST_FUNCTION_NAME_COLLAPSED)) {
        if (!isSafeValue(traversal.getScope(), argument)) {
          compiler.report(JSError.make(argument, CONST_NOT_STRING_LITERAL_ERROR));
        }
      }
    }
  }

  /**
   * Checks if the method call argument is made of constant string literals.
   *
   * <p>This function argument checker will return true if:
   *
   * <ol>
   *   <li>The argument is a constant variable assigned from a string literal, or
   *   <li>The argument is an expression that is a string literal, or
   *   <li>The argument is a ternary expression choosing between string literals, or
   *   <li>The argument is a concatenation of the above.
   * </ol>
   *
   * @param scope The scope chain to use in name lookups.
   * @param argument The node of function argument to check.
   */
  private boolean isSafeValue(Scope scope, Node argument) {
    if (NodeUtil.isSomeCompileTimeConstStringValue(argument)) {
      return true;
    } else if (argument.isAdd()) {
      Node left = argument.getFirstChild();
      Node right = argument.getLastChild();
      return isSafeValue(scope, left) && isSafeValue(scope, right);
    } else if (argument.isName()) {
      String name = argument.getString();
      Var var = scope.getVar(name);
      if (var == null || !var.isInferredConst()) {
        return false;
      }
      Node initialValue = var.getInitialValue();
      if (initialValue == null) {
        return false;
      }
      return isSafeValue(var.getScope(), initialValue);
    }
    return false;
  }
}
