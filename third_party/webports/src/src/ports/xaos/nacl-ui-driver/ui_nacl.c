/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "aconfig.h"

#include "ui_nacl.h"

/*includes */
#include <ui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* getcwd(char* buf, size_t size) {
  NaClLog(LOG_INFO, "getcwd\n");
  strcpy(buf, "/tmp");
  return buf;
}

int kill() {
  NaClLog(LOG_INFO, "kill\n");
  return -1;
}

struct {
  void* buffers[2];
  int current_buffer;
} VideoBuffers;

static void nacl_setpalette(ui_palette pal, int start, int end) {
  NaClLog(LOG_INFO, "nacl_setpalette\n");
}

static void nacl_print(int x, int y, CONST char* text) {
  if (text[0]) {
    NaClLog(LOG_INFO, "nacl_print [%s]\n", text);
  }
}

static void nacl_flush(void) {
  void* data = VideoBuffers.buffers[VideoBuffers.current_buffer];
  NaClLog(LOG_INFO, "nacl_flush %d %p\n", VideoBuffers.current_buffer, data);
  if (!data)
    return;
  CHECK(data != NULL);
  CopyImageDataToVideo(data);
}

static void nacl_display() {
  NaClLog(LOG_INFO, "nacl_display\n");
  nacl_flush();
}

static void nacl_flip_buffers() {
  NaClLog(LOG_INFO, "nacl_flip_buffers\n");
  VideoBuffers.current_buffer ^= 1;
}

static void nacl_free_buffers(char* b1, char* b2) {
  NaClLog(LOG_INFO, "nacl_free_buffers\n");
}

static int nacl_alloc_buffers(char** b1, char** b2) {
  NaClLog(LOG_INFO, "nacl_alloc_buffers\n");
  const int image_byte_size = GetHeight() * GetWidth() * BYTES_PER_PIXEL;

  NaClLog(LOG_INFO, "allocate xaos video buffers (size %d)\n", image_byte_size);
  VideoBuffers.buffers[0] = malloc(image_byte_size * 2);
  VideoBuffers.buffers[1] = malloc(image_byte_size * 2);

  CHECK(VideoBuffers.buffers[0] != NULL && VideoBuffers.buffers[1] != NULL);
  NaClLog(LOG_INFO, "buffer1 %p\n", VideoBuffers.buffers[0]);
  NaClLog(LOG_INFO, "buffer0 %p\n", VideoBuffers.buffers[1]);
  VideoBuffers.current_buffer = 0;

  *b1 = VideoBuffers.buffers[0];
  *b2 = VideoBuffers.buffers[1];
  return GetWidth() * BYTES_PER_PIXEL; /* scanline size */
}

static void nacl_getsize(int* w, int* h) {
  NaClLog(LOG_INFO, "nacl_getsize %d %d\n", GetWidth(), GetHeight());
  *w = GetWidth();
  *h = GetHeight();
}

static unsigned ButtonToMask(int button) {
  if (button == 0)
    return BUTTON1;
  if (button == 1)
    return BUTTON2;
  if (button == 2)
    return BUTTON3;
  NaClLog(LOG_ERROR, "unexpected button %d\n", button);
  return 0;
}

static void nacl_processevents(int wait, int* mx, int* my, int* mb, int* k) {
  static unsigned int mousebuttons = 0;
  static unsigned int mousex = 100;
  static unsigned int mousey = 0;
  static int iflag = 0; /* FIXEM*/

  struct PpapiEvent* event = GetEvent(wait);
  if (event != NULL) {
    /* only support mouse events for now */
    switch (event->type) {
      default:
        break;
      case PP_INPUTEVENT_TYPE_MOUSEDOWN:
        mousebuttons |= ButtonToMask(event->button);
        break;
      case PP_INPUTEVENT_TYPE_MOUSEUP:
        mousebuttons &= ~ButtonToMask(event->button);
        break;
      case PP_INPUTEVENT_TYPE_MOUSEMOVE:
        mousex = event->position.x;
        mousey = event->position.y;
        break;
    }
    free(event);
  }

  *mx = mousex;
  *my = mousey;
  *mb = mousebuttons;
  *k = iflag;
}

static void nacl_getmouse(int* x, int* y, int* b) {
  NaClLog(LOG_INFO, "nacl_getmouse\n");
}

static void nacl_mousetype(int type) {
  NaClLog(LOG_INFO, "nacl_mousetype\n");
}

/* ====================================================================== */

static int nacl_init() {
  NaClLog(LOG_INFO, "nacl_init\n");
  return 1; /*1 for success 0 for fail */
}

static void nacl_uninitialise() {
  NaClLog(LOG_INFO, "nacl_uninitialise\n");
}

static struct params params[] = {
    {"", P_HELP, NULL, "Template driver options:"},
    // {"-flag", P_SWITCH, &variable, "Example flag..."},
    {NULL, 0, NULL, NULL}};

struct ui_driver nacl_driver = {
    "Native Client",
    nacl_init,
    nacl_getsize,
    nacl_processevents,
    nacl_getmouse,
    nacl_uninitialise,
    NULL, /* nacl_set_color, You should implement just one */
    nacl_setpalette, /* nacl_setpalette, of these and add NULL as second */
    nacl_print,
    nacl_display,
    nacl_alloc_buffers,
    nacl_free_buffers,
    nacl_flip_buffers,
    NULL,           /* nacl_mousetype. This should be NULL */
    nacl_flush,     /* flush */
    8,              /* text width */
    8,              /* text height */
    params,         /* params */
    0,              /* flags...see ui.h */
    0.0, 0.0,       /* width/height of screen in centimeters */
    0, 0,           /* resolution of screen for windowed systems */
    UI_TRUECOLOR,   /* Image type */
    0, 255, 255,    /* start, end of palette and maximum allocatable */
    0x00ff0000,     /* Rgb mask */
    0x0000ff00,     /* rGb mask */
    0x000000ff      /* rgB mask */
};
