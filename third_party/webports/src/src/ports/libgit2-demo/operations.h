/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */
#ifndef LIBGIT2_DEMO_OPERATIONS_H_
#define LIBGIT2_DEMO_OPERATIONS_H_

void post_message(const char* message);
void output(const char* message, ...);

void do_git_status(const char* repo_dir);
void do_git_clone(const char* repo_dir, const char* url);
void do_git_init(const char* repo_dir);
void do_git_push(const char* repo_dir, const char* refspec);
void do_git_commit(const char* repo_dir, const char* filename);

#endif
