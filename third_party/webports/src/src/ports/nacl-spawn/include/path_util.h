// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NACL_SPAWN_PATH_UTIL_H_
#define NACL_SPAWN_PATH_UTIL_H_

#include <string>
#include <vector>

// Parses path environment variables such as PATH or LD_LIBRARY_PATH.
void nspawn_get_paths(const char* env, std::vector<std::string>* paths);

// Gets a file for the specified basename in paths. Returns true on
// success and out_path will be updated. On failure, this function
// returns false and out_path will not be updated.
bool nspawn_find_in_paths(const std::string& basename,
                          const std::vector<std::string>& paths,
                          std::string* out_path);

#endif  // NACL_SPAWN_PATH_UTIL_H_
