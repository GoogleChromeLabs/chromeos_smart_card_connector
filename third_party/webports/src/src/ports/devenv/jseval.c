// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
  fprintf(stderr, "USAGE: jseval -e <cmd> [<outfile>]\n");
  fprintf(stderr, "       (eval a string)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "       jseval -f <cmd> [<outfile>]\n");
  fprintf(stderr, "       (eval contents of a file)\n");
  exit(1);
}

static char* read_file_z(const char* filename) {
  FILE* file;
  char* data;
  size_t len;
  size_t read_len;

  file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "ERROR: Can't read: %s\n", filename);
    exit(1);
  }
  fseek(file, 0, SEEK_END);
  len = ftell(file);
  data = malloc(len + 1);
  if (!data) {
    fprintf(stderr, "ERROR: Allocation failed reading: %s\n", filename);
    fclose(file);
    exit(1);
  }
  data[len] = '\0';
  fseek(file, 0, SEEK_SET);
  read_len = fread(data, 1, len, file);
  if (read_len != len) {
    fprintf(stderr, "ERROR: Failed reading: %s\n", filename);
    fclose(file);
    exit(1);
  }
  return data;
}

static void write_file(const void* data, size_t len, const char* filename) {
  FILE* file;
  size_t wrote_len;

  file = fopen(filename, "wb");
  if (!file) {
    fprintf(stderr, "ERROR: Can't write to: %s\n", filename);
    exit(1);
  }
  wrote_len = fwrite(data, 1, len, file);
  if (wrote_len != len) {
    fprintf(stderr, "ERROR: Failed writting to: %s\n", filename);
    fclose(file);
    exit(1);
  }
  fclose(file);
}

int nacl_main(int argc, char** argv) {
  char* indata = 0;
  const char* cmd;
  char* outdata;
  size_t outdata_len;

  if ((argc != 3 && argc != 4) ||
      (strcmp(argv[1], "-f") != 0 && strcmp(argv[1], "-e") != 0)) {
    usage();
    return 1;
  }

  if (strcmp(argv[1], "-f") == 0) {
    indata = read_file_z(argv[2]);
    cmd = indata;
  } else {
    cmd = argv[2];
  }

  if (argc == 4) {
    jseval(cmd, &outdata, &outdata_len);
    write_file(outdata, outdata_len, argv[3]);
  } else {
    jseval(cmd, NULL, NULL);
  }

  if (strcmp(argv[1], "-f") == 0) {
    free(indata);
  }

  return 0;
}
