#ifndef OGSS_GRAPHICS_H
#define OGSS_GRAPHICS_H

#include <mruby.h>
#include <rayfork.h>

#include <ogss/drawable.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OGSS_PLATFORM_GLFW
#include <glad/glad.h>
#include <glfw/glfw3.h>
typedef GLFWwindow * rf_window_ref;
#else
typedef void * rf_window_ref;
#endif

typedef struct rf_graphics_config rf_graphics_config;

struct rf_graphics_config
{
  mrb_int                    width;
  mrb_int                    height;
  mrb_int                    frame_rate;
  mrb_int                    frame_count;
  mrb_int                    brightness;
  rf_context                 context;
  rf_render_batch            render_batch;
  rf_default_font            default_font;
  mrb_bool                   is_open;
  rf_window_ref              window;
  mrb_value                  title;
  rf_gfx_backend_init_data  *data;
  mrb_bool                   is_frozen;
  rf_texture2d               frozen_img;
  rf_render_texture2d        render_texture;
  rf_container               container;
};

rf_container *
mrb_get_graphics_container(mrb_state *mrb);

rf_sizef
mrb_get_graphics_size(mrb_state *mrb);

#ifdef __cplusplus
}
#endif

#endif
