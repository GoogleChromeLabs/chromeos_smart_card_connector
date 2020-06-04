# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="examples/playfile examples/hello_world"

# Without this openal is not detected
export LIBS="-lm"
