#ifndef OGSS_SPRITE_H
#define OGSS_SPRITE_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#include <ogss/bitmap.h>
#include <ogss/drawable.h>
#include <ogss/viewport.h>
#include <ogss/tone.h>
#include <ogss/table.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_tilemap_data_type;

typedef struct rf_tilemap_layer rf_tilemap_layer;
typedef struct rf_tilemap rf_tilemap;

struct rf_tilemap_layer
{
  rf_drawable base;
  rf_tilemap *tilemap;
};

struct rf_tilemap
{
  rf_tilemap_layer lower_layer;
  rf_tilemap_layer upper_layer;
  rf_sizei         tile;
  rf_vec2         *offset;
  rf_table        *map_data;
};

#ifdef __cplusplus
}
#endif

#endif
