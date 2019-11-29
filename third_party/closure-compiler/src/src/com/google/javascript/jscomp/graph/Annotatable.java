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

package com.google.javascript.jscomp.graph;


/**
 * Object that has an annotation.
 */
public interface Annotatable {
  /**
   * Annotates a piece of information to the object.
   *
   * @param data Information to be annotated.
   */
  void setAnnotation(Annotation data);

  /**
   * Retrieves a piece of information that has been annotated.
   *
   * @return The annotation or <code>null</code> if the object has not been annotated.
   */
  @SuppressWarnings("unchecked")
  <A extends Annotation> A getAnnotation();
}
