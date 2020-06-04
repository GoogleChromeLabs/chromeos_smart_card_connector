/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// This example uses the Mesa OpenGL rendering library to render into a
// Pepper 2D framebuffer.  It also demonstrates use of vertex buffer objects
// as specified in the OpenGL specification:
//     http://www.opengl.org/documentation/specs/
// Note that Mesa OpenGL provides software rendering and rasterization only.
// The only way to get hardware accelerated 3D graphics is through Pepper 3D and
// OpenGL ES 2.0.

#include <assert.h>
#include <errno.h>

#include <ppapi/c/pp_completion_callback.h>
#include <ppapi/c/pp_errors.h>
#include <ppapi/c/pp_instance.h>
#include <ppapi/c/pp_module.h>
#include <ppapi/c/pp_size.h>
#include <ppapi/c/pp_var.h>
#include <ppapi/c/ppb.h>
#include <ppapi/c/ppb_core.h>
#include <ppapi/c/ppb_graphics_2d.h>
#include <ppapi/c/ppb_image_data.h>
#include <ppapi/c/ppb_instance.h>
#include <ppapi/c/ppp.h>
#include <ppapi/c/ppp_instance.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/osmesa.h>


PPB_GetInterface g_get_browser_interface = NULL;

const PPB_Core* g_core_interface;
const PPB_Graphics2D* g_graphics_2d_interface;
const PPB_ImageData* g_image_data_interface;
const PPB_Instance* g_instance_interface;
struct GLDemo* gldemo;

// PPP_Instance implementation -----------------------------------------------

struct InstanceInfo {
  PP_Instance pp_instance;
  struct PP_Size last_size;

  struct InstanceInfo* next;
};

// Linked list of all live instances.
struct InstanceInfo* all_instances = NULL;


//-----------------------------------------------------------------------------
//
// The module code.  This section implements the application code.  There are
// three other sections that implement the Pepper module loading and browser
// bridging code.
//
//-----------------------------------------------------------------------------

// Returns a refed resource corresponding to the created device context.
PP_Resource CreateDeviceContext(PP_Instance instance,
                                const struct PP_Size* size) {
  PP_Resource device_context;
  device_context = g_graphics_2d_interface->Create(instance, size, PP_FALSE);
  if (!device_context)
    return 0;
  return device_context;
}

void BindDeviceContext(PP_Instance instance, PP_Resource device_context) {
  if (!g_instance_interface->BindGraphics(instance, device_context)) {
    g_core_interface->ReleaseResource(device_context);
  }
}

void FlushCompletionCallback(void* user_data, int32_t result) {
  // Don't need to do anything here.
}

// The Surface class owns a image resource and a Pepper 2D context, and owns the
// Mesa OpenGL context that renders into that bitmap.
class Surface {
 public:
  explicit Surface(struct InstanceInfo *instance);
  ~Surface();
  bool CreateContext(const struct PP_Size* size);
  void DestroyContext();
  bool MakeCurrentContext() const {
    return OSMesaMakeCurrent(mesa_context_,
                             pixels(),
                             GL_UNSIGNED_BYTE,
                             width(),
                             height()) == GL_TRUE;
  }
  bool IsContextValid() const {
    return static_cast<bool>(g_graphics_2d_interface->IsGraphics2D(context2d_));
  }
  void Flush();
  int width() const {
    return width_;
  }
  int height() const {
    return height_;
  }
  void* pixels() const {
    return static_cast<void *>(g_image_data_interface->Map(image_));
  }

 private:
  InstanceInfo* info_;
  int width_;
  int height_;
  PP_Resource image_;
  PP_Resource context2d_;  // The Pepper device context.
  // Mesa specific
  OSMesaContext mesa_context_;
};

Surface::Surface(InstanceInfo* info)
    : info_(info),
      width_(0),
      height_(0),
      image_(0),
      context2d_(0),
      mesa_context_(0) {
}

Surface::~Surface() {
  DestroyContext();
}

