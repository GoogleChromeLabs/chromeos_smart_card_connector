// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Emulates spawning/waiting process by asking JavaScript to do so.

// Include quoted spawn.h first so we can build in the presence of an installed
// copy of nacl-spawn.
#define IN_NACL_SPAWN_CC
#include "spawn.h"

#include "nacl_main.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <irt.h>
#include <irt_dev.h>
#include <libgen.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string>
#include <vector>

#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppb_var_array.h"
#include "ppapi/c/ppb_var_dictionary.h"

#include "ppapi_simple/ps_interface.h"

#include "library_dependencies.h"
#include "path_util.h"
#include "nacl_spawn.h"


extern char** environ;

int nspawn_pid;
int nspawn_ppid;

static std::string GetCwd() {
  char cwd[PATH_MAX] = ".";
  if (!getcwd(cwd, PATH_MAX)) {
    NACL_LOG("getcwd failed: %s\n", strerror(errno));
    assert(0);
  }
  return cwd;
}

static std::string GetAbsPath(const std::string& path) {
  assert(!path.empty());
  if (path[0] == '/')
    return path;
  else
    return GetCwd() + '/' + path;
}

// Adds a file into nmf. |key| is the key for open_resource IRT or
// "program". |filepath| is not a URL yet. JavaScript code is
// responsible to fix them. |arch| is the architecture string.
static void AddFileToNmf(const std::string& key,
                         const std::string& arch,
                         const std::string& filepath,
                         struct PP_Var dict_var) {

  struct PP_Var url_dict_var = nspawn_dict_create();
  nspawn_dict_setstring(url_dict_var, "url", filepath.c_str());

  struct PP_Var arch_dict_var = nspawn_dict_create();
  nspawn_dict_set(arch_dict_var, arch.c_str(), url_dict_var);

  nspawn_dict_set(dict_var, key.c_str(), arch_dict_var);
}

static void AddNmfToRequestForShared(
    std::string prog,
    const std::string& arch,
    const std::vector<std::string>& dependencies,
    struct PP_Var req_var) {
  struct PP_Var nmf_var = nspawn_dict_create();
  struct PP_Var files_var = nspawn_dict_create();
  const char* prog_base = basename(&prog[0]);
  for (size_t i = 0; i < dependencies.size(); i++) {
    std::string dep = dependencies[i];
    const std::string& abspath = GetAbsPath(dep);
    const char* base = basename(&dep[0]);
    // nacl_helper does not pass the name of program and the dynamic
    // loader always uses "main.nexe" as the main binary.
    if (strcmp(prog_base, base) == 0)
      base = "main.nexe";
    if (strcmp(base, "runnable-ld.so") == 0) {
      AddFileToNmf("program", arch, abspath, nmf_var);
    } else {
      AddFileToNmf(base, arch, abspath, files_var);
    }
  }

  nspawn_dict_set(nmf_var, "files", files_var);
  nspawn_dict_set(req_var, "nmf", nmf_var);
}

static void AddNmfToRequestForStatic(const std::string& prog,
                                     const std::string& arch,
                                     struct PP_Var req_var) {
  struct PP_Var nmf_var = nspawn_dict_create();
  AddFileToNmf("program", arch, GetAbsPath(prog), nmf_var);
  nspawn_dict_set(req_var, "nmf", nmf_var);
}

static void AddNmfToRequestForPNaCl(const std::string& prog,
                                    struct PP_Var req_var) {
  struct PP_Var url_dict_var = nspawn_dict_create();
  nspawn_dict_setstring(url_dict_var, "url", GetAbsPath(prog).c_str());

  struct PP_Var translate_dict_var = nspawn_dict_create();
  nspawn_dict_set(translate_dict_var, "pnacl-translate", url_dict_var);

  struct PP_Var arch_dict_var = nspawn_dict_create();
  nspawn_dict_set(arch_dict_var, "portable", translate_dict_var);

  struct PP_Var nmf_var = nspawn_dict_create();
  nspawn_dict_set(nmf_var, "program", arch_dict_var);

  nspawn_dict_set(req_var, "nmf", nmf_var);
}

