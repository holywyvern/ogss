#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include <ogss/color.h>

const struct mrb_data_type mrb_color_data_type = { "Color", mrb_free };

#define clamp(v) ((v) > 255 ? 255 : ((v) < 0 ? 0 : (v)))

static void
set_color(mrb_state *mrb, mrb_int argc, rf_color *color)
{
  switch (argc)
  {
    case 0:
    {
      color->r = color->g = color->b = color->a = 0;
      break;
    }
    case 1:
    {
      rf_color *original;
      mrb_get_args(mrb, "d", &original, &mrb_color_data_type);
      *color = *original;
      break;
    }
    case 3:
    {
      mrb_int r, g, b;
      mrb_get_args(mrb, "iii", &r, &g, &b);
      color->r = (unsigned char)clamp(r);
      color->g = (unsigned char)clamp(g);
      color->b = (unsigned char)clamp(b);
      color->a = (unsigned char)255;
      break;
    }
    case 4:
    {
      mrb_int r, g, b, a;
      mrb_get_args(mrb, "iiii", &r, &g, &b, &a);
      color->r = (unsigned char)clamp(r);
      color->g = (unsigned char)clamp(g);
      color->b = (unsigned char)clamp(b);
      color->a = (unsigned char)clamp(a);
      break;
    }
  }
}

static mrb_value
color_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  switch (argc)
  {
    case 0: case 1: case 3: case 4: {
      rf_color *color = mrb_malloc(mrb, sizeof *color);
      DATA_TYPE(self) = &mrb_color_data_type;
      DATA_PTR(self) = color;
      set_color(mrb, argc, color);
      break;
    }
    default: {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments, 0, 1, 3, or 4 expected but, %d given.", argc);
      break;
    }
  }
  return self;
}

static mrb_value
color_set(mrb_state *mrb, mrb_value self)
{
  mrb_int argc = mrb_get_argc(mrb);
  switch (argc)
  {
    case 1: case 3: case 4: {
      rf_color *color = mrb_get_color(mrb, self);
      set_color(mrb, argc, color);
      break;
    }
    default: {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments 1, 3, or 4 expected but, %d given.", argc);
      break;
    }
  }
  return self;
}

static mrb_value
color_get_red(mrb_state *mrb, mrb_value self)
{
  rf_color *color = mrb_get_color(mrb, self);
  return mrb_fixnum_value(color->r);
}

static mrb_value
color_get_green(mrb_state *mrb, mrb_value self)
{
  rf_color *color = mrb_get_color(mrb, self);
  return mrb_fixnum_value(color->g);
}

static mrb_value
color_get_blue(mrb_state *mrb, mrb_value self)
{
  rf_color *color = mrb_get_color(mrb, self);
  return mrb_fixnum_value(color->b);
}

static mrb_value
color_get_alpha(mrb_state *mrb, mrb_value self)
{
  rf_color *color = mrb_get_color(mrb, self);
  return mrb_fixnum_value(color->a);
}

static mrb_value
color_set_red(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_color *color = mrb_get_color(mrb, self);
  mrb_get_args(mrb, "i", &value);
  color->r = (unsigned char)clamp(value);
  return mrb_fixnum_value(value);
}

static mrb_value
color_set_green(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_color *color = mrb_get_color(mrb, self);
  mrb_get_args(mrb, "i", &value);
  color->g = (unsigned char)clamp(value);
  return mrb_fixnum_value(value);
}

static mrb_value
color_set_blue(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_color *color = mrb_get_color(mrb, self);
  mrb_get_args(mrb, "i", &value);
  color->b = (unsigned char)clamp(value);
  return mrb_fixnum_value(value);
}

static mrb_value
color_set_alpha(mrb_state *mrb, mrb_value self)
{
  mrb_int value;
  rf_color *color = mrb_get_color(mrb, self);
  mrb_get_args(mrb, "i", &value);
  color->a = (unsigned char)clamp(value);
  return mrb_fixnum_value(value);
}

void
mrb_init_ogss_color(mrb_state *mrb)
{
  struct RClass *color = mrb_define_class(mrb, "Color", mrb->object_class);
  MRB_SET_INSTANCE_TT(color, MRB_TT_DATA);

  mrb_define_method(mrb, color, "initialize", color_initialize, MRB_ARGS_OPT(4));
  mrb_define_method(mrb, color, "initialize_copy", color_initialize, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, color, "r", color_get_red, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "g", color_get_green, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "b", color_get_blue, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "a", color_get_alpha, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "red", color_get_red, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "green", color_get_green, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "blue", color_get_blue, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "alpha", color_get_alpha, MRB_ARGS_NONE());

  mrb_define_method(mrb, color, "r=", color_set_red, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "g=", color_set_green, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "b=", color_set_blue, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "a=", color_set_alpha, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "red=", color_set_red, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "green=", color_set_green, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "blue=", color_set_blue, MRB_ARGS_NONE());
  mrb_define_method(mrb, color, "alpha=", color_set_alpha, MRB_ARGS_NONE());

  mrb_define_method(mrb, color, "set", color_set, MRB_ARGS_OPT(4));
}
