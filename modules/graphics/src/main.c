#include <mruby.h>

void
mrb_init_ogss_color(mrb_state *mrb);
void
mrb_ogss_graphics_gem_init(mrb_state *mrb)
{
  mrb_init_ogss_color(mrb);
}

void
mrb_ogss_graphics_gem_final(mrb_state *mrb)
{
}