static void FindInterpreter(std::string* path) {
  // Check if the path exists.
  if (access(path->c_str(), R_OK) == 0) {
    return;
  }
  // As /bin and /usr/bin are currently only mounted to a memory filesystem
  // in nacl_spawn, programs usually located there are installed to some other
  // location which is included in the PATH.
  // For now, do something non-standard.
  // If the program cannot be found at its full path, strip the program path
  // down to the basename and relying on later path search steps to find the
  // actual program location.
  size_t i = path->find_last_of('/');
  if (i == std::string::npos) {
    return;
  }
  *path = path->substr(i + 1);
}

static bool ExpandShBang(std::string* prog, struct PP_Var req_var) {
  // Open script.
  int fh = open(prog->c_str(), O_RDONLY);
  if (fh < 0) {
    return false;
  }
  // Read first 4k.
  char buffer[4096];
  ssize_t len = read(fh, buffer, sizeof buffer);
  if (len < 0) {
    close(fh);
    return false;
  }
  // Close script.
  if (close(fh) < 0) {
    return false;
  }
  // At least must have room for #!.
  if (len < 2) {
    errno = ENOEXEC;
    return false;
  }
  // Check if it's a script.
  if (memcmp(buffer, "#!", 2) != 0) {
    // Not a script.
    return true;
  }
  const char* start = buffer + 2;
  // Skip leading space
  while (start < buffer + len && *start == ' ') {
    ++start;
  }

  // Find the end of the line while also looking for the first space.
  // Mimicking Linux behavior, in which the first space marks a split point
  // where everything before is the interpreter path and everything after is
  // (including spaces) is treated as a single extra argument.
  const char* split = NULL;
  const char* end = start;

  while (buffer - end < len && *end != '\n' && *end != '\r') {
    if (*end == ' ' && split == NULL) {
      split = end;
    }
    ++end;
  }
  // Update command to run.
  struct PP_Var args_var = nspawn_dict_get(req_var, "args");
  assert(args_var.type == PP_VARTYPE_ARRAY);
  // Set argv[0] in case it was path expanded.
  nspawn_array_setstring(args_var, 0, prog->c_str());
  std::string interpreter;
  if (split) {
    interpreter = std::string(start, split - start);
    std::string arg(split + 1, end - (split + 1));
    nspawn_array_insertstring(args_var, 0, arg.c_str());
  } else {
    interpreter = std::string(start, end - start);
  }
  FindInterpreter(&interpreter);
  nspawn_array_insertstring(args_var, 0, interpreter.c_str());
  nspawn_var_release(args_var);
  *prog = interpreter;
  return true;
}

static bool UseBuiltInFallback(std::string* prog, struct PP_Var req_var) {
  if (prog->find('/') == std::string::npos) {
    const char* path_env = getenv("PATH");
    std::vector<std::string> paths;
    nspawn_get_paths(path_env, &paths);
    if (nspawn_find_in_paths(*prog, paths, prog)) {
      // Update argv[0] to match prog if we ended up changing it.
      struct PP_Var args_var = nspawn_dict_get(req_var, "args");
      assert(args_var.type == PP_VARTYPE_ARRAY);
      nspawn_array_setstring(args_var, 0, prog->c_str());
      nspawn_var_release(args_var);
    } else {
      // If the path does not contain a slash and we cannot find it
      // from PATH, we use NMF served with the JavaScript.
      return true;
    }
  }
  return false;
}

// Check if a file is a pnacl type file.
// If the file can't be read, return false.
static bool IsPNaClType(const std::string& filename) {
  // Open script.
  int fh = open(filename.c_str(), O_RDONLY);
  if (fh < 0) {
    // Default to nacl type if the file can't be read.
    return false;
  }
  // Read first 4 bytes.
  char buffer[4];
  ssize_t len = read(fh, buffer, sizeof buffer);
  close(fh);
  // Decide based on the header.
  return len == 4 && memcmp(buffer, "PEXE", sizeof buffer) == 0;
}

