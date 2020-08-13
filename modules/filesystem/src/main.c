#include <mruby.h>
#include <mruby/variable.h>
#include <mruby/string.h>

#include <ogss/file.h>

void
mrb_init_ogss_msgbox(mrb_state *mrb);
void
mrb_init_file(mrb_state *mrb);
void
mrb_init_file_utils(mrb_state *mrb);

void
mrb_ogss_filesystem_gem_init(mrb_state *mrb)
{
  mrb_setup_filesystem(mrb);
  mrb_init_file(mrb);
  mrb_init_file_utils(mrb);
  mrb_init_ogss_msgbox(mrb);
}

void
mrb_ogss_filesystem_gem_final(mrb_state *mrb)
{
  mrb_close_filesystem(mrb);
}
