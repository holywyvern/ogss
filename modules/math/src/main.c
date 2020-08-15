#include <mruby.h>

void
mrb_init_ogss_point(mrb_state *mrb);
void
mrb_init_ogss_rect(mrb_state *mrb);
void
mrb_init_ogss_table(mrb_state *mrb);

void
mrb_ogss_math_gem_init(mrb_state *mrb)
{
  mrb_init_ogss_point(mrb);
  mrb_init_ogss_rect(mrb);
  mrb_init_ogss_table(mrb);
}

void
mrb_ogss_math_gem_final(mrb_state *mrb)
{
}
