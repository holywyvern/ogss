#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/string.h>
#include <mruby/object.h>
#include <mruby/error.h>

#include <rayfork.h>

#include <orgf/alloc.h>
#include <orgf/bitmap.h>
#include <orgf/file.h>
#include <orgf/drawable.h>
#include <orgf/graphics.h>

#define CONFIG mrb_intern_lit(mrb, "#config")
#define TITLE mrb_intern_lit(mrb, "#title")
#define LAST_UPDATE mrb_intern_lit(mrb, "#last_update")

static void
config_free(mrb_state *mrb, void *p)
{
  if (p)
  {
    rf_graphics_config *config = p;
    mrb_container_free(mrb, &(config->container));
    mrb_free(mrb, p);
  }
}

static const mrb_data_type config_type = {
  "Graphics::Config", config_free
};

static inline mrb_value
get_config_obj(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, CONFIG);
}

static inline rf_graphics_config *
get_config(mrb_state *mrb, mrb_value self)
{
  mrb_value config_value = get_config_obj(mrb, self);
  rf_graphics_config *config;
  Data_Get_Struct(mrb, config_value, &config_type, config);
  config->title = mrb_iv_get(mrb, config_value, TITLE);
  return config;
}

rf_container *
mrb_get_graphics_container(mrb_state *mrb)
{
  mrb_value graphics = mrb_obj_value(mrb_module_get(mrb, "Graphics"));
  return &(get_config(mrb, graphics)->container);
}

mrb_float
mrb_get_dt(mrb_state *mrb)
{
  mrb_value graphics = mrb_obj_value(mrb_module_get(mrb, "Graphics"));
  return get_config(mrb, graphics)->dt;
}

rf_sizef
mrb_get_graphics_size(mrb_state *mrb)
{
  mrb_value graphics = mrb_obj_value(mrb_module_get(mrb, "Graphics"));
  rf_graphics_config *config = get_config(mrb, graphics);
  return (rf_sizef){ (float)config->width, (float)config->height };
}

static mrb_value
mrb_graphics_get_width(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  return mrb_fixnum_value(config->width);
}

static mrb_value
mrb_graphics_get_height(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  return mrb_fixnum_value(config->height);
}

static mrb_value
mrb_graphics_get_title(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  return config->title;
}

static mrb_value
mrb_graphics_set_title(mrb_state *mrb, mrb_value self)
{
  mrb_value value;
  mrb_get_args(mrb, "S", &value);
  mrb_iv_set(mrb, self, TITLE, value);
  rf_graphics_config *config = get_config(mrb, self);
  if (config->is_open)
  {
#ifdef ORGF_PLATFORM_GLFW
    glfwSetWindowTitle(config->window, mrb_str_to_cstr(mrb, value));
#endif
  }
  return value;
}

