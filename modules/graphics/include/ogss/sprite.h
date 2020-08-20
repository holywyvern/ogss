#ifndef OGSS_SPRITE_H
#define OGSS_SPRITE_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#include <ogss/bitmap.h>
#include <ogss/drawable.h>
#include <ogss/tone.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_sprite_data_type;

typedef struct rf_sprite rf_sprite;

struct rf_sprite
{
  rf_drawable   base;
  rf_color      original_flash_color;
  rf_color      flash_color;
  rf_vec2       bush;
  rf_bitmap    *bitmap;
  rf_rec       *src_rect;
  rf_vec2      *position;
  rf_vec2      *anchor;
  rf_vec2      *scale;
  rf_color     *color;
  rf_tone      *tone;
  mrb_float     wave_amp;
  mrb_float     wave_length;
  mrb_float     wave_speed;
  mrb_float     wave_phase;
  mrb_bool      visible;
  rf_blend_mode blend_mode;
  mrb_float     total_flash_time;
  mrb_float     flash_time;
  float         rotation;
};

static inline rf_sprite *
mrb_get_sprite(mrb_state *mrb, mrb_value obj)
{
  rf_sprite *sprite;
  Data_Get_Struct(mrb, obj, &mrb_sprite_data_type, sprite);
  if (!sprite) mrb_raise(mrb, E_DISPOSED_ERROR, "Disposed Sprite");
  return sprite;
}

#ifdef __cplusplus
}
#endif

#endif
