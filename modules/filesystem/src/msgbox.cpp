#include <mruby.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/array.h>
#include <mruby/error.h>

#include <boxer/boxer.h>

extern "C" static mrb_value
mrb_msgbox(mrb_state *mrb, mrb_value self)
{
  mrb_int argc;
  mrb_value *argv;
  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc < 1)
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Print requires an object");
  }
  mrb_value result = mrb_str_new_cstr(mrb, "");
  for (mrb_int i = 0; i < argc; ++i)
  {
    result = mrb_str_cat_str(mrb, result, argv[i]);
    if (i < (argc - 1))
    {
      result = mrb_str_cat_cstr(mrb, result, "\n");
    }
  }
  boxer::show(mrb_str_to_cstr(mrb, result), "Message");
  return mrb_nil_value();
}

extern "C"
{
  mrb_value
  mrb_exc_inspect(mrb_state *mrb, mrb_value exc);

  mrb_value
  mrb_unpack_backtrace(mrb_state *mrb, mrb_value backtrace);
}

static mrb_value
append_backtrace(mrb_state *mrb, mrb_value result, mrb_value error)
{
  mrb_value backtrace;
  if (!mrb->exc) {
    return result;
  }
  backtrace = mrb_obj_iv_get(mrb, mrb->exc, mrb_intern_lit(mrb, "backtrace"));
  if (mrb_nil_p(backtrace)) return result;
  if (!mrb_array_p(backtrace)) backtrace = mrb_unpack_backtrace(mrb, backtrace);
  mrb_int i, n = RARRAY_LEN(backtrace);
  mrb_value *loc;
  for (i = n - 1, loc = &RARRAY_PTR(backtrace)[i]; i > 0; i--, loc--) {
    if (mrb_string_p(*loc)) {
      result = mrb_str_cat_cstr(mrb, result, "\t[");
      mrb_value d = mrb_funcall(mrb, mrb_fixnum_value(i), "to_s", 0);
      result = mrb_str_cat_str(mrb, result, d);
      result = mrb_str_cat_cstr(mrb, result, "] ");
      result = mrb_str_cat_str(mrb, result, *loc);
      result = mrb_str_cat_cstr(mrb, result, "\n");
    }
  }
  if (mrb_string_p(*loc)) {
    result = mrb_str_cat_str(mrb, result, *loc);
    result = mrb_str_cat_cstr(mrb, result, ": ");
  }
  mrb_value mesg = mrb_exc_inspect(mrb, error);
  return mrb_str_cat_str(mrb, result, mesg);
}

extern "C" static mrb_value
mrb_msgbox_error(mrb_state *mrb, mrb_value self)
{
  mrb_value error;
  mrb_get_args(mrb, "o", &error);
  mrb_value result = mrb_str_new_cstr(mrb, "Uncaught Error (backtrace)");
  if (mrb_exception_p(error))
  {
    result = mrb_str_cat_cstr(mrb, result, ":\n");
    result = append_backtrace(mrb, result, error);
    const char *str = mrb_str_to_cstr(mrb, result);
    boxer::show(str, "Error", boxer::Style::Error, boxer::Buttons::Quit);
  }
  return mrb_nil_value();
}

extern "C" void
mrb_init_ogss_msgbox(mrb_state *mrb)
{
  mrb_define_module_function(mrb, mrb->kernel_module, "msgbox", mrb_msgbox, MRB_ARGS_REQ(1)|MRB_ARGS_REST());
  mrb_define_module_function(mrb, mrb->kernel_module, "msgbox_error", mrb_msgbox_error, MRB_ARGS_REQ(1));
}
