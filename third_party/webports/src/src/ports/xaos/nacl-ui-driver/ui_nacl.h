/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef UI_NACL_H

#include "ppapi/c/ppb_input_event.h"


#define BITS_PER_PIXEL 32
#define BYTES_PER_PIXEL (BITS_PER_PIXEL/8)

/* Get screen width and height */
int GetWidth();
int GetHeight();

/* flush video buffer to screen */
void CopyImageDataToVideo(void* data);


struct PpapiEvent {
  PP_InputEvent_Type type;
  PP_InputEvent_MouseButton button;
  struct PP_Point position;
  int clicks;
};

/* get next ppapi event to process, may block if wait == 1 */
struct PpapiEvent* GetEvent(int wait);

/* these headers need an sdk update before they can be used */
#if 0
#include <nacl/nacl_check.h>
#include <nacl/nacl_log.h>
#else
#define CHECK(cond) do { if (!(cond)) {puts("ABORT: " #cond "\n"); abort();}} while(0)
#define NaClLog(lev, ...)  fprintf(stderr, __VA_ARGS__)
#endif

#endif /* UI_NACL_H */
