# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ifeq (,$(findstring $(TOOLCHAIN),glibc bionic))
LIBC = newlib
endif

CFLAGS += -Wno-implicit-function-declaration
CPPFLAGS += -Werror -Wno-unused-value -pthread
CPPFLAGS += -DNACL_SDK_VERSION=$(NACL_SDK_VERSION)
OUT := out
SOURCES := \
  src/alarm.c \
  src/getpass.c \
  src/err.c \
  src/fpending.c \
  src/dirfd.c \
  src/flock.c \
  src/fts.c \
  src/getprotobyname_r.c \
  src/getservbyname_r.c \
  src/herrno.c \
  src/herror.c \
  src/in6_addr.c \
  src/inet_addr.c \
  src/inet_pton.c \
  src/lockf.c \
  src/mktemp.c \
  src/ns_name.c \
  src/ns_netint.c \
  src/ns_samedomain.c \
  src/openat.c \
  src/qsort_r.c \
  src/random.c \
  src/realpath.c \
  src/res_comp.c \
  src/res_data.c \
  src/res_debug.c \
  src/res_init.c \
  src/res_libc.c \
  src/res_mkquery.c \
  src/res_query.c \
  src/res_send.c \
  src/setregid.c \
  src/tcgetpgrp.c \
  src/timegm.c \
  src/writev.c

OBJS := $(patsubst src/%.c,$(OUT)/%.o,$(SOURCES))

ifeq ($(V),1)
CXX_PREFIX =
CC_PREFIX =
AR_PREFIX =
else
CXX_PREFIX = @echo " CXX $@";
CC_PREFIX =  @echo " CC  $@";
AR_PREFIX =  @echo " AR  $@";
endif


ifeq ($(LIBC),newlib)
LIB = $(OUT)/libglibc-compat.a
CPPFLAGS += -Iinclude -Isrc -D_LIBC
endif

$(OUT)/%.o : src/%.c
	@mkdir -p $(OUT)
	$(CC_PREFIX)$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

$(OUT)/%.o : src/%.cc
	@mkdir -p $(OUT)
	$(CXX_PREFIX)$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS)

all: $(LIB) $(OUT)/glibc_compat_test

$(OUT)/libglibc-compat.a: $(OBJS)
	$(AR_PREFIX)$(AR) rcs $@ $^

TEST_LIBS=$(LIB) -lgtest -lpthread

$(OUT)/glibc_compat_test: $(OUT)/test.o $(LIB)
	@mkdir -p $(OUT)
	$(CXX_PREFIX)$(CXX) -o $@ $< -L$(OUT) $(LDFLAGS) $(TEST_LIBS)

test: $(OUT)/glibc_compat_test

clean:
	rm -rf $(OUT)

.PHONY: test clean all
