// Copyright (c) 2009 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// NaCl Drawing demo
//   Uses Anti-Grain Geometry open source rendering library to render using
//   the Pepper Graphics2D interface and ppapi_simple.
//
//   See http://www.antigrain.com for more information on Anti-Grain Geometry

#include <errno.h>
#include <stdio.h>

#include <agg-2.5/agg_basics.h>
#include <agg-2.5/agg_conv_stroke.h>
#include <agg-2.5/agg_conv_transform.h>
#include <agg-2.5/agg_ellipse.h>
#include <agg-2.5/agg_path_storage.h>
#include <agg-2.5/agg_pixfmt_rgba.h>
#include <agg-2.5/agg_rendering_buffer.h>
#include <agg-2.5/agg_rasterizer_outline_aa.h>
#include <agg-2.5/agg_rasterizer_scanline_aa.h>
#include <agg-2.5/agg_renderer_scanline.h>
#include <agg-2.5/agg_scanline_u.h>

#include "ppapi_simple/ps_context_2d.h"
#include "ppapi_simple/ps_main.h"


// Drawing class holds information and functionality needed to render
class DrawingDemo {
 public:
  DrawingDemo(); ~DrawingDemo();
  void Display();
  void Update();
  bool PumpEvents();
  void Run();

 private:
  PSContext2D_t* ps_context_;
};


// This update loop is run once per frame.
// All of the Anti-Grain Geometry (AGG) rendering is done in this function.
// AGG renders straight into the DrawingDemo's ps_context_.
void DrawingDemo::Update() {
  unsigned int i;
  agg::rendering_buffer rbuf((unsigned char *)ps_context_->data,
                             ps_context_->width,
                             ps_context_->height,
                             ps_context_->stride);
  // Setup agg and clear the framebuffer.
  // Use Native Client's bgra pixel format.
  agg::pixfmt_bgra32 pixf(rbuf);
  typedef agg::renderer_base<agg::pixfmt_bgra32> ren_base;
  ren_base ren(pixf);
  ren.clear(agg::rgba(0, 0, 0));
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  ras.reset();
  ras.gamma(agg::gamma_none());

  // Draw an array of filled circles with a cycling color spectrum.
  const double spectrum_violet = 380.0;
  const double spectrum_red = 780.0;
  static double outer_cycle = spectrum_violet;
  static double delta_outer_cycle = 0.4;
  double inner_cycle = outer_cycle;
  double delta_inner_cycle = 0.75;
  for (double y = 0.0; y <= ps_context_->height; y += 32.0) {
    for (double x = 0.0; x <= ps_context_->width; x += 32.0) {
      // Draw a small filled circle at x, y with radius 16
      // using inner_cycle as the fill color.
      agg::rgba color(inner_cycle, 1.0);
      agg::ellipse elp;
      elp.init(x, y, 16, 16, 80);
      ras.add_path(elp);
      agg::render_scanlines_aa_solid(ras, sl, ren, color);
      // Bounce color cycle between red & violet.
      inner_cycle += delta_inner_cycle;
      if ((inner_cycle > spectrum_red) || (inner_cycle < spectrum_violet)) {
        delta_inner_cycle = -delta_inner_cycle;
        inner_cycle += delta_inner_cycle;
      }
    }
  }
  // Bounce outer starting color between red & violet.
  outer_cycle += delta_outer_cycle;
  if ((outer_cycle > spectrum_red) || (outer_cycle < spectrum_violet)) {
    delta_outer_cycle = -delta_outer_cycle;
    outer_cycle += delta_outer_cycle;
  }

  // Draw a semi-translucent triangle over the background.
  // The triangle is drawn as an outline, using a pen width
  // of 24 pixels.  The call to close_polygon() ensures that
  // all three corners are cleanly mitered.
  agg::path_storage ps;
  agg::conv_stroke<agg::path_storage> pg(ps);
  pg.width(24.0);
  ps.remove_all();
  ps.move_to(96, 160);
  ps.line_to(384, 256);
  ps.line_to(192, 352);
  ps.line_to(96, 160);
  ps.close_polygon();
  ras.add_path(pg);
  agg::render_scanlines_aa_solid(ras, sl, ren, agg::rgba8(255, 255, 255, 160));
}


// Displays software rendered image on the screen
void DrawingDemo::Display() {
}


// Pumps events and services them.
bool DrawingDemo::PumpEvents() {
  PSEvent* ps_event;
  while ((ps_event = PSEventTryAcquire()) != NULL) {
    PSContext2DHandleEvent(ps_context_, ps_event);
    PSEventRelease(ps_event);
  }
  return true;
}


// Sets up and initializes DrawingDemo.
DrawingDemo::DrawingDemo() {
  PSEventSetFilter(PSE_ALL);
  ps_context_ = PSContext2DAllocate(PP_IMAGEDATAFORMAT_BGRA_PREMUL);

  // Wait for context to be bound
  PSEvent* ps_event;
  while ((ps_event = PSEventWaitAcquire()) != NULL) {
    PSContext2DHandleEvent(ps_context_, ps_event);
    if (ps_context_->bound)
      break;
  }
}


// Frees up resources.
DrawingDemo::~DrawingDemo() {
  PSContext2DFree(ps_context_);
}


// Runs the demo by looping forever and rendering frames.
void DrawingDemo::Run() {
  for (int i = 0; ; i++) {
    if (!PumpEvents())
      break;
    PSContext2DGetBuffer(ps_context_);
    Update();
    PSContext2DSwapBuffer(ps_context_);
    printf("Frame: %04d\b\b\b\b\b\b\b\b\b\b\b", i);
    fflush(stdout);
  }

  printf("\nDone\n");
}


int example_main(int argc, char **argv) {
  DrawingDemo demo;
  demo.Run();
  return 0;
}


PPAPI_SIMPLE_REGISTER_MAIN(example_main);
