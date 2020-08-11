#ifndef OGSS_COLOR_H
#define OGSS_COLOR_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_color_data_type;

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
