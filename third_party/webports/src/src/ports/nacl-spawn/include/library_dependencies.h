// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NACL_SPAWN_LIBRARY_DEPENDENCIES_H_
#define NACL_SPAWN_LIBRARY_DEPENDENCIES_H_

#include <string>
#include <vector>

// Finds shared objects which are necessary to run |filename|.
// Also finds the architecture string |arch|.
// Output paths will be stored in |dependencies|. |filename| will be
// in |dependencies| if |filename| is dynamically linked. Otherwise,
// |dependencies| will be empty. Returns false and update errno
// appropriately on error.
bool nspawn_find_arch_and_library_deps(const std::string& filename,
                                       std::string* arch,
                                       std::vector<std::string>* dependencies);

#endif  // NACL_SPAWN_LIBRARY_DEPENDENCIES_H_
