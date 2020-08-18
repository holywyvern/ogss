#ifndef OGSS_WINDOW_H
#define OGSS_WINDOW_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#include <ogss/drawable.h>
#include <ogss/bitmap.h>

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
  rf_drawable       base;
  rf_vec2          *offset;
  rf_rec           *rect;
  rf_rec           *cursor_rect;
  rf_color         *color;
  rf_bitmap        *skin;
  rf_bitmap        *contents;
  rf_window_padding padding;
  mrb_int           openness;
  mrb_int           opacity;
  mrb_int           cursor_opacity;
  mrb_int           contents_opacity;
  mrb_bool          visible;
  mrb_bool          active;
  mrb_bool          arrows_visible;
  mrb_bool          pause;
};

#ifdef __cplusplus
}
#endif

#endif