bool Surface::CreateContext(const PP_Size* size) {
  if (IsContextValid())
    return true;
  width_ = size->width;
  height_ = size->height;
  image_ = g_image_data_interface->Create(
      info_->pp_instance, PP_IMAGEDATAFORMAT_BGRA_PREMUL, size, PP_TRUE);
  context2d_ = CreateDeviceContext(info_->pp_instance, size);
  BindDeviceContext(info_->pp_instance, context2d_);

  // Create a Mesa OpenGL context, bind it to the Pepper 2D context.
  mesa_context_ = OSMesaCreateContext(OSMESA_BGRA, NULL);
  if (0 == mesa_context_) {
    printf("OSMesaCreateContext failed!\n");
    DestroyContext();
    return false;
  }
  if (!MakeCurrentContext()) {
    printf("OSMesaMakeCurrent failed!\n");
    DestroyContext();
    return false;
  }
  // check for vertex buffer object support in OpenGL
  const char *extensions = reinterpret_cast<const char*>
      (glGetString(GL_EXTENSIONS));
  printf("OpenGL:supported extensions: %s\n", extensions);
  if (NULL != strstr(extensions, "GL_ARB_vertex_buffer_object")) {
    printf("OpenGL: GL_ARB_vertex_buffer_object available.\n");
  } else {
    // vertex buffer objects aren't available...
    printf("OpenGL: GL_ARB_vertex_buffer_object is not available.\n");
    printf("Vertex buffer objects are required for this demo,\n");
    DestroyContext();
    return false;
  }
  printf("OpenGL: Mesa context created.\n");
  return true;
}

void Surface::DestroyContext() {
  if (0 != mesa_context_) {
    OSMesaDestroyContext(mesa_context_);
    mesa_context_ = 0;
  }
  printf("OpenGL: Mesa context destroyed.\n");
  if (!IsContextValid())
    return;
  g_core_interface->ReleaseResource(context2d_);
  printf("OpenGL: Device context released.\n");
  g_core_interface->ReleaseResource(image_);
  printf("OpenGL: Image context released.\n");
}

void Surface::Flush() {
  g_graphics_2d_interface->ReplaceContents(context2d_, image_);
  g_graphics_2d_interface->Flush(context2d_,
      PP_MakeCompletionCallback(&FlushCompletionCallback, NULL));
}

// GLDemo is an object that responds to calls from the browser to do the 3D
// rendering.
class GLDemo {
 public:
  explicit GLDemo(InstanceInfo* info) : surf_(new Surface(info)) {}
  ~GLDemo() {
    delete surf_;
  }

  void Display() {
    surf_->Flush();
  }
  // Build a simple vertex buffer object
  void Setup(int width, int height);

  // Called from the browser via Invoke() when the method name is "update".
  // All of the opengl rendering is done in this function.
  void Update();

 private:
  Surface* surf_;
  GLuint vbo_color_;
  GLuint vbo_vertex_;
};


void GLDemo::Setup(int width, int height) {
  PP_Size size;
  size.width = width;
  size.height = height;
  if (!surf_->CreateContext(&size)) {
    return;
  }
  const int num_vertices = 3;
  GLfloat triangle_colors[num_vertices * 3] = {
    1.0f, 0.0f, 0.0f,        // color0
    0.0f, 1.0f, 0.0f,        // color1
    0.0f, 0.0f, 1.0f,        // color2
  };
  GLfloat triangle_vertices[num_vertices * 3] = {
    0.0f, 1.0f, -2.0f,       // vertex0
    1.0f, -1.0f, -2.0f,      // vertex1
    -1.0f, -1.0f, -2.0f,     // vertex2
  };

  // build a vertex buffer object (vbo), copy triangle data
  glGenBuffers(1, &vbo_color_);
  printf("OpenGL:vbo_color_: %d\n", vbo_color_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * num_vertices * 3,
      triangle_colors, GL_STATIC_DRAW);
  // build a vertex buffer object, copy triangle data
  glGenBuffers(1, &vbo_vertex_);
  printf("OpenGL:vbo_vertex_: %d\n", vbo_vertex_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * num_vertices * 3,
      triangle_vertices, GL_STATIC_DRAW);
}

