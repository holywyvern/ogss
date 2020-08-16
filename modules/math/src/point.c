#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/array.h>

#include <ogss/point.h>
#include <rayfork.h>

#define VEC2_PACK mrb_str_new_cstr(mrb, "F2")

const struct mrb_data_type mrb_point_data_type = {
  "Point", mrb_free
};

static inline void
set_point(mrb_state *mrb, rf_vec2 *vec, mrb_int argc)
{
  switch (argc)
  {
    case 0:
    {
      vec->x = vec->y = 0;
      break;
    }
    case 1:
    {
      rf_vec2 *original;
      mrb_get_args(mrb, "d", &original, &mrb_point_data_type);
      *vec = *original;
      break;
    }
    case 2:
    {
      mrb_float x, y;
      mrb_get_args(mrb, "ff", &x, &y);
      vec->x = (float)x;
      vec->y = (float)y;
      break;
    }
  }
}

static mrb_value
point_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  rf_vec2 *vec = mrb_malloc(mrb, sizeof *vec);
  DATA_TYPE(self) = &mrb_point_data_type;
  DATA_PTR(self) = vec;
  switch (argc)
  {
  case 0: case 1: case 2:
  {
    set_point(mrb, vec, argc);
    break;
  } 
  default:
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments, expected 0..2 but %d were given", argc);
    break;
  }
  return mrb_nil_value();
}

static mrb_value
point_set(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  switch (argc)
  {
  case 1: case 2:
  {
    set_point(mrb, mrb_get_point(mrb, self), argc);
    break;
  } 
  default:
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments, expected 1 or 2 but %d were given", argc);
    break;
  }
  return self;
}

static mrb_value
mrb_point_get_x(mrb_state *mrb, mrb_value self)
{
  return mrb_float_value(mrb, mrb_get_point(mrb, self)->x);
}

static mrb_value
mrb_point_get_y(mrb_state *mrb, mrb_value self)
{
  return mrb_float_value(mrb, mrb_get_point(mrb, self)->y);
}

static mrb_value
mrb_point_set_x(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  mrb_get_args(mrb, "f", &value);
  rf_vec2 *point = mrb_get_point(mrb, self);
  point->x = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_point_set_y(mrb_state *mrb, mrb_value self)
{
  mrb_float value;
  mrb_check_frozen(mrb, mrb_basic_ptr(self));
  mrb_get_args(mrb, "f", &value);
  rf_vec2 *point = mrb_get_point(mrb, self);
  point->y = (float)value;
  return mrb_float_value(mrb, value);
}

static mrb_value
mrb_point_dump(mrb_state *mrb, mrb_value self)
{
  rf_vec2 *point = mrb_get_point(mrb, self);
  mrb_value values[] = {
    mrb_float_value(mrb, point->x),
    mrb_float_value(mrb, point->y),
  };
  mrb_value ary = mrb_ary_new_from_values(mrb, 2, values);
  return mrb_funcall(mrb, ary, "pack", 1, VEC2_PACK);
}

static mrb_value
mrb_point_load(mrb_state *mrb, mrb_value self)
{
  mrb_value args;
  mrb_get_args(mrb, "S", &args);
  args = mrb_funcall(mrb, args, "unpack", 1, VEC2_PACK);
  return mrb_funcall(mrb, self, "new", 2,
    mrb_ary_entry(args, 0), mrb_ary_entry(args, 1)
  );
}

static mrb_value
mrb_point_equal(mrb_state *mrb, mrb_value self)
{
  mrb_value other;
  mrb_get_args(mrb, "o", &other);
  if (mrb_point_p(other))
  {
    rf_vec2 *a = mrb_get_point(mrb, self);
    rf_vec2 *b = mrb_get_point(mrb, other);
    mrb_bool cmp = (a->x == b->x) && (a->y == b->y);
    return mrb_bool_value(cmp);
  }
  return mrb_false_value();
}

void
mrb_init_ogss_point(mrb_state *mrb)
{
  struct RClass *point = mrb_define_class(mrb, "Point", mrb->object_class);
  MRB_SET_INSTANCE_TT(point, MRB_TT_DATA);

  mrb_define_method(mrb, point, "initialize", point_initialize, MRB_ARGS_OPT(2));
  mrb_define_method(mrb, point, "initialize_copy", point_initialize, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, point, "set", point_set, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));

  mrb_define_method(mrb, point, "x", mrb_point_get_x, MRB_ARGS_NONE());
  mrb_define_method(mrb, point, "y", mrb_point_get_y, MRB_ARGS_NONE());

  mrb_define_method(mrb, point, "x=", mrb_point_set_x, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, point, "y=", mrb_point_set_y, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, point, "==", mrb_point_equal, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, point, "_dump", mrb_point_dump, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, point, "_load", mrb_point_load, MRB_ARGS_REQ(1));
}
