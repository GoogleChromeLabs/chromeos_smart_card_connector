/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_SYS_STATVFS_H
#define GLIBCEMU_SYS_STATVFS_H

typedef long fsblkcnt_t;
typedef long fsfilcnt_t;

struct statvfs {
  unsigned long  f_bsize;    /* file system block size */
  unsigned long  f_frsize;   /* fragment size */
  fsblkcnt_t     f_blocks;   /* size of fs in f_frsize units */
  fsblkcnt_t     f_bfree;    /* # free blocks */
  fsblkcnt_t     f_bavail;   /* # free blocks for unprivileged users */
  fsfilcnt_t     f_files;    /* # inodes */
  fsfilcnt_t     f_ffree;    /* # free inodes */
  fsfilcnt_t     f_favail;   /* # free inodes for unprivileged users */
  unsigned long  f_fsid;     /* file system ID */
  unsigned long  f_flag;     /* mount flags */
  unsigned long  f_namemax;  /* maximum filename length */
};

int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);

#endif
