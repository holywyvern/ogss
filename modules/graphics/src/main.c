#include <mruby.h>

void
mrb_init_ogss_color(mrb_state *mrb);

void
mrb_init_ogss_font(mrb_state *mrb);

const char *
mrb_get_default_font_name(mrb_state *mrb);

mrb_int
mrb_get_default_font_size(mrb_state *mrb);

mrb_bool
mrb_get_default_font_antialias(mrb_state *mrb);

void
mrb_ogss_graphics_gem_init(mrb_state *mrb)
{
  mrb_init_ogss_color(mrb);
  mrb_init_ogss_font(mrb);
}

void
mrb_ogss_graphics_gem_final(mrb_state *mrb)
{
}
