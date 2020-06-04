/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

/*
 * Sample set of libgit2 operations. The code in this file is generic
 * and should not depend on PPAPI or chrome (i.e. it should run just
 * run on desktop linux).
 */

#include "operations.h"

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <git2.h>

static void voutput(const char* message, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);

  // First send to stdout
  vfprintf(stdout, message, args);

  // Next send via postmessage
  char buffer[1024];
  vsnprintf(buffer, 1024, message, args_copy);
  post_message(buffer);
  va_end(args_copy);
}

void output(const char* message, ...) {
  va_list ap;
  va_start(ap, message);
  voutput(message, ap);
  va_end(ap);
}

static int transfer_progress(const git_transfer_progress* stats,
                             void* payload) {
  output("transfered: %d/%d %d KiB\n", stats->received_objects,
         stats->total_objects, stats->received_bytes / 1024);
  return 0;
}

static int status_callback(const char* path,
                           unsigned int flags,
                           void* payload) {
  output("%#x: %s\n", flags, path);
  return 0;
}

git_repository* init_repo(const char* repo_directory) {
  git_repository* repo = NULL;
  int rtn = git_repository_open(&repo, repo_directory);
  if (rtn != 0) {
    const git_error* err = giterr_last();
    output("git_repository_open failed %d [%d] %s\n", rtn, err->klass,
           err->message);
    return NULL;
  }

  return repo;
}

void do_git_status(const char* repo_directory) {
  time_t start = time(NULL);
  output("status: %s\n", repo_directory);
  git_repository* repo = init_repo(repo_directory);
  if (!repo)
    return;

  int rtn = git_status_foreach(repo, status_callback, NULL);
  if (rtn == 0) {
    output("status success [%d]\n", time(NULL) - start);
  } else {
    const git_error* err = giterr_last();
    output("status failed %d [%d] %s\n", rtn, err->klass, err->message);
  }

  git_repository_free(repo);

  output("%s contents:\n", repo_directory);
  DIR* dir = opendir(repo_directory);
  struct dirent* ent;
  while ((ent = readdir(dir)) != NULL) {
    output("  %s\n", ent->d_name);
  }
  closedir(dir);
}

void do_git_init(const char* repo_directory) {
  output("init new git repo: %s\n", repo_directory);
  git_repository* repo;
  git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
  options.flags = GIT_REPOSITORY_INIT_NO_REINIT | GIT_REPOSITORY_INIT_MKPATH;
  int rtn = git_repository_init_ext(&repo, repo_directory, &options);
  if (rtn == 0) {
    output("init success: %s\n", repo_directory);
  } else {
    const git_error* err = giterr_last();
    output("init failed %d [%d] %s\n", rtn, err->klass, err->message);
  }

  git_repository_free(repo);
}

void do_git_clone(const char* repo_directory, const char* url) {
  git_repository* repo;
  git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
  callbacks.transfer_progress = &transfer_progress;

  git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
  opts.remote_callbacks = callbacks;
  opts.ignore_cert_errors = 1;

  output("cloning %s -> %s\n", url, repo_directory);
  int rtn = git_clone(&repo, url, repo_directory, &opts);
  if (rtn == 0) {
    output("clone success\n");
    git_repository_free(repo);
  } else {
    const git_error* err = giterr_last();
    output("clone failed %d [%d] %s\n", rtn, err->klass, err->message);
  }
}

void do_git_push(const char* repo_directory, const char* refspec) {
  git_repository* repo = init_repo(repo_directory);
  if (!repo)
    return;

  git_push_options opts = GIT_PUSH_OPTIONS_INIT;
  git_push* push = NULL;
  git_remote* remote = NULL;
  const git_error* err = NULL;
  const char* failure = NULL;

  int rtn;
  rtn = git_remote_load(&remote, repo, "origin");
  if (rtn != 0) {
    failure = "git_remote_load";
    goto error;
  }

  output("pushing %s [%s]\n", repo_directory, refspec);
  rtn = git_push_new(&push, remote);
  if (rtn != 0) {
    failure = "git_push_new";
    goto error;
  }

  rtn = git_push_set_options(push, &opts);
  if (rtn != 0) {
    failure = "git_push_set_options";
    goto error;
  }

  rtn = git_push_add_refspec(push, refspec);
  if (rtn != 0) {
    failure = "git_push_add_refspec";
    goto error;
  }

  rtn = git_push_finish(push);
  if (rtn != 0) {
    failure = "git_push_finish";
    goto error;
  }

  rtn = git_push_unpack_ok(push);
  if (rtn == 0) {
    failure = "git_push_unpack_ok";
    goto error;
  }

  goto cleanup;

error:
  err = giterr_last();
  if (err) {
    output("%s failed %d [%d] %s\n", failure, rtn, err->klass, err->message);
  } else {
    output("%s failed %d <unknown error>\n", failure, rtn);
  }
  return;

cleanup:
  git_push_free(push);
  git_remote_free(remote);
  git_repository_free(repo);

  output("push success\n");
}

