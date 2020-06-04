/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NACL_SPAWN_NACL_MAIN_H_
#define NACL_SPAWN_NACL_MAIN_H_

#include <spawn.h>

#include "nacl_io/log.h"

#if defined(NDEBUG)
#define NACL_LOG(format, ...)
#else
#define NACL_LOG(format, ...) nacl_io_log(format, ##__VA_ARGS__)
#endif

__BEGIN_DECLS

/*
 * Entry point expected by libcli_main.a
 */
int nacl_main(int argc, char* argv[]) __attribute__ ((visibility ("default")));

/*
 * Untar a startup bundle to a particular root.
 *
 * NOTE: This lives in libcli_main.a
 * Args:
 *   arg0: The contents of argv[0], used to determine relative tar location.
 *   tarfile: The name of a tarfile to extract.
 *   root: The absolute path to extract the startup tar file to.
 * Returns: 0 on success, non-zero on failure.
 */
int nacl_startup_untar(const char* argv0, const char* tarfile,
                       const char* root);

/*
 * Setup common environment variables and mounts.
 * Returns: 0 on success, non-zero on failure.
 */
int nacl_setup_env(void);

__END_DECLS

#endif /* NACL_SPAWN_NACL_MAIN_H_ */
