#ifndef OGSS_FONT_H
#define OGSS_FONT_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_font_data_type;

const char *
mrb_get_default_font_name(mrb_state *mrb);

mrb_int
mrb_get_default_font_size(mrb_state *mrb);

mrb_bool
mrb_get_default_font_antialias(mrb_state *mrb);

static inline mrb_bool
mrb_font_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_font_data_type;
}

static inline rf_font *
mrb_get_font(mrb_state *mrb, mrb_value obj)
{
  rf_font *font;
  Data_Get_Struct(mrb, obj, &mrb_font_data_type, font);
  return font;
}

static inline mrb_value
mrb_new_default_font(mrb_state *mrb)
{
  struct RClass *font = mrb_class_get(mrb, "Font");
  return mrb_obj_new(mrb, font, 0, NULL);
}

#ifdef __cplusplus
}
#endif

#endif