// Adds a NMF to the request if |prog| is stored in HTML5 filesystem.
static bool AddNmfToRequest(std::string prog, struct PP_Var req_var) {
  if (UseBuiltInFallback(&prog, req_var)) {
    return true;
  }
  if (access(prog.c_str(), R_OK) != 0) {
    errno = ENOENT;
    return false;
  }

  if (!ExpandShBang(&prog, req_var)) {
    return false;
  }

  // Check fallback again in case of #! expanded to something else.
  if (UseBuiltInFallback(&prog, req_var)) {
    return true;
  }

  // Check for pnacl.
  if (IsPNaClType(prog)) {
    AddNmfToRequestForPNaCl(prog, req_var);
    return true;
  }

  std::string arch;
  std::vector<std::string> dependencies;
  if (!nspawn_find_arch_and_library_deps(prog, &arch, &dependencies))
    return false;

  if (!dependencies.empty()) {
    AddNmfToRequestForShared(prog, arch, dependencies, req_var);
  } else  {
    // No dependencies means the main binary is statically linked.
    AddNmfToRequestForStatic(prog, arch, req_var);
  }

  return true;
}

static pid_t waitpid_impl(int pid, int* status, int options);

// TODO(bradnelson): Add sysconf means to query this in all libc's.
#define MAX_FILE_DESCRIPTOR 1000

static int CloneFileDescriptors(struct PP_Var envs_var) {
  int fd;
  int count = 0;

  for (fd = 0; fd < MAX_FILE_DESCRIPTOR; ++fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
      if (errno == EBADF) {
        continue;
      }
      return -1;
    }
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
      fprintf(stderr, "fcntl failed when spawning on descriptor %d: %s\n",
          fd, strerror(errno));
      return -1;
    }
    // Skip close on exec descriptors.
    if (flags & FD_CLOEXEC) {
      continue;
    }
    if (S_ISREG(st.st_mode)) {
      // TODO(bradnelson): Land nacl_io ioctl to support this.
    } else if (S_ISDIR(st.st_mode)) {
      // TODO(bradnelson): Land nacl_io ioctl to support this.
    } else if (S_ISCHR(st.st_mode)) {
      // Unsupported.
    } else if (S_ISBLK(st.st_mode)) {
      // Unsupported.
    } else if (S_ISFIFO(st.st_mode)) {
      char entry[100];
      snprintf(entry, sizeof entry,
          "NACL_SPAWN_FD_SETUP_%d=pipe:%d:%d:%d", count++, fd,
          static_cast<int>(st.st_ino), st.st_rdev == O_WRONLY);
      nspawn_array_appendstring(envs_var, entry);
    } else if (S_ISLNK(st.st_mode)) {
      // Unsupported.
    } else if (S_ISSOCK(st.st_mode)) {
      // Unsupported.
    }
  }
  return 0;
}

static void stash_file_descriptors(void) {
  int fd;

  for (fd = 0; fd < MAX_FILE_DESCRIPTOR; ++fd) {
    // TODO(bradnelson): Make this more robust if there are more than
    // MAX_FILE_DESCRIPTOR descriptors.
    if (dup2(fd, fd + MAX_FILE_DESCRIPTOR) < 0) {
      assert(errno == EBADF);
      continue;
    }
  }
}

static void unstash_file_descriptors(void) {
  int fd;

  for (fd = 0; fd < MAX_FILE_DESCRIPTOR; ++fd) {
    int alt_fd = fd + MAX_FILE_DESCRIPTOR;
    if (dup2(alt_fd, fd) < 0) {
      assert(errno == EBADF);
      continue;
    }
    close(alt_fd);
  }
}

NACL_SPAWN_TLS jmp_buf nacl_spawn_vfork_env;
static NACL_SPAWN_TLS pid_t vfork_pid = -1;
static NACL_SPAWN_TLS int vforking = 0;

