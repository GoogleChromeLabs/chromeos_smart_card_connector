# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# sdl2-config --libs includes -lppapi which cannot be linked
# with shared libraries, so for now we disable the shared build.
# In the future we could instead filter out this flag or remove
# it and force SDL2 programs to add it themselves.
EXTRA_CONFIGURE_ARGS=--disable-shared
