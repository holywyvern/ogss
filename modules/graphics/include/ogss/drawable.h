#ifndef OGSS_DRAWABLE_H
#define OGSS_DRAWABLE_H 1

#include <mruby.h>
#include <mruby/data.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

#define E_DISPOSED_ERROR mrb_exc_get(mrb, "DisposedError")

typedef struct rf_drawable rf_drawable;
typedef struct rf_container rf_container;

typedef void (*rf_drawable_update_callback)(mrb_state *mrb, rf_drawable *obj);
typedef void (*rf_drawable_draw_callback)(rf_drawable *obj);

struct rf_drawable
{
  struct rf_container          *container;
  rf_drawable_update_callback   update;
  rf_drawable_draw_callback     draw;
  mrb_int                       z;
};

struct rf_container
{
  rf_drawable   base;
  rf_drawable **items;
  mrb_int       items_capa;
  mrb_int       items_size;
  mrb_bool      dirty;
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
mrb_container_update(mrb_state *mrb, rf_container *parent);

void
mrb_container_draw_children(rf_container *container);

static inline void
mrb_container_invalidate(mrb_state *mrb, rf_container *container)
{
  container->dirty = TRUE;
}

#ifdef __cplusplus
}
#endif

#endif