// Shared spawnve implementation. Declared static so that shared library
// overrides doesn't break calls meant to be internal to this implementation.
static int spawnve_impl(int mode,
                        const char* path,
                        char* const argv[],
                        char* const envp[]) {
  if (NULL == path || NULL == argv[0]) {
    errno = EINVAL;
    return -1;
  }
  if (mode == P_WAIT) {
    int pid = spawnve_impl(P_NOWAIT, path, argv, envp);
    if (pid < 0) {
      return -1;
    }
    int status;
    int result = waitpid_impl(pid, &status, 0);
    if (result < 0) {
      return -1;
    }
    return status;
  } else if (mode == P_NOWAIT || mode == P_NOWAITO) {
    // The normal case.
  } else if (mode == P_OVERLAY) {
    if (vforking) {
      vfork_pid = spawnve_impl(P_NOWAIT, path, argv, envp);
      longjmp(nacl_spawn_vfork_env, 1);
    }
    // TODO(bradnelson): Add this by allowing javascript to replace the
    // existing module with a new one.
    errno = ENOSYS;
    return -1;
  } else {
    errno = EINVAL;
    return -1;
  }
  if (NULL == envp) {
    envp = environ;
  }

  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_spawn");

  struct PP_Var args_var = nspawn_array_create();
  for (int i = 0; argv[i]; i++)
    nspawn_array_setstring(args_var, i, argv[i]);
  nspawn_dict_set(req_var, "args", args_var);

  struct PP_Var envs_var = nspawn_array_create();
  for (int i = 0; envp[i]; i++)
    nspawn_array_setstring(envs_var, i, envp[i]);

  if (CloneFileDescriptors(envs_var) < 0) {
    return -1;
  }

  nspawn_dict_set(req_var, "envs", envs_var);
  nspawn_dict_setstring(req_var, "cwd", GetCwd().c_str());

  if (!AddNmfToRequest(path, req_var)) {
    errno = ENOENT;
    return -1;
  }

  return nspawn_dict_getint_release(nspawn_send_request(req_var), "pid");
}

// Spawn a new NaCl process. This is an alias for
// spawnve(mode, path, argv, NULL). Returns 0 on success. On error -1 is
// returned and errno will be set appropriately.
int spawnv(int mode, const char* path, char* const argv[]) {
  return spawnve_impl(mode, path, argv, NULL);
}

int spawnve(int mode, const char* path,
            char* const argv[], char* const envp[]) {
  return spawnve_impl(mode, path, argv, envp);
}

// Shared below by waitpid and wait.
// Done as a static so that users that replace waitpid and call wait (gcc)
// don't cause infinite recursion.
static pid_t waitpid_impl(int pid, int* status, int options) {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_wait");
  nspawn_dict_set(req_var, "pid", PP_MakeInt32(pid));
  nspawn_dict_set(req_var, "options", PP_MakeInt32(options));

  struct PP_Var result_var = nspawn_send_request(req_var);
  int result_pid = nspawn_dict_getint(result_var, "pid");

  // WEXITSTATUS(s) is defined as ((s >> 8) & 0xff).
  struct PP_Var status_var;
  if (nspawn_dict_has_key(result_var, "status", &status_var)) {
    int raw_status = status_var.value.as_int;
    *status = (raw_status & 0xff) << 8;
  }
  nspawn_var_release(result_var);
  return result_pid;
}

