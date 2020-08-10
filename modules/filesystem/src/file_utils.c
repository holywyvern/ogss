#include <mruby.h>

#include <ogss/file.h>

static mrb_value
file_mkdir(mrb_state *mrb, mrb_value self)
{
  const char *name;
  mrb_get_args(mrb, "z", &name);
  return mrb_bool_value(mrb_file_mkdir(mrb, name));
}

static mrb_value
file_remove(mrb_state *mrb, mrb_value self)
{
  const char *name;
  mrb_get_args(mrb, "z", &name);
  return mrb_bool_value(mrb_file_remove(mrb, name));
}

static mrb_value
file_setup_write(mrb_state *mrb, mrb_value self)
{
  const char *org, *name;
  mrb_get_args(mrb, "zz", &org, &name);
  mrb_file_set_write(mrb, org, name);
  return mrb_nil_value();
}

void
mrb_init_file_utils(mrb_state *mrb)
{
  struct RClass *utils = mrb_define_module(mrb, "FileUtils");

  mrb_define_module_function(mrb, utils, "mkdir", file_mkdir, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, utils, "remove", file_remove, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, utils, "delete", file_remove, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, utils, "unlink", file_remove, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, utils, "setup_write", file_setup_write, MRB_ARGS_REQ(2));
}
