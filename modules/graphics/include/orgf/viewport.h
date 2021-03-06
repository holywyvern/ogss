#ifndef ORGF_VIEWPORT_H
#define ORGF_VIEWPORT_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#include <orgf/drawable.h>
#include <orgf/tone.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_viewport_data_type;

typedef struct rf_viewport rf_viewport;

struct rf_viewport
{
  rf_container         base;
  rf_color             original_flash_color;
  rf_color             flash_color;  
  rf_color            *color;
  rf_tone             *tone;
  rf_rec              *rect;
  rf_vec2             *offset;
  rf_render_texture2d  render;
  mrb_float            total_flash_time;
  mrb_float            flash_time;
};

static inline rf_viewport *
mrb_get_viewport(mrb_state *mrb, mrb_value obj)
{
  rf_viewport *result;
  Data_Get_Struct(mrb, obj, &mrb_viewport_data_type, result);
  if (!result)
  {
    mrb_raise(mrb, E_DISPOSED_ERROR, "Disposed Viewport");
  }
  return result;
}

static inline mrb_bool
mrb_viewport_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_viewport_data_type;
}

#ifdef __cplusplus
}
#endif

#endif
