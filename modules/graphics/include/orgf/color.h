#ifndef ORGF_COLOR_H
#define ORGF_COLOR_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_color_data_type;

static inline mrb_value
mrb_color_new(mrb_state *mrb, mrb_int r, mrb_int g, mrb_int b, mrb_int a)
{
  struct RClass *color = mrb_class_get(mrb, "Color");
  mrb_value args[] = {
    mrb_fixnum_value(r),
    mrb_fixnum_value(g),
    mrb_fixnum_value(b),
    mrb_fixnum_value(a),
  };
  return mrb_obj_new(mrb, color, 4, args);
}

static inline mrb_value
mrb_color_white(mrb_state *mrb)
{
  return mrb_color_new(mrb, 255, 255, 255, 255);
}

static inline mrb_bool
mrb_color_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_color_data_type;
}

static inline rf_color *
mrb_get_color(mrb_state *mrb, mrb_value obj)
{
  rf_color *color;
  Data_Get_Struct(mrb, obj, &mrb_color_data_type, color);
  return color;
}

#ifdef __cplusplus
}
#endif

#endif
