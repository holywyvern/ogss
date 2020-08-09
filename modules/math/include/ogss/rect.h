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

#ifdef __cplusplus
}
#endif

#endif /* OGSS_RECT_H */
