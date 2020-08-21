#ifndef OGSS_WINDOW_H
#define OGSS_WINDOW_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#include <ogss/drawable.h>
#include <ogss/bitmap.h>
#include <ogss/tone.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_window_data_type;
extern const struct mrb_data_type mrb_window_padding_data_type;

typedef struct rf_window_padding rf_window_padding;
typedef struct rf_window rf_window;

struct rf_window_padding
{
  mrb_int top, left, right, bottom;
};

struct rf_window
{
  rf_drawable         base;
  rf_window_padding   padding;
  struct
  {
    rf_rec         backgrounds[2];
    rf_rec         pause[4];
    rf_npatch_info cursor;
    struct
    {
      rf_rec top;
      rf_rec bottom;
      rf_rec left;
      rf_rec right;
    }     arrows;
    struct
    {
      rf_rec    top_left;
      rf_rec    top;
      rf_rec    top_right;
      rf_rec    right;
      rf_rec    bottom_right;
      rf_rec    bottom;
      rf_rec    bottom_left;
      rf_rec    left;
      mrb_float offset;
    }      borders;
  }                   skin_rects;
  rf_render_texture2d render;
  rf_vec2            *offset;
  rf_rec             *rect;
  rf_rec             *cursor_rect;
  rf_tone            *tone;
  rf_bitmap          *skin;
  rf_bitmap          *contents;
  mrb_int             openness;
  mrb_int             opacity;
  mrb_int             back_opacity;
  mrb_float           cursor_count;
  mrb_int             cursor_opacity;
  mrb_int             pause_frame;
  mrb_float           pause_count;
  mrb_int             contents_opacity;
  mrb_bool            active;
  mrb_bool            arrows_visible;
  mrb_bool            pause;
};

static inline rf_window *
mrb_get_window(mrb_state *mrb, mrb_value obj)
{
  rf_window *result;
  Data_Get_Struct(mrb, obj, &mrb_window_data_type, result);
  if (!result)
  {
    mrb_raise(mrb, E_DISPOSED_ERROR, "Disposed Window");
  }
  return result;
}

static inline mrb_bool
mrb_window_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_window_data_type;
}

#ifdef __cplusplus
}
#endif

#endif
