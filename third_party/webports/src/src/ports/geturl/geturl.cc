/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define __STDC_FORMAT_MACROS

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "nacl_main.h"

#include "ppapi_simple/ps.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_request_info.h"
#include "ppapi/cpp/url_response_info.h"

static bool IsTTY() {
  if (getenv("NO_TTY") != NULL)
    return false;
  return isatty(STDOUT_FILENO) != 0;
}

static int Download(int quiet, const char* url, const char* dst) {
  int32_t result;
  int percent_recieved = 0;
  int percent_recieved_previous = 0;

  pp::InstanceHandle instance(PSGetInstanceId());

  pp::URLRequestInfo url_request(instance);
  url_request.SetURL(url);
  url_request.SetMethod("GET");
  url_request.SetAllowCrossOriginRequests(true);
  url_request.SetRecordDownloadProgress(true);

  pp::URLLoader url_loader(instance);
  result = url_loader.Open(url_request, pp::BlockUntilComplete());
  if (result != PP_OK) {
    fprintf(stderr, "ERROR: Can't open url (%d): %s\n", result, url);
    return 1;
  }

  int fh = open(dst, O_WRONLY | O_CREAT);
  if (fh < 0) {
    fprintf(stderr, "ERROR: Can't open file (%d): %s\n", errno, dst);
    return 1;
  }

  char buffer[0x10000];
  ssize_t len;
  int64_t received = 0;
  int64_t total = 0;
  pp::URLResponseInfo info = url_loader.GetResponseInfo();
  if (info.GetStatusCode() != 200) {
    fprintf(stderr, "ERROR: got http error code %d for: %s\n",
        info.GetStatusCode(), url);
    close(fh);
    goto fail;
  }

  for (;;) {
    result = url_loader.ReadResponseBody(
        buffer, sizeof buffer, pp::BlockUntilComplete());
    if (result <= 0)
      break;
    len = write(fh, buffer, result);
    if (len != result) {
      fprintf(stderr, "ERROR: Failed writing to file (%d): %s\n", errno, dst);
      close(fh);
      goto fail;
    }
    url_loader.GetDownloadProgress(&received, &total);
    if (!quiet) {
      percent_recieved = received * 100 / total;
      if (IsTTY()) {
        printf("[%"PRId64"/%"PRId64" KiB %d%%]\r",
               received / 1024, total / 1024, percent_recieved);
        fflush(stdout);
      } else {
        if (percent_recieved / 10 > percent_recieved_previous / 10) {
          printf(".");
          percent_recieved_previous = percent_recieved;
        }
      }
    }
  } while (result > 0);
  if (!quiet) {
    if (IsTTY()) {
      printf("                                           \r");
      printf("[%"PRId64"/%"PRId64" KiB 100%%] Done.\n",
             received / 1024, total / 1024);
    } else {
        printf(" Done.\n");
    }
  }

  if (result != PP_OK) {
    fprintf(stderr, "ERROR: Failed downloading url (%d): %s\n", result, url);
    close(fh);
    goto fail;
  }

  if (close(fh) < 0) {
    fprintf(stderr, "ERROR: Failed closing file (%d): %s\n", errno, dst);
    goto fail;
  }

  return 0;

fail:
  result = remove(dst);
  if (result < 0) {
    fprintf(stderr, "ERROR: Failed removing file (%d): %s\n", errno, dst);
  }
  return 1;
}

int nacl_main(int argc, char *argv[]) {
  if (argc == 4 && strcmp(argv[1], "-q") == 0) {
    return Download(1, argv[2], argv[3]);
  } else if (argc == 3) {
    return Download(0, argv[1], argv[2]);
  }
  fprintf(stderr, "USAGE: %s [-q] <url> <dst>\n", argv[0]);
  fprintf(stderr, "\n");
  fprintf(stderr, "-q = quiet mode\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "NOTE: This utility can only be used to download URLs\n");
  fprintf(stderr, "from the same origin or that have been whitelisted\n");
  fprintf(stderr, "in an extension manifest\n");
  return 1;
}
