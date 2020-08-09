#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/string.h>
#include <mruby/object.h>

#include <rayfork.h>

#include <ogss/graphics.h>

#define CONFIG mrb_intern_lit(mrb, "#config")
#define TITLE mrb_intern_lit(mrb, "#title")

static const mrb_data_type config_type = {
  "Graphics::Config", mrb_free
};

static inline rf_graphics_config *
get_config(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config;
  mrb_value config_value = mrb_iv_get(mrb, self, CONFIG);
  Data_Get_Struct(mrb, config_value, &config_type, config);
  config->title = mrb_iv_get(mrb, config_value, TITLE);
  return config;
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
mrb_graphics_resize_screen(mrb_state *mrb, mrb_value self)
{
  mrb_int width, height;
  rf_graphics_config *config = get_config(mrb, self);
  mrb_get_args(mrb, "ii", &width, &height);
  if (width < 1) mrb_raise(mrb, E_ARGUMENT_ERROR, "Width must be positive");
  if (height < 1) mrb_raise(mrb, E_ARGUMENT_ERROR, "Height must be positive");
  return mrb_nil_value();
}

static mrb_value
mrb_graphics_update(mrb_state *mrb, mrb_value self)
{
  rf_graphics_config *config = get_config(mrb, self);
#ifdef OGSS_PLATFORM_GLFW
  glfwPollEvents();
  glfwSwapBuffers(config->window);
#endif
}

static mrb_value
mrb_graphics_wait(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_fadein(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_fadeout(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_freeze(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_frame_reset(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_transition(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_snap_to_bitmap(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_play_movie(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_get_frame_rate(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_get_frame_count(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_get_brightness(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_set_frame_rate(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_set_frame_count(mrb_state *mrb, mrb_value self)
{
}

static mrb_value
mrb_graphics_set_brightness(mrb_state *mrb, mrb_value self)
{
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
  config->context = (rf_context){0};
  config->render_batch = (rf_default_render_batch){0};
  DATA_TYPE(self) = &config_type;
  DATA_PTR(self) = config;
  mrb_iv_set(mrb, self, TITLE, mrb_str_new_cstr(mrb, "OGSS Game"));
  return mrb_nil_value();
}

static mrb_value
mrb_start_game(mrb_state *mrb, mrb_value self)
{
  mrb_value block;
  const char *title;
  mrb_get_args(mrb, "&!", &block);
  struct RClass *graphics = mrb_module_get(mrb, "Graphics");
  rf_graphics_config *config = get_config(mrb, mrb_obj_value(graphics));
#ifdef OGSS_PLATFORM_GLFW
  if (!glfwInit()) mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create game screen");
  title = mrb_str_to_cstr(mrb, config->title);
  config->window = glfwCreateWindow(config->width, config->height, title, NULL, NULL);
  if (!config->window) mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create game screen");
  glfwMakeContextCurrent(config->window);
  glfwSwapInterval(1);
  gladLoadGL();
  config->data = mrb_malloc(mrb, sizeof(rf_opengl_procs));
  *((rf_opengl_procs *)(void *)(config->data)) = RF_DEFAULT_OPENGL_PROCS;
#endif
  rf_init(&(config->context), config->width, config->height, RF_DEFAULT_LOGGER, config->data);
  config->is_open = 1;
  mrb_funcall(mrb, block, "call", 0);
  config->is_open = 0;
#ifdef OGSS_PLATFORM_GLFW
  glfwDestroyWindow(config->window);
  config->window = NULL;
  glfwTerminate();
#endif
  return mrb_nil_value();
}

void
mrb_init_graphics(mrb_state *mrb)
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
  mrb_define_module_function(mrb, graphics, "resize_screen", mrb_graphics_resize_screen, MRB_ARGS_REQ(2));

  mrb_define_module_function(mrb, mrb->kernel_module, "start_game", mrb_start_game, MRB_ARGS_BLOCK());
}
