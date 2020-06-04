/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>

#include "nacl_spawn.h"

#include "ppapi_simple/ps_interface.h"
#include "ppapi_simple/ps.h"
#include "ppapi_simple/ps_instance.h"

void nspawn_var_release(struct PP_Var var) {
  PSInterfaceVar()->Release(var);
}

struct PP_Var nspawn_dict_create(void) {
  struct PP_Var ret = PSInterfaceVarDictionary()->Create();
  return ret;
}

bool nspawn_dict_has_key(struct PP_Var dict,
                                const char* key,
                                struct PP_Var* out_value) {
  assert(out_value);
  struct PP_Var key_var = PSInterfaceVar()->VarFromUtf8(key, strlen(key));
  bool has_value = PSInterfaceVarDictionary()->HasKey(dict, key_var);
  if (has_value) {
    *out_value = PSInterfaceVarDictionary()->Get(dict, key_var);
  }
  PSInterfaceVar()->Release(key_var);
  return has_value;
}

struct PP_Var nspawn_dict_get(struct PP_Var dict, const char* key) {
  struct PP_Var key_var = PSInterfaceVar()->VarFromUtf8(key, strlen(key));
  struct PP_Var ret = PSInterfaceVarDictionary()->Get(dict, key_var);
  PSInterfaceVar()->Release(key_var);
  return ret;
}

void nspawn_dict_set(struct PP_Var dict,
                      const char* key,
                      struct PP_Var value_var) {
  struct PP_Var key_var = PSInterfaceVar()->VarFromUtf8(key, strlen(key));
  PSInterfaceVarDictionary()->Set(dict, key_var, value_var);
  PSInterfaceVar()->Release(key_var);
  PSInterfaceVar()->Release(value_var);
}

void nspawn_dict_setstring(struct PP_Var dict,
                                   const char* key,
                                   const char* value) {
  struct PP_Var value_var = PSInterfaceVar()->VarFromUtf8(value, strlen(value));
  nspawn_dict_set(dict, key, value_var);
}

void nspawn_dict_setint(struct PP_Var dict_var,
                        const char* key,
                        int32_t v) {
  nspawn_dict_set(dict_var, key, PP_MakeInt32(v));
}

struct PP_Var nspawn_array_create(void) {
  struct PP_Var ret = PSInterfaceVarArray()->Create();
  return ret;
}

void nspawn_array_insert(struct PP_Var array,
                         uint32_t index,
                         struct PP_Var value_var) {
  uint32_t old_length = PSInterfaceVarArray()->GetLength(array);
  PSInterfaceVarArray()->SetLength(array, old_length + 1);

  for (uint32_t i = old_length; i > index; --i) {
    struct PP_Var from_var = PSInterfaceVarArray()->Get(array, i - 1);
    PSInterfaceVarArray()->Set(array, i, from_var);
    PSInterfaceVar()->Release(from_var);
  }
  PSInterfaceVarArray()->Set(array, index, value_var);
  PSInterfaceVar()->Release(value_var);
}

void nspawn_array_setstring(struct PP_Var array,
                            uint32_t index,
                            const char* value) {
  struct PP_Var value_var = PSInterfaceVar()->VarFromUtf8(value, strlen(value));
  PSInterfaceVarArray()->Set(array, index, value_var);
  PSInterfaceVar()->Release(value_var);
}

void nspawn_array_insertstring(struct PP_Var array,
                               uint32_t index,
                               const char* value) {
  struct PP_Var value_var = PSInterfaceVar()->VarFromUtf8(value, strlen(value));
  nspawn_array_insert(array, index, value_var);
}

void nspawn_array_appendstring(struct PP_Var array,
                               const char* value) {
  uint32_t index = PSInterfaceVarArray()->GetLength(array);
  nspawn_array_setstring(array, index, value);
}

