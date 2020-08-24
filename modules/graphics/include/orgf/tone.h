#ifndef ORGF_TONE_H
#define ORGF_TONE_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_tone rf_tone;

struct rf_tone
{
  int16_t r, g, b;
  uint8_t a;
};

extern const struct mrb_data_type mrb_tone_data_type;

static inline mrb_value
mrb_tone_new(mrb_state *mrb, mrb_int r, mrb_int g, mrb_int b, mrb_int a)
{
  struct RClass *color = mrb_class_get(mrb, "Tone");
  mrb_value args[] = {
    mrb_fixnum_value(r),
    mrb_fixnum_value(g),
    mrb_fixnum_value(b),
    mrb_fixnum_value(a),
  };
  return mrb_obj_new(mrb, color, 4, args);
}

static inline mrb_value
mrb_tone_neutral(mrb_state *mrb)
{
  return mrb_tone_new(mrb, 0, 0, 0, 0);
}

static inline mrb_bool
mrb_tone_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_tone_data_type;
}

static inline rf_tone *
mrb_get_tone(mrb_state *mrb, mrb_value obj)
{
  rf_tone *tone;
  Data_Get_Struct(mrb, obj, &mrb_tone_data_type, tone);
  return tone;
}

#ifdef __cplusplus
}
#endif

#endif
