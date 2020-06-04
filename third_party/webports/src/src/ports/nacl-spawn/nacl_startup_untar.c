/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "nacl_main.h"

#include <assert.h>
#include <fcntl.h>
#include <libtar.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppapi_simple/ps.h"

#define MAX_HASH_LEN (10 * 1024)

struct hashfiles {
  char expected[PATH_MAX];
  char installed[PATH_MAX];
};

/*
 * Read file contents into buffer, and return number of bytes read.
 */
static ssize_t read_file(const char* filename, char* buffer, int buffer_len) {
  ssize_t bytes_read = 0;
  FILE* f = fopen(filename, "r");
  if (!f)
    return -1;
  while (bytes_read < buffer_len) {
    int read = fread(buffer + bytes_read, 1, buffer_len - bytes_read, f);
    if (read == 0) {
      if (!feof(f)) {
        fclose(f);
        return -1;
      }
      break;
    }
    bytes_read += read;
  }
  fclose(f);
  return bytes_read;
}

/*
 * Write file, and return number of bytes read.
 */
static ssize_t write_file(const char* filename, char* buffer, int buffer_len) {
  ssize_t bytes_written = 0;
  FILE* f = fopen(filename, "w");
  if (!f)
    return -1;
  while (bytes_written < buffer_len) {
    int written = fwrite(buffer + bytes_written, 1,
                         buffer_len - bytes_written, f);
    if (written == 0) {
      fclose(f);
      return -1;
    }
    bytes_written += written;
  }
  fclose(f);
  return bytes_written;
}

static void copy_hashfile(struct hashfiles* files) {
  char buffer[MAX_HASH_LEN];
  int len = read_file(files->expected, buffer, MAX_HASH_LEN);
  if (len == -1) {
    return;
  }
  NACL_LOG("nacl_startup_untar: writing hash: %s\n", files->installed);
  write_file(files->installed, buffer, len);
}

/*
 * Return true is the given tarfile has already been extracted
 * to the given location.  This is done by checking for the
 * existence of <tarfile>.hash, and then comparing it to
 * <root>/<tarfile>.installed_hash.  If either of these does
 * not exist, or if the content does not match then we assume
 * the tarfile needs to be extracted.
 */
static bool already_extracted(struct hashfiles* files) {
  char expected_hash[MAX_HASH_LEN];
  char installed_hash[MAX_HASH_LEN];
  ssize_t hash_len = read_file(files->expected, expected_hash, MAX_HASH_LEN);
  if (hash_len == -1) {
    // hash file could not be read
    NACL_LOG("nacl_startup_untar: hash file not found: %s\n", files->expected);
    return false;
  }

  if (read_file(files->installed, installed_hash, MAX_HASH_LEN) != hash_len) {
    // either installed hash could not be read, or size doesn't match.
    NACL_LOG("nacl_startup_untar: installed hash not found: %s\n",
             files->installed);
    return false;
  }

  if (strncmp(expected_hash, installed_hash, hash_len) != 0) {
    NACL_LOG("nacl_startup_untar: hash mismatch\n");
    return false;
  }

  return true;
}

int nacl_startup_untar(const char* argv0,
                       const char* tarfile,
                       const char* root) {
  int ret;
  TAR* tar;
  char filename[PATH_MAX];
  char* pos;
  struct stat statbuf;
  struct hashfiles files;

  if (PSGetInstanceId() == 0) {
    NACL_LOG("nacl_startup_untar: skipping untar; running in sel_ldr\n");
    return 0;
  }

  if (getenv("NACL_DEVENV") != NULL) {
    NACL_LOG("nacl_startup_untar: running in NaCl Dev Env\n");
    return 0;
  }

  NACL_LOG("nacl_startup_untar[%s]: %s -> %s\n", argv0, tarfile, root);

  /* First try relative to argv[0]. */
  strcpy(filename, argv0);
  pos = strrchr(filename, '/');
  if (pos) {
    pos[1] = '\0';
  } else {
    filename[0] = '\0';
  }
  strcat(filename, tarfile);
  if (stat(filename, &statbuf) != 0) {
    /* Fallback to /mnt/http. */
    strcpy(filename, "/mnt/http/");
    strcat(filename, tarfile);
  }

  strcpy(files.expected, filename);
  strcat(files.expected, ".hash");

  const char* basename = strrchr(filename, '/');
  if (!basename)
    basename = tarfile;

  strcpy(files.installed, root);
  strcat(files.installed, basename);
  strcat(files.installed, ".hash");

  if (already_extracted(&files)) {
    NACL_LOG("nacl_startup_untar: tar file already extracted: %s\n", filename);
    return 0;
  }

  ret = tar_open(&tar, filename, NULL, O_RDONLY, 0, 0);
  if (ret) {
    fprintf(stderr, "nacl_startup_untar: error opening %s\n", filename);
    return 1;
  }

  ret = tar_extract_all(tar, (char*)root);
  if (ret) {
    fprintf(stderr, "nacl_startup_untar: error extracting %s\n", filename);
    return 1;
  }

  ret = tar_close(tar);
  assert(ret == 0);

  copy_hashfile(&files);
  return 0;
}
