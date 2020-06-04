/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _GLIBCEMU_SYS_DIRENT_H
#define _GLIBCEMU_SYS_DIRENT_H

#include <sys/cdefs.h>
#include_next <sys/dirent.h>

/* Work around a dirent.h wrapping includes in extern "C". */
#if defined(_DIRENT_H_)
__BEGIN_DECLS
#endif

int dirfd(DIR *dirp);
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
int alphasort(const void *a, const void *b);
int scandir(const char *dirp, struct dirent ***namelist,
              int (*filter)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **));
DIR *fdopendir(int fd);

/* Work around a dirent.h wrapping includes in extern "C". */
#if defined(_DIRENT_H_)
__END_DECLS
#endif

#endif
