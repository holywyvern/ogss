#include <mruby.h>

void
mrb_init_orgf_graphics(mrb_state *mrb);

void
mrb_init_orgf_color(mrb_state *mrb);

void
mrb_init_orgf_tone(mrb_state *mrb);

void
mrb_init_orgf_font(mrb_state *mrb);

void
mrb_init_orgf_bitmap(mrb_state *mrb);

void
mrb_init_orgf_viewport(mrb_state *mrb);

void
mrb_init_orgf_sprite(mrb_state *mrb);

void
mrb_init_orgf_plane(mrb_state *mrb);

void
mrb_init_orgf_window(mrb_state *mrb);

void
mrb_orgf_graphics_gem_init(mrb_state *mrb)
{
  mrb_define_class(mrb, "DisposedError", mrb->eStandardError_class);
  mrb_init_orgf_color(mrb);
  mrb_init_orgf_tone(mrb);
  mrb_init_orgf_font(mrb);
  mrb_init_orgf_graphics(mrb);
  mrb_init_orgf_bitmap(mrb);
  mrb_init_orgf_viewport(mrb);
  mrb_init_orgf_sprite(mrb);
  mrb_init_orgf_plane(mrb);
  mrb_init_orgf_window(mrb);
}

void
mrb_orgf_graphics_gem_final(mrb_state *mrb)
{
}
