/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#include "operations.h"

#include <alloca.h>
#include <assert.h>
#include <errno.h>
#include <git2.h>
#include <git2/transport.h>
#include <nacl_main.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <ppapi/c/pp_errors.h>
#include <ppapi/c/pp_var.h>
#include <ppapi/c/ppb_messaging.h>
#include <ppapi/c/ppb_var_array.h>
#include <ppapi/c/ppb_var.h>
#include <ppapi_simple/ps.h>
#include <ppapi_simple/ps_event.h>

const struct PPB_Var_1_2* var_iface = NULL;
const struct PPB_VarArray_1_0* array_iface = NULL;
const struct PPB_Messaging_1_0* msg_iface = NULL;

void post_message(const char* message) {
  struct PP_Var var = var_iface->VarFromUtf8(message, strlen(message));
  msg_iface->PostMessage(PSGetInstanceId(), var);
  var_iface->Release(var);
}

void handle_cmd(const char* cmd, const char* repo_directory, const char* arg) {
  if (!strcmp(cmd, "clone")) {
    do_git_clone(repo_directory, arg);
  } else if (!strcmp(cmd, "push")) {
    do_git_push(repo_directory, arg);
  } else if (!strcmp(cmd, "commit")) {
    do_git_commit(repo_directory, arg);
  } else if (!strcmp(cmd, "init")) {
    do_git_init(repo_directory);
  } else if (!strcmp(cmd, "status")) {
    do_git_status(repo_directory);
  } else {
    output("Got unhandled cmd=%s\n", cmd);
  }
}

void do_mount(PP_Resource filesystem, const char* prefix, const char* dir) {
  char options[64];
  snprintf(options, 64, "filesystem_resource=%d,SOURCE=%s", filesystem, prefix);
  output("mounting filesystem at %s [%s]\n", dir, options);
  int rtn = mount("/", dir, "html5fs", 0, options);
  if (rtn != 0)
    output("mount failed: %s\n", strerror(errno));
  struct stat buf;
  char full[512];
  sprintf(full, "%s/gyp", dir);
  if (stat(full, &buf) != 0) {
    output("stat failed (%s): %s\n", full, strerror(errno));
    return;
  }
  output("st_mode %#x\n", buf.st_mode);
}

void handle_message(struct PP_Var message) {
  uint32_t len;
  char* cmd;
  char* repo_directory;
  const char* var_str;
  char* arg = NULL;
  struct PP_Var var;

  // Extract command from incoming array of strings
  if (message.type != PP_VARTYPE_ARRAY) {
    output("Got unexpected message type from js: %d\n", message.type);
    return;
  }

  var = array_iface->Get(message, 0);
  assert(var.type == PP_VARTYPE_STRING);
  var_str = var_iface->VarToUtf8(var, &len);
  cmd = alloca(len + 1);
  memcpy(cmd, var_str, len);
  cmd[len] = '\0';
  var_iface->Release(var);

  var = array_iface->Get(message, 1);
  assert(var.type == PP_VARTYPE_STRING);
  var_str = var_iface->VarToUtf8(var, &len);
  repo_directory = alloca(len + 1);
  memcpy(repo_directory, var_str, len);
  repo_directory[len] = '\0';
  var_iface->Release(var);

  if (array_iface->GetLength(message) > 2) {
    var = array_iface->Get(message, 2);
    if (!strcmp(cmd, "mount") && var.type == PP_VARTYPE_RESOURCE) {
      char* prefix;
      PP_Resource fs = var_iface->VarToResource(var);
      var = array_iface->Get(message, 3);
      var_str = var_iface->VarToUtf8(var, &len);
      prefix = alloca(len + 1);
      memcpy(prefix, var_str, len);
      prefix[len] = '\0';
      var_iface->Release(var);
      // special handling for 'mount' command
      do_mount(fs, prefix, repo_directory);
      return;
    }
    if (var.type != PP_VARTYPE_STRING) {
      output("Got unexpected arg type from js: %d\n", var.type);
      return;
    }
    var_str = var_iface->VarToUtf8(var, &len);
    arg = alloca(len + 1);
    memcpy(arg, var_str, len);
    arg[len] = '\0';
    var_iface->Release(var);
  }

  handle_cmd(cmd, repo_directory, arg);
}

git_smart_subtransport_definition pepper_http_subtransport_definition = {
    git_smart_subtransport_pepper_http,
    1 /* use rpc */
};

int nacl_main(int argc, char** argv) {
  srand(123);
  int rtn = git_threads_init();
  if (rtn) {
    output("git_threads_init failed: %d\n", rtn);
    return 1;
  }

  rtn = git_smart_subtransport_pepper_http_init(PSGetInstanceId(),
                                                PSGetInterface);
  if (rtn) {
    const git_error* err = giterr_last();
    output("git_smart_subtransport_pepper_http_init failed %d [%d] %s\n", rtn,
           err->klass, err->message);
    return 1;
  }

  git_transport_register("pepper_http://", 2, git_transport_smart,
                         &pepper_http_subtransport_definition);

  git_transport_register("pepper_https://", 2, git_transport_smart,
                         &pepper_http_subtransport_definition);

  var_iface = PSGetInterface(PPB_VAR_INTERFACE_1_2);
  array_iface = PSGetInterface(PPB_VAR_ARRAY_INTERFACE_1_0);
  msg_iface = PSGetInterface(PPB_MESSAGING_INTERFACE_1_0);
  assert(var_iface);
  assert(array_iface);
  assert(msg_iface);

  PSEventSetFilter(PSE_INSTANCE_HANDLEMESSAGE);
  while (1) {
    PSEvent* event = PSEventWaitAcquire();
    if (event->type != PSE_INSTANCE_HANDLEMESSAGE)
      output("unexpected message type: %d\n", event->type);
    handle_message(event->as_var);
    PSEventRelease(event);
  }
  return 0;
}
