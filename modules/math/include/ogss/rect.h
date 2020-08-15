#ifndef OGSS_RECT_H
#define OGSS_RECT_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_rect_data_type;

static inline rf_rec *
mrb_get_rect(mrb_state *mrb, mrb_value obj)
{
  rf_rec *ptr;
  Data_Get_Struct(mrb, obj, &mrb_rect_data_type, ptr);
  return ptr;
}

static inline mrb_bool
mrb_rect_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_rect_data_type;
}

static inline mrb_value
mrb_rect_new(mrb_state *mrb, mrb_float x, mrb_float y, mrb_float w, mrb_float h)
{
  struct RClass *rect = mrb_class_get(mrb, "Rect");
  mrb_value args[] = {
    mrb_float_value(mrb, x),
    mrb_float_value(mrb, y),
    mrb_float_value(mrb, w),
    mrb_float_value(mrb, h),
  };
  return mrb_obj_new(mrb, rect, 4, args);
}

#ifdef __cplusplus
}
#endif

#endif /* OGSS_RECT_H */