extern "C" {

#if defined(__GLIBC__)
pid_t wait(void* status) {
#else
pid_t wait(int* status) {
#endif
  return waitpid_impl(-1, static_cast<int*>(status), 0);
}

// Waits for the specified pid. The semantics of this function is as
// same as waitpid, though this implementation has some restrictions.
// Returns 0 on success. On error -1 is returned and errno will be set
// appropriately.
pid_t waitpid(pid_t pid, int* status, int options) {
  return waitpid_impl(pid, status, options);
}

// BSD wait variant with rusage.
#if defined(__BIONIC__)
pid_t wait3(int* status, int options, struct rusage* unused_rusage) {
#else
pid_t wait3(void* status, int options, struct rusage* unused_rusage) {
#endif
  return waitpid_impl(-1, static_cast<int*>(status), options);
}

// BSD wait variant with pid and rusage.
#if defined(__BIONIC__)
pid_t wait4(pid_t pid, int* status, int options,
            struct rusage* unused_rusage) {
#else
pid_t wait4(pid_t pid, void* status, int options,
            struct rusage* unused_rusage) {
#endif
  return waitpid_impl(pid, static_cast<int*>(status), options);
}

/*
 * Fake version of getpid().  This is used if there is no
 * nspawn_ppid set and no IRT getpid interface available.
 */
static int getpid_fake(int* pid) {
  *pid = 1;
  return 0;
}

static struct nacl_irt_dev_getpid irt_dev_getpid;

/*
 * IRT version of getpid().  This is used if there is no
 * nspawn_ppid set.
 */
static pid_t getpid_irt() {
  if (irt_dev_getpid.getpid == NULL) {
    int res = nacl_interface_query(NACL_IRT_DEV_GETPID_v0_1,
                                   &irt_dev_getpid,
                                   sizeof(irt_dev_getpid));
    if (res != sizeof(irt_dev_getpid)) {
      irt_dev_getpid.getpid = getpid_fake;
    }
  }

  int pid;
  int error = irt_dev_getpid.getpid(&pid);
  if (error != 0) {
    errno = error;
    return -1;
  }
  return pid;
}

// Get the process ID of the calling process.
pid_t getpid() {
  if (nspawn_pid == -1) {
    return getpid_irt();
  }
  return nspawn_pid;
}

// Get the process ID of the parent process.
pid_t getppid() {
  if (nspawn_ppid == -1) {
    errno = ENOSYS;
  }
  return nspawn_ppid;
}

// Spawn a process.
int posix_spawn(
    pid_t* pid, const char* path,
    const posix_spawn_file_actions_t* file_actions,
    const posix_spawnattr_t* attrp,
    char* const argv[], char* const envp[]) {
  int ret = spawnve_impl(P_NOWAIT, path, argv, envp);
  if (ret < 0) {
    return ret;
  }
  *pid = ret;
  return 0;
}

// Spawn a process using PATH to resolve.
int posix_spawnp(
    pid_t* pid, const char* file,
    const posix_spawn_file_actions_t* file_actions,
    const posix_spawnattr_t* attrp,
    char* const argv[], char* const envp[]) {
  // TODO(bradnelson): Make path expansion optional.
  return posix_spawn(pid, file, file_actions, attrp, argv, envp);
}

// Get the process group ID of the given process.
pid_t getpgid(pid_t pid) {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_getpgid");
  nspawn_dict_set(req_var, "pid", PP_MakeInt32(pid));

  return nspawn_dict_getint_release(nspawn_send_request(req_var), "pgid");
}

// Get the process group ID of the current process. This is an alias for
// getpgid(0).
pid_t getpgrp() {
  return getpgid(0);
}

// Set the process group ID of the given process.
pid_t setpgid(pid_t pid, pid_t pgid) {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_setpgid");
  nspawn_dict_set(req_var, "pid", PP_MakeInt32(pid));
  nspawn_dict_set(req_var, "pgid", PP_MakeInt32(pgid));

  return nspawn_dict_getint_release(nspawn_send_request(req_var), "result");
}

// Set the process group ID of the given process. This is an alias for
// setpgid(0, 0).
pid_t setpgrp() {
  return setpgid(0, 0);
}

// Get the session ID of the given process.
pid_t getsid(pid_t pid) {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_getsid");
  nspawn_dict_set(req_var, "pid", PP_MakeInt32(pid));
  return nspawn_dict_getint_release(nspawn_send_request(req_var), "sid");
}

// Make the current process a session leader.
pid_t setsid() {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_setsid");
  return nspawn_dict_getint_release(nspawn_send_request(req_var), "sid");
}

void jseval(const char* cmd, char** data, size_t* len) {
  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_jseval");
  nspawn_dict_setstring(req_var, "cmd", cmd);

  struct PP_Var result_dict_var = nspawn_send_request(req_var);
  struct PP_Var result_var = nspawn_dict_get(result_dict_var, "result");
  uint32_t result_len;
  const char* result = PSInterfaceVar()->VarToUtf8(result_var, &result_len);
  if (len) {
    *len = result_len;
  }
  if (data) {
    *data = static_cast<char*>(malloc(result_len + 1));
    assert(*data);
    memcpy(*data, result, result_len);
    (*data)[result_len] = '\0';
  }
  nspawn_var_release(result_var);
  nspawn_var_release(result_dict_var);
}

// Create a javascript pipe. pipefd[0] will be the read end of the pipe
// and pipefd[1] the write end of the pipe.
int nacl_spawn_pipe(int pipefd[2]) {
  if (pipefd == NULL) {
    errno = EFAULT;
    return -1;
  }

  struct PP_Var req_var = nspawn_dict_create();
  nspawn_dict_setstring(req_var, "command", "nacl_apipe");

  struct PP_Var result_var = nspawn_send_request(req_var);
  int id = nspawn_dict_getint(result_var, "pipe_id");
  nspawn_var_release(result_var);

  int read_fd;
  int write_fd;
  char path[100];
  sprintf(path, "/apipe/%d", id);
  read_fd = open(path, O_RDONLY);
  write_fd = open(path, O_WRONLY);
  if (read_fd < 0 || write_fd < 0) {
    if (read_fd >= 0) {
      close(read_fd);
    }
    if (write_fd >= 0) {
      close(write_fd);
    }
    return -1;
  }
  pipefd[0] = read_fd;
  pipefd[1] = write_fd;

  return 0;
}

void nacl_spawn_vfork_before(void) {
  assert(!vforking);
  vforking = 1;
  stash_file_descriptors();
}

pid_t nacl_spawn_vfork_after(int jmping) {
  if (jmping) {
    unstash_file_descriptors();
    vforking = 0;
    return vfork_pid;
  }
  return 0;
}

void nacl_spawn_vfork_exit(int status) {
  if (vforking) {
    struct PP_Var req_var = nspawn_dict_create();
    nspawn_dict_setstring(req_var, "command", "nacl_deadpid");
    nspawn_dict_set(req_var, "status", PP_MakeInt32(status));

    struct PP_Var response_var = nspawn_send_request(req_var);
    int result = nspawn_dict_getint_release(response_var, "pid");
    if (result < 0) {
      errno = -result;
      vfork_pid = -1;
    } else {
      vfork_pid = result;
    }
    longjmp(nacl_spawn_vfork_env, 1);
  } else {
    _exit(status);
  }
}

#define VARG_TO_ARGV_START \
  va_list vl; \
  va_start(vl, arg); \
  va_list vl_count; \
  va_copy(vl_count, vl); \
  int count = 1; \
  while (va_arg(vl_count, char*)) { \
    ++count; \
  } \
  va_end(vl_count); \
  /* Copying all the args into argv plus a trailing NULL */ \
  char** argv = static_cast<char**>(alloca(sizeof(char *) * (count + 1))); \
  argv[0] = const_cast<char*>(arg); \
  for (int i = 1; i <= count; i++) { \
    argv[i] = va_arg(vl, char*); \
  }

#define VARG_TO_ARGV \
  VARG_TO_ARGV_START; \
  va_end(vl);

#define VARG_TO_ARGV_ENVP \
  VARG_TO_ARGV_START; \
  char* const* envp = va_arg(vl, char* const*); \
  va_end(vl);

int execve(const char *filename, char *const argv[], char *const envp[]) {
  return spawnve_impl(P_OVERLAY, filename, argv, envp);
}

int execv(const char *path, char *const argv[]) {
  return spawnve_impl(P_OVERLAY, path, argv, environ);
}

int execvp(const char *file, char *const argv[]) {
  // TODO(bradnelson): Limit path resolution to 'p' variants.
  return spawnve_impl(P_OVERLAY, file, argv, environ);
}

int execvpe(const char *file, char *const argv[], char *const envp[]) {
  // TODO(bradnelson): Limit path resolution to 'p' variants.
  return spawnve_impl(P_OVERLAY, file, argv, envp);
}

int execl(const char *path, const char *arg, ...) {
  VARG_TO_ARGV;
  return spawnve_impl(P_OVERLAY, path, argv, environ);
}

int execlp(const char *file, const char *arg, ...) {
  VARG_TO_ARGV;
  // TODO(bradnelson): Limit path resolution to 'p' variants.
  return spawnve_impl(P_OVERLAY, file, argv, environ);
}

int execle(const char *path, const char *arg, ...) {  /* char* const envp[] */
  VARG_TO_ARGV_ENVP;
  return spawnve_impl(P_OVERLAY, path, argv, envp);
}

int execlpe(const char *path, const char *arg, ...) {  /* char* const envp[] */
  VARG_TO_ARGV_ENVP;
  // TODO(bradnelson): Limit path resolution to 'p' variants.
  return spawnve_impl(P_OVERLAY, path, argv, envp);
}

};  // extern "C"
