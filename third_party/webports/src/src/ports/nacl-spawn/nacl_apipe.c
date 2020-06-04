/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>

/* TODO(sbc): These types should really be forward declared in fuse.h */
struct statvfs;
struct fuse_pollhandle;
struct fuse_bufvec;

#include "nacl_io/fuse.h"
#include "nacl_io/nacl_io.h"
#include "ppapi_simple/ps_interface.h"

#include "nacl_spawn.h"

static struct fuse_operations anonymous_pipe_ops;

static int apipe_open(
    const char* path,
    struct fuse_file_info* info) {
  int id;
  if (sscanf(path, "/%d", &id) != 1) {
    return -ENOENT;
  }
  info->fh = id;
  info->nonseekable = 1;
  return 0;
}

static int apipe_read(
    const char* path, char* buf, size_t count, off_t offset,
    struct fuse_file_info* info) {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_apipe_read");
  nspawn_dict_setint(req_var, "pipe_id", info->fh);
  nspawn_dict_setint(req_var, "count", count);

  struct PP_Var result_var = nspawn_send_request(req_var);
  struct PP_Var data = nspawn_dict_get(result_var, "data");
  assert(data.type == PP_VARTYPE_ARRAY_BUFFER);
  uint32_t len;
  if (!PSInterfaceVarArrayBuffer()->ByteLength(data, &len)) {
    nspawn_var_release(data);
    nspawn_var_release(result_var);
    return -EIO;
  }
  void *p = PSInterfaceVarArrayBuffer()->Map(data);
  if (len > 0 && !p) {
    nspawn_var_release(data);
    nspawn_var_release(result_var);
    return -EIO;
  }
  assert(len <= count);
  memcpy(buf, p, len);
  PSInterfaceVarArrayBuffer()->Unmap(data);
  nspawn_var_release(data);
  nspawn_var_release(result_var);

  return len;
}

static int apipe_write(
    const char* path,
    const char* buf,
    size_t count,
    off_t offset,
    struct fuse_file_info* info) {
  if (count == 0) return 0;

  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_apipe_write");
  nspawn_dict_setint(req_var, "pipe_id", info->fh);
  struct PP_Var data = PSInterfaceVarArrayBuffer()->Create(count);
  if (data.type == PP_VARTYPE_NULL) return -EIO;
  void *p = PSInterfaceVarArrayBuffer()->Map(data);
  if (count > 0 && !p) {
    nspawn_var_release(data);
    nspawn_var_release(req_var);
    return -EIO;
  }
  memcpy(p, buf, count);
  PSInterfaceVarArrayBuffer()->Unmap(data);
  nspawn_dict_set(req_var, "data", data);

  struct PP_Var result_var = nspawn_send_request(req_var);
  int ret = nspawn_dict_getint(result_var, "count");
  nspawn_var_release(result_var);

  return ret;
}

static int apipe_release(const char* path, struct fuse_file_info* info) {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_apipe_close");
  nspawn_dict_setint(req_var, "pipe_id", info->fh);
  nspawn_dict_setint(req_var, "writer", info->flags == O_WRONLY);

  struct PP_Var result_var = nspawn_send_request(req_var);
  int ret = nspawn_dict_getint(result_var, "result");
  nspawn_var_release(result_var);

  return ret;
}

static int apipe_fgetattr(
    const char* path, struct stat* st, struct fuse_file_info* info) {
  memset(st, 0, sizeof(*st));
  st->st_ino = info->fh;
  st->st_mode = S_IFIFO | S_IRUSR | S_IWUSR;
  // TODO(bradnelson): Do something better.
  // Stashing away the open flags (not a great place).
  st->st_rdev = info->flags;
  return 0;
}

int nspawn_setup_anonymous_pipes(void) {
  const char fs_type[] = "anonymous_pipe";
  int result;

  anonymous_pipe_ops.open = apipe_open;
  anonymous_pipe_ops.read = apipe_read;
  anonymous_pipe_ops.write = apipe_write;
  anonymous_pipe_ops.release = apipe_release;
  anonymous_pipe_ops.fgetattr = apipe_fgetattr;

  result = nacl_io_register_fs_type(fs_type, &anonymous_pipe_ops);
  if (!result) {
    fprintf(stderr, "error: resigstering fstype '%s' failed.\n", fs_type);
    return 1;
  }
  mkdir("/apipe", 0777);
  result = mount("", "/apipe", fs_type, 0, NULL);
  if (result != 0) {
    fprintf(stderr, "error: mount of '%s' failed: %d.\n", fs_type, result);
    return 1;;
  }

  return 0;
}
