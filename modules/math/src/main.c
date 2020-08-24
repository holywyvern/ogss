#include <mruby.h>

void
mrb_init_orgf_point(mrb_state *mrb);
void
mrb_init_orgf_rect(mrb_state *mrb);
void
mrb_init_orgf_table(mrb_state *mrb);

void
mrb_orgf_math_gem_init(mrb_state *mrb)
{
  mrb_init_orgf_point(mrb);
  mrb_init_orgf_rect(mrb);
  mrb_init_orgf_table(mrb);
}

void
mrb_orgf_math_gem_final(mrb_state *mrb)
{
}
