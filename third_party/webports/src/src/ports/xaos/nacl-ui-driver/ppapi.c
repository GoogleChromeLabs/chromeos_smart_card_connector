/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "aconfig.h"

/*includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_var.h"

#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/ppb_graphics_2d.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_view.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_input_event.h"
#include "ppapi/c/ppp_instance.h"

#include "ui_nacl.h"

#define kMaxEvents 1024
/* chrome cananot handle all that many refreshs */
#define kRefreshInterval 40

static struct {
  pthread_mutex_t mutex;
  pthread_cond_t condvar;
  int tail;
  int num;
  struct PpapiEvent* queue[kMaxEvents];
} EventQueue;

static struct {
  pthread_mutex_t flush_mutex;
  int width;
  int height;

  void* image_data;
  /* TODO make this atomic */
  int dirty;
  PP_Resource image;
  PP_Resource device;
} Video;

/*  zero initialized */
static struct {
  PPB_Core* if_core;
  PPB_Graphics2D* if_graphics_2d;
  PPB_ImageData* if_image_data;
  PPB_InputEvent* if_input_event;
  PPB_Instance* if_instance;
  PPB_MouseInputEvent* if_mouse_input_event;
  PPB_View* if_view;

  PP_Module module;
  PP_Instance instance;
  pthread_t tid;
  int num_instances;
  int num_viewchanges;
} Global;

int GetWidth() {
  return Video.width;
}

int GetHeight() {
  return Video.height;
}

extern int original_main(int argc, char* argv[]);

static void* ThreadForRunningXaosMain(void* arg) {
  char* argv[] = {"xaos", 0};
  NaClLog(LOG_INFO, "Xaos main started\n");
  original_main(1, argv);
  NaClLog(LOG_INFO, "Xaos main stopped\n");
  return 0;
}

extern struct PP_CompletionCallback ScreenUpdateCallback;

static void ScheduleScreenRefresh() {
  Global.if_core->CallOnMainThread(kRefreshInterval, ScreenUpdateCallback, 0);
}

static void FlushCallbackFun(void* user_data, int32_t result) {
  /* it is now safe to use the video buffer */
  Video.dirty = 0;
  pthread_mutex_unlock(&Video.flush_mutex);
  ScheduleScreenRefresh();
}

void CopyImageDataToVideo(void* data) {
  pthread_mutex_lock(&Video.flush_mutex);
  Video.dirty = 1;
  memcpy(Video.image_data, data, Video.width * Video.height * BYTES_PER_PIXEL);
  /* do not let anybody write into video buffer while flush in progress*/
  pthread_mutex_unlock(&Video.flush_mutex);
}

struct PP_CompletionCallback FlushCallback = {FlushCallbackFun, NULL};

void ScreenUpdateCallbackFun(void* user_data, int32_t result) {
  if (!Video.dirty) {
    ScheduleScreenRefresh();
    return;
  }

  pthread_mutex_lock(&Video.flush_mutex);
  struct PP_Point top_left = PP_MakePoint(0, 0);
  Global.if_graphics_2d->PaintImageData(Video.device, Video.image, &top_left,
                                        NULL);

  Global.if_graphics_2d->Flush(Video.device, FlushCallback);
}

struct PP_CompletionCallback ScreenUpdateCallback = {ScreenUpdateCallbackFun,
                                                     NULL};

static void InitEvents() {
  NaClLog(LOG_INFO, "initialize event queue\n");
  pthread_mutex_init(&EventQueue.mutex, NULL);
  pthread_cond_init(&EventQueue.condvar, NULL);
}

