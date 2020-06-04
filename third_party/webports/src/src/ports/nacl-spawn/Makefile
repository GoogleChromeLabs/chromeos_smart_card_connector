# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(hamaji): include $NACL_SDK_ROOT/tools/common.mk.

NACL_SPAWN_OBJS = nacl_spawn.o nacl_setup_env.o path_util.o elf_reader.o \
                  library_dependencies.o nacl_apipe.o nacl_pp_helpers.o

TEST_EXES = test/unittests
LIBRARIES = libcli_main.a libnacl_spawn.a

CFLAGS += -std=gnu99
CPPFLAGS += -Wall -Werror -Iinclude
LDFLAGS += -lnacl_io

ifeq ($(NACL_SHARED),1)
LIBRARIES += libnacl_spawn.so
endif

ifeq ($(TOOLCHAIN),glibc)
TEST_EXES += test/elf_reader test/library_dependencies
TEST_BINARIES = test/test_exe test/libtest1.so test/libtest2.so test/libtest3.so
endif

all: $(LIBRARIES) $(TEST_EXES) $(TEST_BINARIES)

test: $(TEST_EXES) $(TEST_BINARIES)

# Targets for libcli_main

libcli_main.a: cli_main.o nacl_startup_untar.o
	rm -f $@
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Targets for libnacl_spawn

libnacl_spawn.so: $(NACL_SPAWN_OBJS)
	$(CXX) $(LDFLAGS) -shared $^ -o $@

libnacl_spawn.a: $(NACL_SPAWN_OBJS)
	rm -f $@
	$(AR) rcs $@ $^

%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Targets for tests

LIBS=-Xlinker -uPSUserMainGet -lcli_main -lnacl_spawn
LIBS+=-lgtest -lppapi_simple_cpp -lnacl_io -lppapi_cpp -lppapi

test/unittests: nacl_spawn_test.o $(LIBRARIES)
	$(CXX) -L. $(LDFLAGS) $< -o $@ $(LIBS)


ifeq ($(TOOLCHAIN),glibc)
test:
	./test/run_test.sh

test/elf_reader: elf_reader.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -DDEFINE_ELF_READER_MAIN $< -o $@
test/library_dependencies: elf_reader.o path_util.o library_dependencies.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -DDEFINE_LIBRARY_DEPENDENCIES_MAIN $^ -o $@

# We use -nostdlib not to have libc.so in their dependencies.
test/test_exe: test/test_exe.c test/libtest1.so test/libtest2.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -nostdlib -Wl,-rpath-link=test -Ltest -ltest1 -ltest2 -o $@
test/libtest1.so: test/libtest1.c test/libtest3.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -fPIC -nostdlib -shared -Ltest -ltest3 -o $@
test/libtest2.so: test/libtest2.c test/libtest3.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -fPIC -nostdlib -shared -Ltest -ltest3 -o $@
test/libtest3.so: test/libtest3.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -fPIC -nostdlib -shared -o $@
endif

clean:
	rm -f *.a *.o *.so $(TEST_EXES) $(TEST_BINARIES)

.PHONY: clean all test
