#ifndef OGSS_DRAWABLE_H
#define OGSS_DRAWABLE_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_drawable rf_drawable;
typedef struct rf_container rf_container;

typedef void (*rf_draw_callback)(rf_drawable *obj);

struct rf_container
{
  rf_drawable **items;
  mrb_int       items_capa;
  mrb_int       items_size;
  mrb_bool      dirty;
};

struct rf_drawable
{
  struct rf_container *container;
  rf_draw_callback     draw;
  mrb_int              z;
};

void
mrb_container_init(mrb_state *mrb, rf_container *container);

void
mrb_container_free(mrb_state *mrb, rf_container *container);

void
mrb_container_add_child(mrb_state *mrb, rf_container *parent, rf_drawable *child);

void
mrb_container_remove_child(mrb_state *mrb, rf_container *parent, rf_drawable *child);

void
mrb_container_refresh(mrb_state *mrb, rf_container *parent);

static inline void
mrb_container_invalidate(mrb_state *mrb, rf_container *container)
{
  container->dirty = TRUE;
}

#ifdef __cplusplus
}
#endif

#endif