static mrb_value
mrb_graphics_resize_screen(mrb_state *mrb, mrb_value self)
{
  mrb_int width, height;
  rf_graphics_config *config = get_config(mrb, self);
  if (config->is_frozen) return mrb_nil_value();
  mrb_get_args(mrb, "ii", &width, &height);
  if (width < 1) mrb_raise(mrb, E_ARGUMENT_ERROR, "Width must be positive");
  if (height < 1) mrb_raise(mrb, E_ARGUMENT_ERROR, "Height must be positive");
  config->width = width;
  config->height = height;
  if (config->is_open)
  {
    rf_unload_render_texture(config->render_texture);
    config->render_texture = rf_load_render_texture((int)config->width, (int)config->height);
#ifdef ORGF_PLATFORM_GLFW
    glfwSetWindowSize(config->window, (int)width, (int)height);
#endif
  }
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_update(mrb_state *mrb, mrb_value self);

static mrb_value
mrb_graphics_wait(mrb_state *mrb, mrb_value self)
{
  mrb_float tx;
  mrb_get_args(mrb, "f", &tx);
  rf_graphics_config *config = get_config(mrb, self);
  while (tx > 0)
  {
    tx -= config->dt;
    mrb_graphics_update(mrb, self);
  }
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_fadein(mrb_state *mrb, mrb_value self)
{
  mrb_float tx;
  mrb_get_args(mrb, "f", &tx);
  rf_graphics_config *config = get_config(mrb, self);
  if (tx > 0)
  {
    mrb_float tc = tc;
    mrb_float brightness = (mrb_float)config->brightness;
    while (tc > 0)
    {
      tc -= config->dt;
      if (tc < 0) tc = 0;
      config->brightness = (mrb_int)(brightness * tc / tx);
      mrb_graphics_update(mrb, self);
    }
  }
  else
  {
    config->brightness = 0;
  }
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_fadeout(mrb_state *mrb, mrb_value self)
{
  mrb_float tx;
  mrb_get_args(mrb, "f", &tx);
  rf_graphics_config *config = get_config(mrb, self);
  if (tx > 0)
  {
    mrb_float tc = tc;
    while (tc > 0)
    {
      tc -= config->dt;
      config->brightness = (mrb_int)(255.0 * (1 - tc / tx));
      mrb_graphics_update(mrb, self);
    }
  }
  else
  {
    config->brightness = 255;
  }
  return mrb_nil_value();
}

#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401

#define rf_gl (rf_get_context()->gfx_ctx.gl)

static mrb_value
mrb_graphics_freeze(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  if (config->is_frozen) return mrb_nil_value();
  config->is_frozen = TRUE;
  size_t size = config->width * config->height;
  rf_color *buffer = mrb_malloc(mrb, size * sizeof *buffer);
  rf_gl.ReadPixels(0, 0, config->width, config->height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  for (size_t i = 0; i < size; ++i)
  {
    buffer[i].a = 255; // remove transparency
  }
  rf_image image;
  image.data   = buffer;
  image.width  = config->width;
  image.height = config->height;
  image.format = RF_UNCOMPRESSED_R8G8B8A8;
  image.valid  = true;
  config->frozen_img = rf_load_texture_from_image(image);
  rf_unload_image(image, mrb_get_allocator(mrb));
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_frame_reset(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  mrb_value last_update = mrb_iv_get(mrb, self, LAST_UPDATE);
  mrb_value now = mrb_funcall(mrb, mrb_obj_value(mrb_class_get(mrb, "Time")), "now", 0);
  config->dt = mrb_float(mrb_Float(mrb, mrb_funcall(mrb, now, "-", 1, last_update)));
  mrb_iv_set(mrb, self, LAST_UPDATE, now);
#ifdef ORGF_PLATFORM_GLFW
  glfwSwapBuffers(config->window);
  glfwPollEvents();
  if (glfwWindowShouldClose(config->window)) {
    exit(0);
  }
#endif
  return mrb_nil_value();
}

static const float corners[4][2] = {
  // Bottom-left corner for texture and quad
  { 0, 1 },
  // Bottom-right corner for texture and quad
  { 0, 0 },
  // Top-right corner for texture and quad
  { 1, 0 },
  // Top-left corner for texture and quad
  { 1, 1 }          
};

static rf_shader transition_shader;

static struct
{
  int transition_texture, left;
} transition_shader_locations;

static mrb_bool transition_shader_init = FALSE;

static const char *transition_frag =
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
  "#version 100\n"
  "precision mediump float;"
  "varying vec2 frag_tex_coord;"
  "varying vec4 frag_color;"
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
  "#version 330\n"
  "precision mediump float;"
  "in vec2 frag_tex_coord;"
  "in vec4 frag_color;"
  "out vec4 final_color;"
#endif
  "uniform sampler2D texture0;"
  "uniform sampler2D transition;"
  "uniform float left;"
  "uniform vec4 col_diffuse;"
  "void main()"
  "{"
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
  "    vec4 tt = texture2D(transition, frag_tex_coord);"
  "    vec4 texel_color = texture2D(texture0, frag_tex_coord);"
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
  "    vec4 tt = texture(transition, frag_tex_coord);"
  "    vec4 texel_color = texture(texture0, frag_tex_coord);"
#endif
  "    float gray = (0.3 * tt.r) + (0.59 * tt.g) + (0.11 * tt.b);"
  "    if (gray <= left) texel_color.a = 0;"
#if defined(RAYFORK_GRAPHICS_BACKEND_GL_ES3)
  "    frag_color = texel_color*col_diffuse*frag_color;"
#elif defined(RAYFORK_GRAPHICS_BACKEND_GL_33)
  "    final_color = texel_color*col_diffuse*frag_color;"
#endif
"}";

static inline void
init_transition_shader()
{
  if (!transition_shader_init)
  {
    transition_shader = rf_gfx_load_shader(NULL, transition_frag);
    transition_shader_locations.left = rf_gfx_get_shader_location(transition_shader, "left");
    transition_shader_locations.transition_texture = rf_gfx_get_shader_location(transition_shader, "transition");
    transition_shader_init = true;
  }
}

static inline void
bind_transition_shader(rf_texture2d texture, float left)
{
  rf_gfx_set_shader_value(
    transition_shader, transition_shader_locations.left, &left, RF_UNIFORM_FLOAT
  );
}

#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE_2D 0x0DE1

static void
draw_screen(rf_texture2d tex, rf_color color, rf_texture2d *texture)
{
  rf_gfx_enable_texture(tex.id);
  // TODO: Fix image transition not actually working
  if (texture)
  {
    rf_gl.ActiveTexture(GL_TEXTURE0 + 1);
    rf_gl.BindTexture(GL_TEXTURE_2D, texture->id);    
    rf_gl.ActiveTexture(GL_TEXTURE2);
    rf_gl.BindTexture(GL_TEXTURE_2D, texture->id);
    rf_gfx_set_shader_value_texture(
      transition_shader, transition_shader_locations.transition_texture, *texture
    );
  }
  rf_gfx_push_matrix();
    rf_gfx_begin(RF_QUADS);
      rf_gfx_color4ub(color.a, color.g, color.b, color.a);
      // Bottom-left corner for texture and quad
      rf_gfx_tex_coord2f(corners[0][0], corners[0][1]);
      rf_gfx_vertex2f(0.0f, 0.0f);
      // Bottom-right corner for texture and quad
      rf_gfx_tex_coord2f(corners[1][0], corners[1][1]);
      rf_gfx_vertex2f(0.0f, tex.height);
      // Top-right corner for texture and quad
      rf_gfx_tex_coord2f(corners[2][0], corners[2][1]);
      rf_gfx_vertex2f(tex.width, tex.height);
      // Top-left corner for texture and quad
      rf_gfx_tex_coord2f(corners[3][0], corners[3][1]);
      rf_gfx_vertex2f(tex.width, 0.0f);
    rf_gfx_end();
  rf_gfx_pop_matrix();
  rf_gfx_disable_texture();
}

static mrb_value
mrb_graphics_update(mrb_state *mrb, mrb_value self)
{
  mrb_graphics_frame_reset(mrb, self);
  rf_graphics_config *config = get_config(mrb, self);
  rf_begin();
  mrb_container_update(mrb, &(config->container));
  rf_clear(RF_BLANK);
  rf_begin_render_to_texture(config->render_texture);
    rf_clear(RF_BLANK);
    mrb_container_draw_children(mrb, &(config->container));
  rf_end_render_to_texture();
  rf_texture2d tex;
  if (config->is_frozen)
  {
    tex = config->frozen_img;
  }
  else
  {
    tex = config->render_texture.texture;
  }
  draw_screen(tex, (rf_color){255, 255, 255, config->brightness}, 0);
  rf_end();
  config->frame_count += 1;  
  return mrb_nil_value();
}



static mrb_value
mrb_graphics_transition(mrb_state *mrb, mrb_value self)
{
  mrb_float duration, vague;
  const char *name;
  rf_graphics_config *config = get_config(mrb, self);
  if (!config->is_frozen) return mrb_nil_value();
  mrb_int argc = mrb_get_args(mrb, "|fzf", &duration, &name, &vague);
  if (argc < 3) vague = 40;
  if (argc < 2) name = NULL;
  if (argc < 1) duration = 0.17;
  mrb_container_update(mrb, &(config->container));
  rf_begin_render_to_texture(config->render_texture);
    rf_clear(RF_BLANK);
    mrb_container_draw_children(mrb, &(config->container));
  rf_end_render_to_texture();
  if (duration > 0)
  {
    if (name)
    {
      const char *new_name = mrb_filesystem_join(mrb, "Graphics", name);
      rf_io_callbacks io = mrb_get_io_callbacks_for_extensions(mrb, MRB_IMAGE_EXTENSIONS);
      rf_texture2d transition_texture = rf_load_texture_from_file(new_name, mrb_get_allocator(mrb), io);
      mrb_float dt = duration;
      while (dt > 0)
      {
        dt -= config->dt;
        if (dt < 0) dt = 0;
        float left = (float)(dt / duration);
        rf_begin();
          rf_clear(RF_BLANK);
          draw_screen(config->render_texture.texture, RF_RAYWHITE, 0);
          rf_begin_blend_mode(RF_BLEND_ALPHA);
            rf_begin_shader(transition_shader);
              bind_transition_shader(transition_texture, 1.0f - left);
              draw_screen(config->frozen_img, RF_RAYWHITE, &transition_texture);
            rf_end_shader();
          rf_end_blend_mode();
        rf_end();
        mrb_graphics_frame_reset(mrb, self);
      }
      rf_unload_texture(transition_texture);
    }
    else
    {
      mrb_float dt = duration;
      while (dt > 0)
      {
        dt -= config->dt;
        if (dt < 0) dt = 0;
        mrb_int left = (mrb_int)(255 * dt / duration);
        mrb_int bg = config->brightness * left / 255;
        rf_begin();
          draw_screen(config->render_texture.texture, (rf_color){255, 255, 255, config->brightness}, 0);
          rf_begin_blend_mode(RF_BLEND_ALPHA);
            draw_screen(config->frozen_img, (rf_color){255, 255, 255, bg}, 0);
          rf_end_blend_mode();
        rf_end();
        mrb_graphics_frame_reset(mrb, self);
      }
    }
  }
  rf_unload_texture(config->frozen_img);
  config->is_frozen = FALSE;
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_snap_to_bitmap(mrb_state *mrb, mrb_value self)
{
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_play_movie(mrb_state *mrb, mrb_value self)
{
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_get_frame_rate(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  return mrb_fixnum_value(config->frame_rate);
}

static mrb_value
mrb_graphics_get_frame_count(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  return mrb_fixnum_value(config->frame_count);
}

static mrb_value
mrb_graphics_get_brightness(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  return mrb_fixnum_value(config->brightness);
}

static mrb_value
mrb_graphics_get_dt(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
  return mrb_float_value(mrb, config->dt);
}


static mrb_value
mrb_graphics_set_frame_rate(mrb_state *mrb, mrb_value self)
{
  mrb_int fps;
  rf_graphics_config *config = get_config(mrb, self);
  mrb_get_args(mrb, "i", &fps);
  if (fps < 20) fps = 20;
  if (fps > 480) fps = 480;
  config->frame_rate = fps;
  return mrb_fixnum_value(fps);
}

static mrb_value
mrb_graphics_set_frame_count(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  rf_graphics_config *config = get_config(mrb, self);
  config->frame_count = value;
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_graphics_set_brightness(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  mrb_get_args(mrb, "i", &value);
  rf_graphics_config *config = get_config(mrb, self);
  config->brightness = value > 255 ? 255 : (value < 0 ? 0 : value);
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_config_initialize(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = mrb_malloc(mrb, sizeof(*config));
  config->width = 480;
  config->height = 320;
  config->frame_rate = 60;
  config->frame_count = 0;
  config->brightness = 255;
  config->is_open = 0;
  config->is_frozen = 0;
  config->data = NULL;
  mrb_container_init(mrb, &(config->container));
  DATA_TYPE(self) = &config_type;
  DATA_PTR(self) = config;
  mrb_iv_set(mrb, self, TITLE, mrb_str_new_cstr(mrb, "ORGF Game"));
  return mrb_nil_value();
}

static mrb_value
call_block(mrb_state *mrb, mrb_value block)
{
  return mrb_funcall(mrb, block, "call", 0);
}

#ifdef ORGF_PLATFORM_GLFW
void center_window(GLFWwindow *window, GLFWmonitor *monitor)
{
    if (!monitor)
        return;

    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (!mode)
        return;

    int monitorX, monitorY;
    glfwGetMonitorPos(monitor, &monitorX, &monitorY);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    glfwSetWindowPos(window,
                     monitorX + (mode->width - windowWidth) / 2,
                     monitorY + (mode->height - windowHeight) / 2);
}
#endif

static mrb_value
mrb_start_game(mrb_state *mrb, mrb_value self)
{
  mrb_value block;
  const char *title;
  mrb_get_args(mrb, "&!", &block);
  struct RClass *graphics = mrb_module_get(mrb, "Graphics");
  rf_graphics_config *config = get_config(mrb, mrb_obj_value(graphics));
#ifdef ORGF_PLATFORM_GLFW
  if (!glfwInit()) mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create game screen");
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  title = mrb_str_to_cstr(mrb, config->title);
  config->window = glfwCreateWindow((int)config->width, (int)config->height, title, NULL, NULL);
  center_window(config->window, glfwGetPrimaryMonitor());
  if (!config->window) mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create game screen");
  glfwMakeContextCurrent(config->window);
  glfwSwapInterval(1);
  gladLoadGL();
  config->data = RF_DEFAULT_GFX_BACKEND_INIT_DATA;
#endif
  rf_init_context(&(config->context));
  config->context.logger_filter = RF_LOG_TYPE_ALL;
  config->context.logger = RF_DEFAULT_LOGGER;
  rf_init_gfx((int)config->width, (int)config->height, config->data);
  init_transition_shader();
  rf_allocator alloc = mrb_get_allocator(mrb);
  config->render_batch = rf_create_default_render_batch(alloc);
  rf_set_active_render_batch(&(config->render_batch));
  config->is_open = 1;
  config->render_texture = rf_load_render_texture((int)config->width, (int)config->height);
  mrb_value tc = mrb_obj_value(mrb_class_get(mrb, "Time"));
  mrb_iv_set(mrb, mrb_obj_value(graphics), LAST_UPDATE, mrb_funcall(mrb, tc, "now", 0));
  config->dt = 0;
  mrb_bool error;
  mrb_value ret = mrb_protect(mrb, call_block, block, &error);
  if (error) mrb_exc_raise(mrb, ret);
  config->is_open = 0;
#ifdef ORGF_PLATFORM_GLFW
  glfwDestroyWindow(config->window);
  config->window = NULL;
  glfwTerminate();
#endif
  rf_unload_render_batch(config->render_batch, alloc);
  return mrb_nil_value();
}

void
mrb_init_orgf_graphics(mrb_state *mrb)
{
  struct RClass *graphics = mrb_define_module(mrb, "Graphics");
  struct RClass *config = mrb_define_class_under(mrb, graphics, "Config", mrb->object_class);
  MRB_SET_INSTANCE_TT(config, MRB_TT_DATA);

  mrb_define_method(mrb, config, "initialize", mrb_config_initialize, MRB_ARGS_NONE());

  mrb_mod_cv_set(mrb, graphics, CONFIG, mrb_obj_new(mrb, config, 0, NULL));

  mrb_define_module_function(mrb, graphics, "update", mrb_graphics_update, MRB_ARGS_NONE());

  mrb_define_module_function(mrb, graphics, "frame_rate", mrb_graphics_get_frame_rate, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "frame_count", mrb_graphics_get_frame_count, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "brightness", mrb_graphics_get_brightness, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "delta_time", mrb_graphics_get_dt, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "dt", mrb_graphics_get_dt, MRB_ARGS_NONE());

  mrb_define_module_function(mrb, graphics, "frame_rate=", mrb_graphics_set_frame_rate, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, graphics, "frame_count=", mrb_graphics_set_frame_count, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, graphics, "brightness=", mrb_graphics_set_brightness, MRB_ARGS_REQ(1));

  mrb_define_module_function(mrb, graphics, "wait", mrb_graphics_wait, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, graphics, "freeze", mrb_graphics_freeze, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "snap_to_bitmap", mrb_graphics_snap_to_bitmap, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "frame_reset", mrb_graphics_frame_reset, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "play_movie", mrb_graphics_play_movie, MRB_ARGS_REQ(1));

  mrb_define_module_function(mrb, graphics, "fadeout", mrb_graphics_fadeout, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, graphics, "fadein", mrb_graphics_fadein, MRB_ARGS_REQ(1));

  mrb_define_module_function(mrb, graphics, "transition", mrb_graphics_transition, MRB_ARGS_OPT(3));

  mrb_define_module_function(mrb, graphics, "width", mrb_graphics_get_width, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "height", mrb_graphics_get_height, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "title", mrb_graphics_get_title, MRB_ARGS_NONE());
  mrb_define_module_function(mrb, graphics, "screen_title", mrb_graphics_get_title, MRB_ARGS_NONE());

  mrb_define_module_function(mrb, graphics, "title=", mrb_graphics_set_title, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, graphics, "screen_title=", mrb_graphics_set_title, MRB_ARGS_REQ(1));

  mrb_define_module_function(mrb, graphics, "resize_screen", mrb_graphics_resize_screen, MRB_ARGS_REQ(2));
  mrb_define_module_function(mrb, graphics, "resize", mrb_graphics_resize_screen, MRB_ARGS_REQ(2));

  mrb_define_module_function(mrb, mrb->kernel_module, "start_game", mrb_start_game, MRB_ARGS_BLOCK());
}
