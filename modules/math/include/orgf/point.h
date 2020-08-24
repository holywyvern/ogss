#ifndef ORGF_POINT_H
#define ORGF_POINT_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_point_data_type;

static inline rf_vec2 *
mrb_get_point(mrb_state *mrb, mrb_value obj)
{
  rf_vec2 *ptr;
  Data_Get_Struct(mrb, obj, &mrb_point_data_type, ptr);
  return ptr;
}

static inline mrb_bool
mrb_point_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_point_data_type;
}

static inline mrb_value
mrb_point_new(mrb_state *mrb, mrb_float x, mrb_float y)
{
  struct RClass *point = mrb_class_get(mrb, "Point");
  mrb_value args[] = {
    mrb_float_value(mrb, x),
    mrb_float_value(mrb, y),
  };
  return mrb_obj_new(mrb, point, 2, args);
}

#ifdef __cplusplus
}
#endif

#endif /* ORGF_POINT_H */
