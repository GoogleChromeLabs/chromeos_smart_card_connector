/* Copyright (c) 2014 Google Inc. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#include <python2.7/Python.h>
#include <libtar.h>
#include <locale.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <errno.h>

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi_simple/ps_interface.h"

#include "nacl_io/nacl_io.h"
#include "ppapi_simple/ps_main.h"
#include "ppapi_simple/ps_instance.h"

#ifdef __pnacl__
#define DATA_FILE "pydata_pnacl.tar"
#else
#error "Unknown arch"
#endif

/* TODO(bradnelson): Switch ipython to use cli_main and drop this. */

static int setup_unix_environment() {
    int ret = umount("/");
    if (ret) {
      printf("unmounting root fs failed\n");
      return 1;
    }

    ret = mount("", "/", "memfs", 0, NULL);
    if (ret) {
      printf("mounting root fs failed\n");
      return 1;
    }

    const char* data_url = getenv("NACL_DATA_URL");
    if (!data_url)
      data_url = "./";
    mkdir("/mnt/http", 0777);
    ret = mount(data_url, "/mnt/http", "httpfs", 0, "allow_cross_origin_requests:true allow_credentials:false");
    if (ret) {
      printf("mounting http filesystem failed\n");
      return 1;
    }

    char filename[PATH_MAX];
    strcpy(filename, "/mnt/http/");
    strcat(filename, DATA_FILE);
    TAR* tar;
    ret = tar_open(&tar, filename, NULL, O_RDONLY, 0, 0);
    if (ret) {
      printf("error opening %s\n", filename);
      return 1;
    }

    mkdir("/lib", 0777);
    ret = tar_extract_all(tar, (char *)"/");
    if (ret) {
      printf("error extracting %s\n", filename);
      return 1;
    }

    ret = tar_close(tar);

    setenv("PYTHONHOME", "", 1);
    return 0;
}

extern "C" {

static PyObject * post_json_message(PyObject * self, PyObject * args) {
  char * stream;
  char * json;
  if (!PyArg_ParseTuple(args, "ss", &stream, &json)) {
    return NULL;
  }

  pp::VarDictionary message;
  message.Set("stream", pp::Var(stream));
  message.Set("json", pp::Var(json));

  const PPB_Messaging * messaging = PSInterfaceMessaging();
  PP_Instance instance = PSGetInstanceId();
  messaging->PostMessage(instance, message.pp_var());

  Py_RETURN_NONE;
}

static PyObject * acquire_json_message_wait(PyObject *self, PyObject * args) {
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }

  const PPB_Messaging * messaging = PSInterfaceMessaging();
  PP_Instance instance = PSGetInstanceId();
  PSEventSetFilter(PSE_INSTANCE_HANDLEMESSAGE);
  PSEvent *event = PSEventWaitAcquire();
  if (event->type != PSE_INSTANCE_HANDLEMESSAGE)
    Py_RETURN_NONE;

  pp::Var message(event->as_var);

  if (!message.is_dictionary())
    Py_RETURN_NONE;

  pp::VarDictionary request(message);
  pp::Var json(request.Get("json"));
  if (!json.is_string())
    Py_RETURN_NONE;

  return PyString_FromString(json.AsString().c_str());
}

const PPB_Messaging *setup_ppapi_connection(PP_Instance *instance) {
  printf("Initializing external PPAPI signals for PyPPAPI\n");
  (*instance) = PSGetInstanceId();
  const PPB_Messaging *msg = PSInterfaceMessaging();
  if (msg == NULL) {
    printf("PPB Messaging is NULL, which will cause a failure.\n");
  }
  return msg;
}

}

static PyMethodDef PPMessageMethods[] = {
  {
    "_PostJSONMessage",
    post_json_message,
    METH_VARARGS,
    "Post a message encoded as JSON"
  },
  {
    "_AcquireJSONMessageWait",
    acquire_json_message_wait,
    METH_VARARGS,
    "Acquire a message encoded as JSON (blocking)"},
  {NULL, NULL, 0, NULL}
};

int ipython_kernel_main(int argc, char **argv) {
  printf("Setting up unix environment...\n");
  if (setup_unix_environment()) {
    printf("Error: %s\n", strerror(errno));
    return -1;
  }
  printf("done\n");

  // Initialize Pepper API
  PSInterfaceInit();

  int quit = 0;

  while(!quit) {
    // Initialize Python interpreter
    Py_Initialize();

    // Load module that provides access to Pepper messaging API
    // from within the interpreter
    Py_InitModule("ppmessage", PPMessageMethods);

     // Run the interpreter main loop.
    const char * main_filename = "/mnt/http/kernel.py";
    FILE *main = fopen(main_filename, "r");
    if (main == NULL) {
      printf("failed to load interpreter code\n");
      return -1;
    }

    quit = PyRun_SimpleFileEx(main, main_filename, 1);

    Py_Finalize();
  }

  return 0;
}

PPAPI_SIMPLE_REGISTER_MAIN(ipython_kernel_main)
