# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXEEXT ?= .nexe

SDL_LDFLAGS := $(shell sdl-config --libs)
SDL_CFLAGS := $(shell sdl-config --cflags)
LDFLAGS += $(SDL_LDFLAGS)
CFLAGS += -Wno-pointer-sign -Wno-implicit-function-declaration

CFLAGS += -fcommon -DSDL $(SDL_CFLAGS)
ifeq ($(TOOLCHAIN),pnacl)
# TODO(sbc): remove -fcommon once PNaCl toolchain changes its default
# https://code.google.com/p/nativeclient/issues/detail?id=3888
CFLAGS += -fcommon -Wno-dangling-else
endif

SDL_ROOT ?= /home/dmichael/naclports/src/packages/repository/SDL-1.2.14
NACL_TOOLCHAIN_ROOT ?= $(NACL_SDK_ROOT)/toolchain/linux_x86

X86_SRCS := snd_mixa.S sys_dosa.S d_draw.S d_draw16.S d_parta.S d_polysa.S \
            d_scana.S d_spr8.S d_varsa.S math.S r_aclipa.S r_aliasa.S \
            r_drawa.S r_edgea.S surf16.S surf8.S worlda.S

ifneq (V,1)
RUN_CC    = @echo " CC    " $@;
RUN_CCLD  = @echo " CCLD  " $@;
RUN_STRIP = @echo " STRIP " $@;
endif

NONX86_SRCS :=  d_vars.c nonintel.c

SOURCES += cd_nacl.c chase.c cl_demo.c cl_input.c cl_main.c cl_parse.c\
         cl_tent.c cmd.c common.c console.c crc.c cvar.c d_edge.c d_fill.c\
         d_init.c d_modech.c d_part.c d_polyse.c d_scan.c d_sky.c d_sprite.c\
         d_surf.c d_zpoint.c draw.c host.c host_cmd.c keys.c mathlib.c menu.c\
         model.c net_bsd.c net_dgrm.c net_loop.c net_main.c net_udp.c \
         net_vcr.c net_wso.c pr_cmds.c pr_edict.c pr_exec.c r_aclip.c\
         r_alias.c r_bsp.c r_draw.c r_edge.c r_efrag.c r_light.c r_main.c \
         r_misc.c r_part.c r_sky.c r_sprite.c r_surf.c r_vars.c \
         sbar.c screen.c snd_dma.c snd_mem.c snd_mix.c snd_sdl.c sv_main.c\
         sv_move.c sv_phys.c sv_user.c sys_sdl.c vid_sdl.c view.c \
         wad.c world.c zone.c

#ifeq ($(NACL_ARCH),x86_64)
#SOURCES += $(X86_SRCS)
#else
#ifeq ($(NACL_ARCH),i686)
#SOURCES += $(X86_SRCS)
#else
#SOURCES += $(NONX86_SRCS)
#endif
#endif

SOURCES += $(NONX86_SRCS)

# These files were excluded from FILES because they use instructions
# disallowed by Native Client
OMITTED_S_FILES := d_copy.S dosasm.S

OBJECTS := $(SOURCES:%.c=build/%.o)
OBJECTS := $(OBJECTS:%.S=build/%.o)

all: quake${EXEEXT}

quake_debug${EXEEXT}: $(OBJECTS)
	$(RUN_CCLD)$(CXX) $(LDFLAGS) $^ -o $@

quake${EXEEXT}: quake_debug${EXEEXT}
	$(RUN_STRIP)$(STRIP) -o $@ $<

clean:
	rm -f build/*.o *.${EXEEXT}

build/%.o: %.S
	@mkdir -p build
	$(RUN_CC)$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

build/%.o: %.c
	@mkdir -p build
	$(RUN_CC)$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<