void GLDemo::Update() {
  if (!surf_->MakeCurrentContext()) {
    return;
  }
  // frame setup
  static float angle = 0.0f;
  glViewport(80, 0, 480, 480);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 100.0);
  glClearColor(0, 0, 0, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef(angle, 0.0f, 0.0f, 1.0f);
  angle = angle + 0.1f;
  glClear(GL_COLOR_BUFFER_BIT);
  // enable color & vertex arrays
  glEnable(GL_COLOR_ARRAY);
  glEnable(GL_VERTEX_ARRAY);
  // render the vertex buffer object (created in Setup)
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color_);
  glColorPointer(3, GL_FLOAT, 0, NULL);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex_);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  // disable color & vertex arrays
  glDisable(GL_COLOR_ARRAY);
  glDisable(GL_VERTEX_ARRAY);
  // make sure everything renders into the framebuffer
  glFlush();
}

//-----------------------------------------------------------------------------
//
// The scripting bridge code.
//
//-----------------------------------------------------------------------------

// Returns the info for the given instance, or NULL if it's not found.
struct InstanceInfo* FindInstance(PP_Instance instance) {
  struct InstanceInfo* cur = all_instances;
  while (cur) {
    if (cur->pp_instance == instance)
      return cur;
  }
  return NULL;
}

PP_Bool Instance_DidCreate(PP_Instance instance,
                           uint32_t argc,
                           const char* argn[],
                           const char* argv[]) {
  struct InstanceInfo* info =
      (struct InstanceInfo*)malloc(sizeof(struct InstanceInfo));
  info->pp_instance = instance;
  info->last_size.width = 0;
  info->last_size.height = 0;

  // Insert into linked list of live instances.
  info->next = all_instances;
  all_instances = info;

  gldemo = new GLDemo(info);
  return PP_TRUE;
}

void Instance_DidDestroy(PP_Instance instance) {
  // Find the matching item in the linked list, delete it, and patch the
  // links.
  struct InstanceInfo** prev_ptr = &all_instances;
  struct InstanceInfo* cur = all_instances;
  while (cur) {
    if (instance == cur->pp_instance) {
      *prev_ptr = cur->next;
      free(cur);
      return;
    }
    prev_ptr = &cur->next;
  }
}

void Instance_DidChangeView(PP_Instance pp_instance,
                            const struct PP_Rect* position,
                            const struct PP_Rect* clip) {

  struct InstanceInfo* info = FindInstance(pp_instance);
  if (!info)
    return;

  if (info->last_size.width != position->size.width ||
      info->last_size.height != position->size.height) {
    // Got a resize, repaint the plugin.
    gldemo->Setup(position->size.width, position->size.height);
    gldemo->Update();
    gldemo->Display();
    info->last_size.width = position->size.width;
    info->last_size.height = position->size.height;
  }
}

void Instance_DidChangeFocus(PP_Instance pp_instance, PP_Bool has_focus) {
}

PP_Bool Instance_HandleDocumentLoad(PP_Instance pp_instance,
                                    PP_Resource pp_url_loader) {
  return PP_FALSE;
}

static PPP_Instance_1_0 instance_interface = {
  &Instance_DidCreate,
  &Instance_DidDestroy,
  &Instance_DidChangeView,
  &Instance_DidChangeFocus,
  &Instance_HandleDocumentLoad,
};


// Global entrypoints --------------------------------------------------------

PP_EXPORT int32_t PPP_InitializeModule(PP_Module module,
                                       PPB_GetInterface get_browser_interface) {
  g_get_browser_interface = get_browser_interface;

  g_core_interface = (const PPB_Core*)
      get_browser_interface(PPB_CORE_INTERFACE);
  g_instance_interface = (const PPB_Instance*)
      get_browser_interface(PPB_INSTANCE_INTERFACE);
  g_image_data_interface = (const PPB_ImageData*)
      get_browser_interface(PPB_IMAGEDATA_INTERFACE);
  g_graphics_2d_interface = (const PPB_Graphics2D*)
      get_browser_interface(PPB_GRAPHICS_2D_INTERFACE);
  if (!g_core_interface || !g_instance_interface || !g_image_data_interface ||
      !g_graphics_2d_interface)
    return -1;
  return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE_1_0) == 0)
    return &instance_interface;
  return NULL;
}
