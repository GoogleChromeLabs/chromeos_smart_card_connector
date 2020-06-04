/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_MNTENT_H
#define GLIBCEMU_MNTENT_H 1

#include <stdio.h>
#include <sys/cdefs.h>

#define MNTOPT_DEFAULTS "defaults"  /* Use all default options.  */
#define MNTOPT_RO "ro"    /* Read only.  */
#define MNTOPT_RW "rw"    /* Read/write.  */
#define MNTOPT_SUID "suid"    /* Set uid allowed.  */
#define MNTOPT_NOSUID "nosuid"  /* No set uid allowed.  */
#define MNTOPT_NOAUTO "noauto"  /* Do not auto mount.  */

struct mntent {
  char *mnt_fsname;
  char *mnt_dir;
  char *mnt_type;
  char *mnt_opts;
  int   mnt_freq;
  int   mnt_passno;
};

__BEGIN_DECLS

FILE *setmntent(const char *filename, const char *type);

struct mntent *getmntent(FILE *fp);

int addmntent(FILE *fp, const struct mntent *mnt);

int endmntent(FILE *fp);

char *hasmntopt(const struct mntent *mnt, const char *opt);

__END_DECLS

#endif
