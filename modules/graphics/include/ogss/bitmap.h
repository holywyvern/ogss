#ifndef OGSS_BITMAP_H
#define OGSS_BITMAP_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#include <ogss/drawable.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_bitmap rf_bitmap;

extern const struct mrb_data_type mrb_bitmap_data_type;

struct rf_bitmap
{
  rf_image     image;
  rf_texture2d texture;
  mrb_bool     dirty;
};

static inline rf_bitmap *
mrb_get_bitmap(mrb_state *mrb, mrb_value obj)
{
  rf_bitmap *bmp;
  Data_Get_Struct(mrb, obj, &mrb_bitmap_data_type, bmp);
  if (!bmp)
  {
    mrb_raise(mrb, E_DISPOSED_ERROR, "Disposed bitmap");
  }
  return bmp;
}

static inline void
mrb_refresh_bitmap(rf_bitmap *bmp)
{
  if (bmp->dirty)
  {
    bmp->dirty = FALSE;
    rf_unload_texture(bmp->texture);
    bmp->texture = rf_load_texture_from_image(bmp->image);
  }
}

static inline mrb_bool
mrb_bitmap_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_bitmap_data_type;
}

#ifdef __cplusplus
}
#endif

#endif
