#ifndef OGSS_PLANE_H
#define OGSS_PLANE_H

#include <mruby.h>
#include <mruby/data.h>

#include <rayfork.h>

#include <ogss/drawable.h>
#include <ogss/viewport.h>
#include <ogss/bitmap.h>
#include <ogss/tone.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_plane_data_type;

typedef struct rf_plane rf_plane;

struct rf_plane
{
  rf_drawable   base;
  rf_viewport  *viewport;
  rf_bitmap    *bitmap;
  rf_vec2      *offset;
  rf_vec2      *scale;
  rf_color     *color;
  rf_tone      *tone;
  rf_blend_mode blend_mode;
};

static inline mrb_bool
mrb_plane_p(mrb_value object)
{
  return mrb_data_p(object) && DATA_TYPE(object) == &mrb_plane_data_type;
}

static inline rf_plane *
mrb_get_plane(mrb_state *mrb, mrb_value obj)
{
  rf_plane *plane;
  Data_Get_Struct(mrb, obj, &mrb_plane_data_type, plane);
  if (!plane) mrb_raise(mrb, E_DISPOSED_ERROR, "disposed Plane");
  return plane;
}

#ifdef __cplusplus
}
#endif

#endif
