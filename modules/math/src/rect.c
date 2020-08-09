#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/array.h>

#include <ogss/rect.h>
#include <rayfork.h>

#define RECT_PACK mrb_str_new_cstr(mrb, "F4")

const struct mrb_data_type mrb_rect_data_type = {
  "Rect", mrb_free
};

static inline void
set_rect(mrb_state *mrb, rf_rec *rect, mrb_int argc)
{
  switch (argc)
  {
    case 0: {
      rect->x = rect->y = rect->width = rect->height = 0;
      break;
    }
    case 1: {
      rf_rec *original;
      mrb_get_args(mrb, "d", &original, &mrb_rect_data_type);
      *rect = *original;
      break;
    }
    case 4: {
      mrb_float x, y, w, h;
      mrb_get_args(mrb, "ffff", &x, &y, &w, &h);
      if (w < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Width must be positive");
      if (h < 0) mrb_raise(mrb, E_ARGUMENT_ERROR, "Height must be positive");
      rect->x = (float)x;
      rect->y = (float)y;
      rect->width = (float)w;
      rect->height = (float)h;
      break;
    }
    default: break;
  }
}

static mrb_value
mrb_rect_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  switch (argc)
  {
    case 0: case 1: case 4: {
      rf_rec *rect = mrb_malloc(mrb, sizeof(*rect));
      DATA_TYPE(self) = &mrb_rect_data_type;
      DATA_PTR(self) = rect;      
      set_rect(mrb, rect, argc);
      break;
    }
    default:
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments, 0, 1 or 4 expected but, %d given.", argc);
      break;
  }
  return self;
}

static mrb_value
mrb_rect_get_x(mrb_state *mrb, mrb_value self)
{
  rf_rec *rect = mrb_get_rect(mrb, self);
  return mrb_float_value(mrb, rect->x);
}

static mrb_value
mrb_rect_get_y(mrb_state *mrb, mrb_value self)
{
  rf_rec *rect = mrb_get_rect(mrb, self);
  return mrb_float_value(mrb, rect->y);
}

static mrb_value
mrb_rect_get_width(mrb_state *mrb, mrb_value self)
{
  rf_rec *rect = mrb_get_rect(mrb, self);
  return mrb_float_value(mrb, rect->width);
}

static mrb_value
mrb_rect_get_height(mrb_state *mrb, mrb_value self)
{
  rf_rec *rect = mrb_get_rect(mrb, self);
  return mrb_float_value(mrb, rect->height);
}

static mrb_value
mrb_rect_set_x(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  rf_rec *rect = mrb_get_rect(mrb, self);
  mrb_get_args(mrb, "f", &value);
  rect->x = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_rect_set_y(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  rf_rec *rect = mrb_get_rect(mrb, self);
  mrb_get_args(mrb, "f", &value);
  rect->y = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_rect_set_width(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  rf_rec *rect = mrb_get_rect(mrb, self);
  mrb_get_args(mrb, "f", &value);
  rect->width = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_rect_set_height(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  rf_rec *rect = mrb_get_rect(mrb, self);
  mrb_get_args(mrb, "f", &value);
  rect->height = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_rect_set(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  switch (argc)
  {
    case 1: case 4: {
      rf_rec *rect = mrb_get_rect(mrb, self);
      set_rect(mrb, rect, argc);
      break;
    }
    default:
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments, 1 or 4 expected but, %d given.", argc);
      break;
  }
  return mrb_nil_value();
}

static mrb_value
mrb_rect_empty(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  rf_rec *rect = mrb_get_rect(mrb, self);
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  rect->x = rect->y = rect->width = rect->height = 0;
  return self;
}

static mrb_value
mrb_rect_dump(mrb_state *mrb, mrb_value self)
{
  rf_rec *rect = mrb_get_rect(mrb, self);
  mrb_value values[] = {
    mrb_float_value(mrb, rect->x),
    mrb_float_value(mrb, rect->y),
    mrb_float_value(mrb, rect->width),
    mrb_float_value(mrb, rect->height),
  };
  mrb_value ary = mrb_ary_new_from_values(mrb, 4, values);
  return mrb_funcall(mrb, ary, "pack", 1, RECT_PACK);
}

static mrb_value
mrb_rect_load(mrb_state *mrb, mrb_value self)
{
  mrb_value args;
  mrb_get_args(mrb, "S", &args);
  args = mrb_funcall(mrb, args, "unpack", 1, RECT_PACK);
  return mrb_funcall(mrb, self, "new", 4,
    mrb_ary_entry(args, 0), mrb_ary_entry(args, 1), mrb_ary_entry(args, 2), mrb_ary_entry(args, 3)
  );
}

void
mrb_init_rect(mrb_state *mrb)
{
  struct RClass *rect = mrb_define_class(mrb, "Rect", mrb->object_class);
  MRB_SET_INSTANCE_TT(rect, MRB_TT_DATA);

  mrb_define_method(mrb, rect, "initialize", mrb_rect_initialize, MRB_ARGS_OPT(4));
  mrb_define_method(mrb, rect, "initialize_copy", mrb_rect_initialize, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, rect, "x", mrb_rect_get_x, MRB_ARGS_NONE());
  mrb_define_method(mrb, rect, "y", mrb_rect_get_y, MRB_ARGS_NONE());
  mrb_define_method(mrb, rect, "width", mrb_rect_get_width, MRB_ARGS_NONE());
  mrb_define_method(mrb, rect, "height", mrb_rect_get_height, MRB_ARGS_NONE());

  mrb_define_method(mrb, rect, "x=", mrb_rect_set_x, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rect, "y=", mrb_rect_set_y, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rect, "width=", mrb_rect_set_width, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, rect, "height=", mrb_rect_set_height, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, rect, "set", mrb_rect_set, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(3));
  mrb_define_method(mrb, rect, "empty", mrb_rect_empty, MRB_ARGS_NONE());

  mrb_define_method(mrb, rect, "_dump", mrb_rect_dump, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, rect, "_load", mrb_rect_load, MRB_ARGS_REQ(1));
}