static void InitScreenRefresh(PP_Instance instance,
                              const struct PP_Size* size) {
  NaClLog(LOG_INFO, "initialize screen refresh\n");
  /* NOTE: these limits are not tight but there seem to be some
     not so well documented limitations inside xaos */
  CHECK(size->width <= 640);
  CHECK(size->height <= 480);

  Video.width = size->width;
  Video.height = size->height;

  NaClLog(LOG_INFO, "create PPAPI graphics device\n");
  Video.device = Global.if_graphics_2d->Create(instance, size, PP_FALSE);
  CHECK(Video.device != 0);
  NaClLog(LOG_INFO, "create PPAPI image");
  CHECK(Global.if_instance->BindGraphics(Global.instance, Video.device));
  Video.image = Global.if_image_data->Create(
      instance, PP_IMAGEDATAFORMAT_BGRA_PREMUL, size, PP_TRUE);
  CHECK(Video.image != 0);
  NaClLog(LOG_INFO, "map image into shared memory\n");
  Video.image_data = (void*)Global.if_image_data->Map(Video.image);
  CHECK(Video.image_data != NULL);
  NaClLog(LOG_INFO, "map is %p\n", Video.image_data);

  /* assert some simplifying assumptions */
  struct PP_ImageDataDesc desc;
  Global.if_image_data->Describe(Video.image, &desc);
  CHECK(desc.stride == size->width * BYTES_PER_PIXEL);

  pthread_mutex_init(&Video.flush_mutex, NULL);

  ScheduleScreenRefresh();
}

static void Init(PP_Instance instance, const struct PP_Size* size) {
  InitEvents();
  Global.if_input_event->RequestInputEvents(instance,
                                            PP_INPUTEVENT_CLASS_MOUSE);
  NaClLog(LOG_INFO, "allocate xaos video buffers\n");
  /* HORRIBLE HACK - TO AVOID SOME MEMORY CORRUPTION PROBLEMS */
  malloc(1024 * 1024);
  Global.instance = instance;
  InitScreenRefresh(instance, size);

  NaClLog(LOG_INFO, "spawn xaos main thread\n");
  int rv = pthread_create(&Global.tid, NULL, ThreadForRunningXaosMain, 0);
  if (rv != 0) {
    NaClLog(LOG_FATAL, "cannot spawn xaos thread\n");
  }
}

static PP_Bool DidCreate(PP_Instance instance,
                         uint32_t argc,
                         const char** argn,
                         const char** argv) {
  NaClLog(LOG_INFO, "DidCreate\n");
  if (Global.num_instances != 0) {
    NaClLog(LOG_FATAL, "only one instance supported\n");
  }
  ++Global.num_instances;

  return PP_TRUE;
}

static void DidDestroy(PP_Instance instance) {
  NaClLog(LOG_INFO, "DidDestroy\n");
  /* ignore this for now */
}

static void DidChangeView(PP_Instance instance, PP_Resource view) {
  ++Global.num_viewchanges;
  struct PP_Rect position;
  Global.if_view->GetRect(view, &position);

  const int width = position.size.width;
  const int height = position.size.height;

  NaClLog(LOG_INFO, "DidChangeView [%d] %d %d\n", Global.num_viewchanges, width,
          height);

  if (Global.num_viewchanges > 1) {
    NaClLog(LOG_INFO, "only first view change has any effect\n");
    return;
  }

  Init(instance, &position.size);
}

static void DidChangeFocus(PP_Instance instance, PP_Bool has_focus) {
  NaClLog(LOG_INFO, "DidChangeFocus\n");
  /* force a refresh */
  Video.dirty = 1;
}

static PP_Bool HandleInputEvent(PP_Instance instance, PP_Resource input_event) {
  NaClLog(LOG_INFO, "HandleInputEvent\n");
  if (!Global.if_mouse_input_event->IsMouseInputEvent(input_event)) {
    return PP_FALSE;
  }

  // XaoS has problems with Buttons numbered higher than the right button
  if (Global.if_mouse_input_event->GetButton(input_event) > 2) {
    return PP_FALSE;
  }

  struct PpapiEvent* event = (struct PpapiEvent*)malloc(sizeof *event);
  event->type = Global.if_input_event->GetType(input_event);
  event->button = Global.if_mouse_input_event->GetButton(input_event);
  event->position = Global.if_mouse_input_event->GetPosition(input_event);
  event->clicks = Global.if_mouse_input_event->GetClickCount(input_event);

  pthread_mutex_lock(&EventQueue.mutex);
  if (EventQueue.num >= kMaxEvents) {
    NaClLog(LOG_ERROR, "dropping events because of overflow\n");
  } else {
    int head = (EventQueue.tail + EventQueue.num) % kMaxEvents;
    EventQueue.queue[head] = event;
    ++EventQueue.num;
    if (EventQueue.num >= kMaxEvents)
      EventQueue.num -= kMaxEvents;
    pthread_cond_signal(&EventQueue.condvar);
  }

  pthread_mutex_unlock(&EventQueue.mutex);
  return PP_TRUE;
}