int nspawn_dict_getint(struct PP_Var dict_var, const char* key) {
  struct PP_Var value_var;
  if (!nspawn_dict_has_key(dict_var, key, &value_var)) {
    return -1;
  }
  assert(value_var.type == PP_VARTYPE_INT32);
  int value = value_var.value.as_int;
  if (value < 0) {
    errno = -value;
    return -1;
  }
  return value;
}

bool nspawn_dict_getbool(struct PP_Var dict_var, const char* key) {
  struct PP_Var value_var;
  if (!nspawn_dict_has_key(dict_var, key, &value_var)) {
    return -1;
  }
  assert(value_var.type == PP_VARTYPE_BOOL);
  bool value = value_var.value.as_bool;
  return value;
}

int nspawn_dict_getint_release(struct PP_Var dict_var, const char* key) {
  int ret = nspawn_dict_getint(dict_var, key);
  nspawn_var_release(dict_var);
  return ret;
}

// Returns a unique request ID to make all request strings different
// from each other.
static int64_t get_request_id() {
  static int64_t req_id = 0;
  static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&mu);
  int64_t id = ++req_id;
  pthread_mutex_unlock(&mu);
  return id;
}

struct NaClSpawnReply {
  pthread_mutex_t mu;
  pthread_cond_t cond;

  struct PP_Var result_var;
};

/*
 * Handle reply from JavaScript. The key is the request string and the
 * value is Zero or positive on success or -errno on failure. The
 * user_data must be an instance of NaClSpawnReply.
 */
static void handle_reply(struct PP_Var key, struct PP_Var value,
                         void* user_data) {
  if (key.type != PP_VARTYPE_STRING || value.type != PP_VARTYPE_DICTIONARY) {
    fprintf(stderr, "Invalid parameter for handle_reply\n");
    fprintf(stderr, "key type=%d\n", key.type);
    fprintf(stderr, "value type=%d\n", value.type);
  }
  assert(key.type == PP_VARTYPE_STRING);
  assert(value.type == PP_VARTYPE_DICTIONARY);

  struct NaClSpawnReply* reply = (struct NaClSpawnReply*)user_data;
  pthread_mutex_lock(&reply->mu);

  PSInterfaceVar()->AddRef(value);
  reply->result_var = value;

  pthread_cond_signal(&reply->cond);
  pthread_mutex_unlock(&reply->mu);
}

struct PP_Var nspawn_send_request(struct PP_Var req_var) {
  /*
   * naclprocess.js is required in order send requests to JavasCript.
   * If NACL_PROCESS is not set in the environment then we assume it is
   * not present and exit early. Without this check we would block forever
   * waiting for a response for the JavaScript side.
   */
  const char* naclprocess = getenv("NACL_PROCESS");
  if (naclprocess == NULL) {
    fprintf(stderr, "nspawn_send_request called without NACL_PROCESS set\n");
    return PP_MakeNull();
  }

  int64_t id = get_request_id();
  char req_id[64];
  sprintf(req_id, "%lld", id);
  nspawn_dict_setstring(req_var, "id", req_id);

  struct NaClSpawnReply reply;
  pthread_mutex_init(&reply.mu, NULL);
  pthread_cond_init(&reply.cond, NULL);
  PSEventRegisterMessageHandler(req_id, &handle_reply, &reply);

  PSInterfaceMessaging()->PostMessage(PSGetInstanceId(), req_var);
  nspawn_var_release(req_var);

  pthread_mutex_lock(&reply.mu);
  /*
   * Wait for response for JavaScript.  This can block for an unbounded amount
   * of time (e.g. waiting for a response to waitpid).
   */
  int error = pthread_cond_wait(&reply.cond, &reply.mu);
  pthread_mutex_unlock(&reply.mu);

  pthread_cond_destroy(&reply.cond);
  pthread_mutex_destroy(&reply.mu);

  PSEventRegisterMessageHandler(req_id, NULL, &reply);

  if (error != 0) {
    fprintf(stderr, "nspawn_send_request: pthread_cond_timedwait: %s\n",
          strerror(error));
    return PP_MakeNull();
  }

  return reply.result_var;
}
