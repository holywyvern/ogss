#include <mruby.h>

void
mrb_init_ogss_graphics(mrb_state *mrb);

void
mrb_init_ogss_color(mrb_state *mrb);

void
mrb_init_ogss_tone(mrb_state *mrb);

void
mrb_init_ogss_font(mrb_state *mrb);

void
mrb_init_ogss_bitmap(mrb_state *mrb);

void
mrb_init_ogss_viewport(mrb_state *mrb);

void
mrb_init_ogss_sprite(mrb_state *mrb);

void
mrb_init_ogss_plane(mrb_state *mrb);

void
mrb_ogss_graphics_gem_init(mrb_state *mrb)
{
  mrb_define_class(mrb, "DisposedError", mrb->eStandardError_class);
  mrb_init_ogss_color(mrb);
  mrb_init_ogss_tone(mrb);
  mrb_init_ogss_font(mrb);
  mrb_init_ogss_graphics(mrb);
  mrb_init_ogss_bitmap(mrb);
  mrb_init_ogss_viewport(mrb);
  mrb_init_ogss_sprite(mrb);
  mrb_init_ogss_plane(mrb);
}

void
mrb_ogss_graphics_gem_final(mrb_state *mrb)
{
}
