/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NACL_SPAWN_NACL_SPAWN_H_
#define NACL_SPAWN_NACL_SPAWN_H_

/*
 * Internal nacl_spawn functions
 */

#include <stdbool.h>
#include <sys/cdefs.h>

#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppb_var_array.h"
#include "ppapi/c/ppb_var_dictionary.h"

__BEGIN_DECLS

void nspawn_var_release(struct PP_Var var);

/* Array helpers */
struct PP_Var nspawn_array_create(void);
void nspawn_array_insertstring(struct PP_Var array, uint32_t index,
                               const char* value);
void nspawn_array_setstring(struct PP_Var array, uint32_t index,
                            const char* value);
void nspawn_array_appendstring(struct PP_Var array, const char* value);

/* Dictionary Helpers */
struct PP_Var nspawn_dict_create(void);
bool nspawn_dict_has_key(struct PP_Var dict, const char* key,
                         struct PP_Var* out_value);
struct PP_Var nspawn_dict_get(struct PP_Var dict, const char* key);
void nspawn_dict_set(struct PP_Var dict, const char* key,
                     struct PP_Var value_var);
void nspawn_dict_setstring(struct PP_Var dict, const char* key,
                           const char* value);
void nspawn_dict_setint(struct PP_Var dict_var, const char* key, int32_t v);
int nspawn_dict_getint(struct PP_Var dict_var, const char* key);
int nspawn_dict_getint_release(struct PP_Var dict_var, const char* key);
bool nspawn_dict_getbool(struct PP_Var dict_var, const char* key);

/* Sends a spawn/wait request to JavaScript and returns the result. */
struct PP_Var nspawn_send_request(struct PP_Var req_var);

int nspawn_setup_anonymous_pipes(void);

extern int nspawn_pid;
extern int nspawn_ppid;

__END_DECLS

#endif  /* NACL_SPAWN_NACL_SPAWN_H_ */