struct PpapiEvent* GetEvent(int wait) {
  struct PpapiEvent* event = NULL;
  pthread_mutex_lock(&EventQueue.mutex);
  if (EventQueue.num == 0 && wait) {
    pthread_cond_wait(&EventQueue.condvar, &EventQueue.mutex);
  }

  if (EventQueue.num > 0) {
    event = EventQueue.queue[EventQueue.tail];
    ++EventQueue.tail;
    if (EventQueue.tail >= kMaxEvents)
      EventQueue.tail -= kMaxEvents;
    --EventQueue.num;
  }
  pthread_mutex_unlock(&EventQueue.mutex);
  return event;
}

static PP_Bool HandleDocumentLoad(PP_Instance instance,
                                  PP_Resource url_loader) {
  NaClLog(LOG_INFO, "HandleDocumentLoad\n");
  /* ignore this for now */
  return PP_TRUE;
}

struct PP_Var GetInstanceObject(PP_Instance instance) {
  struct PP_Var var = {PP_VARTYPE_UNDEFINED};
  NaClLog(LOG_INFO, "GetInstanceObject\n");
  /* ignore this for now */
  return var;
}
/* ====================================================================== */
PP_EXPORT int32_t PPP_InitializeModule(PP_Module module_id,
                                       PPB_GetInterface get_browser_interface) {
  NaClLog(LOG_INFO, "PPP_InitializeModule\n");
  Global.module = module_id;
  Global.if_core = (PPB_Core*)get_browser_interface(PPB_CORE_INTERFACE);
  CHECK(Global.if_core != 0);
  Global.if_instance =
      (PPB_Instance*)get_browser_interface(PPB_INSTANCE_INTERFACE);
  CHECK(Global.if_instance != 0);
  Global.if_image_data =
      (PPB_ImageData*)get_browser_interface(PPB_IMAGEDATA_INTERFACE);
  CHECK(Global.if_image_data != 0);
  Global.if_graphics_2d =
      (PPB_Graphics2D*)get_browser_interface(PPB_GRAPHICS_2D_INTERFACE);
  CHECK(Global.if_graphics_2d != 0);
  Global.if_input_event =
      (PPB_InputEvent*)get_browser_interface(PPB_INPUT_EVENT_INTERFACE);
  CHECK(Global.if_input_event != 0);
  Global.if_mouse_input_event = (PPB_MouseInputEvent*)get_browser_interface(
      PPB_MOUSE_INPUT_EVENT_INTERFACE);
  CHECK(Global.if_input_event != 0);
  Global.if_view = (PPB_View*)get_browser_interface(PPB_VIEW_INTERFACE);
  CHECK(Global.if_view != 0);
  return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {
  NaClLog(LOG_INFO, "PPP_ShutdownModule\n");
}

static PPP_Instance GlobalInstanceInterface = {DidCreate,
                                               DidDestroy,
                                               DidChangeView,
                                               DidChangeFocus,
                                               HandleDocumentLoad};

static PPP_InputEvent GlobalInputEventInterface = {HandleInputEvent};

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  NaClLog(LOG_INFO, "PPP_GetInterface\n");
  if (0 == strncmp(PPP_INSTANCE_INTERFACE, interface_name,
                   strlen(PPP_INSTANCE_INTERFACE))) {
    return &GlobalInstanceInterface;
  }
  if (0 == strncmp(PPP_INPUT_EVENT_INTERFACE, interface_name,
                   strlen(PPP_INPUT_EVENT_INTERFACE))) {
    return &GlobalInputEventInterface;
  }
  return NULL;
}
