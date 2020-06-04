// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "path_util.h"

#include <unistd.h>

void nspawn_get_paths(const char* env, std::vector<std::string>* paths) {
  if (!env || !*env)
    return;
  for (const char* p = env; *p; p++) {
    if (*p == ':') {
      if (p == env)
        paths->push_back(".");
      else
        paths->push_back(std::string(env, p));
      env = p + 1;
    }
  }
  paths->push_back(env);
}

bool nspawn_find_in_paths(const std::string& basename,
                          const std::vector<std::string>& paths,
                          std::string* out_path) {
  for (size_t i = 0; i < paths.size(); i++) {
    const std::string path = paths[i] + '/' + basename;
    // We use this function for executables and shared objects, so
    // ideally we should use X_OK instead of R_OK. As nacl_io does not
    // support permissions well, we use R_OK for now.
    if (access(path.c_str(), R_OK) == 0) {
      *out_path = path;
      return true;
    }
  }
  return false;
}
