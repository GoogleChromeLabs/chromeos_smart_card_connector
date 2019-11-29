/*
 * Copyright 2018 The Closure Compiler Authors.
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
package com.google.javascript.jscomp.gwt.client;

import com.google.gwt.core.client.EntryPoint;
import javax.annotation.Nullable;
import jsinterop.annotations.JsMethod;
import jsinterop.base.JsPropertyMap;

/** Entry point that exports just the JsfileParser. */
final class JsfileParserMainGwtEntryPoint implements EntryPoint {

  @Override
  public void onModuleLoad() {
    JsfileParserMain.exportGjd();
  }

  // We need to forward this function from a GWT only file so we can specify a custom namespace.
  @JsMethod(namespace = "jscomp")
  public static JsPropertyMap<Object> gjd(
      String code, String filename, @Nullable JsfileParserMain.Reporter reporter) {
    return JsfileParserMain.gjd(code, filename, reporter);
  }
}
