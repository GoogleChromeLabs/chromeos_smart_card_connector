/*
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Rhino code, released
 * May 6, 1999.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1997-1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Bob Jervis
 *   Google Inc.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 or later (the "GPL"), in which
 * case the provisions of the GPL are applicable instead of those above. If
 * you wish to allow use of your version of this file only under the terms of
 * the GPL and not to allow others to use your version of this file under the
 * MPL, indicate your decision by deleting the provisions above and replacing
 * them with the notice and other provisions required by the GPL. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * ***** END LICENSE BLOCK ***** */

package com.google.javascript.rhino;

import java.util.ArrayList;
import java.util.List;
import javax.annotation.Nullable;

/**
 * A simple {@link ErrorReporter} that collects warnings and errors and makes them accessible via
 * {@link #errors} and {@link #warnings}.
 */
public class SimpleErrorReporter implements ErrorReporter {
  @Nullable private List<String> warnings = null;
  @Nullable private List<String> errors = null;

  @Override
  public void warning(String message, String sourceName, int line, int lineOffset) {
    if (warnings == null) {
      warnings = new ArrayList<>();
    }
    warnings.add(formatDetailedMessage(message, sourceName, line));
  }

  @Override
  public void error(String message, String sourceName, int line, int lineOffset) {
    if (errors == null) {
      errors = new ArrayList<>();
    }
    errors.add(formatDetailedMessage(message, sourceName, line));
  }

  private static String formatDetailedMessage(String message, String sourceName, int lineNumber) {
    String details = message;
    if (sourceName == null || lineNumber <= 0) {
      return details;
    }
    StringBuilder buf = new StringBuilder(details);
    buf.append(" (");
    if (sourceName != null) {
      buf.append(sourceName);
    }
    if (lineNumber > 0) {
      buf.append('#');
      buf.append(lineNumber);
    }
    buf.append(')');
    return buf.toString();
  }
}