int index_matched_path_cb(const char* path,
                          const char* matched_pathspec,
                          void* payload) {
  output("update_all matched: %s\n", path);
  return 0;
}

void do_git_commit(const char* repo_directory, const char* filename) {
  const char kUserName[] = "Foo Bar";
  const char kUserEmail[] = "foobar@example.com";
  const size_t kFileSize = 1024 * 50;

  int rtn;
  char buffer[1024];
  git_signature* sig = NULL;
  const char* failure = NULL;
  git_repository* repo = init_repo(repo_directory);
  git_index* index = NULL;
  git_oid tree_id;
  git_oid head_id;
  git_commit* parent = NULL;
  git_oid commit_id;
  git_tree* tree = NULL;
  FILE* f = NULL;
  const git_error* err = NULL;

  if (!repo)
    return;

  output("committing %s (%d random bytes) as \"%s <%s>\"\n", filename,
         kFileSize, kUserName, kUserEmail);

  rtn = git_signature_now(&sig, kUserName, kUserEmail);
  if (rtn != 0) {
    failure = "git_signature_now";
    goto error;
  }

  rtn = git_repository_index(&index, repo);
  if (rtn != 0) {
    failure = "git_repository_index";
    goto error;
  }

  rtn = git_index_read(index, 1);
  if (rtn != 0) {
    failure = "git_index_read";
    goto error;
  }

  snprintf(buffer, 1024, "%s/%s", repo_directory, filename);

  f = fopen(buffer, "wb");
  if (!f) {
    failure = "fopen";
    goto error;
  }

  size_t size_left = kFileSize;
  while (size_left > 0) {
    int i;
    size_t buffer_len = size_left;
    if (buffer_len > 1024)
      buffer_len = 1024;

    for (i = 0; i < buffer_len; ++i) {
      buffer[i] = rand() % 256;
    }

    if (fwrite(&buffer, 1, buffer_len, f) != buffer_len) {
      failure = "fwrite";
      goto error;
    }
    size_left -= buffer_len;
  }

  fclose(f);
  f = NULL;

  const char* paths[] = {"*"};
  git_strarray arr = {(char**)paths, 1};
  rtn = git_index_add_all(index, &arr, GIT_INDEX_ADD_DEFAULT,
                          index_matched_path_cb, NULL);
  if (rtn != 0) {
    failure = "git_index_update_all";
    goto error;
  }

  rtn = git_index_write_tree(&tree_id, index);
  if (rtn != 0) {
    failure = "git_index_write_tree";
    goto error;
  }

  rtn = git_tree_lookup(&tree, repo, &tree_id);
  if (rtn != 0) {
    failure = "git_tree_lookup";
    goto error;
  }

  rtn = git_reference_name_to_id(&head_id, repo, "HEAD");
  if (rtn != 0) {
    failure = "git_reference_name_to_id";
    goto error;
  }

  rtn = git_commit_lookup(&parent, repo, &head_id);
  if (rtn != 0) {
    failure = "git_commit_lookup";
    goto error;
  }

  snprintf(buffer, 1024, "Add file %s", filename);

  rtn = git_commit_create_v(&commit_id, repo, "HEAD", sig, sig, NULL, buffer,
                            tree, 1, parent);
  if (rtn != 0) {
    failure = "git_commit_create_v";
    goto error;
  }

  goto cleanup;

error:
  err = giterr_last();
  if (err) {
    output("%s failed %d [%d] %s\n", failure, rtn, err->klass, err->message);
  } else {
    output("%s failed %d <unknown error>\n", failure, rtn);
  }

  return;

cleanup:
  if (f)
    fclose(f);

  git_commit_free(parent);
  git_tree_free(tree);
  git_index_free(index);
  git_repository_free(repo);
  git_signature_free(sig);

  output("commit success\n");
}
