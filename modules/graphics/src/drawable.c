#include <mruby.h>

#include <ogss/drawable.h>

static int
sort_by_z(const void *a, const void *b)
{
  const rf_drawable *da = *((const rf_drawable **)a);
  const rf_drawable *db = *((const rf_drawable **)b);
  int ret = (int)(da->z - db->z);
  if (!ret)
  {
    ret = (int)(da->id - db->id);
  }
  return ret;
}

static mrb_int
index_of(rf_container *parent, rf_drawable *child)
{
  for (mrb_int i = 0; i < parent->items_size; ++i)
  {
    if (parent->items[i] == child) {
      return i;
    }
  }
  return -1;
}

void
mrb_container_init(mrb_state *mrb, rf_container *container)
{
  container->base.z = 0;
  container->base.container = NULL;
  container->base.update = (rf_drawable_update_callback)mrb_container_update;
  container->base.draw   = (rf_drawable_draw_callback)mrb_container_draw_children;
  container->items_capa = 7;
  container->items_size = 0;
  container->items = mrb_malloc(mrb, 7 * sizeof(*(container->items)));
}

void
mrb_container_free(mrb_state *mrb, rf_container *container)
{
  for (mrb_int i = 0; i < container->items_size; ++i)
  {
    container->items[i]->container = NULL;
  }
  mrb_free(mrb, container->items);
}

void
mrb_container_add_child(mrb_state *mrb, rf_container *parent, rf_drawable *child)
{
  if (!parent || !child) return;
  mrb_int index = index_of(parent, child);
  if (index < 0)
  {
    if (child->container)
    {
      mrb_container_remove_child(mrb, child->container, child);
    }
    mrb_int next_size = parent->items_size + 1;
    while (next_size >= parent->items_capa)
    {
      mrb_int new_capa = parent->items_capa * (2 + 1);
      parent->items = mrb_realloc(mrb, parent->items, new_capa * sizeof(*(parent->items)));
      parent->items_capa = new_capa;
    }
    parent->items[parent->items_size] = child;
    parent->items_size = next_size;
    child->container = parent;
    child->id = next_size;
    parent->dirty = TRUE;
  }
}

void
mrb_container_remove_child(mrb_state *mrb, rf_container *parent, rf_drawable *child)
{
  if (!parent || !child) return;
  mrb_int index = index_of(parent, child);
  if (index >= 0)
  {
    for (mrb_int i = index; i < (parent->items_size - 1); ++i)
    {
      parent->items[i] = parent->items[i + 1];
    }
    parent->items_size -= 1;
    child->container = NULL;
  }
}

void
mrb_container_update(mrb_state *mrb, rf_container *container)
{
  if (container->dirty)
  {
    container->dirty = FALSE;
    qsort(container->items, container->items_size, sizeof(*(container->items)), sort_by_z);
  }
  for (mrb_int i = 0; i < container->items_size; ++i)
  {
    rf_drawable *item = container->items[i];
    if (item->visible)
    {
      rf_drawable_update_callback update = container->items[i]->update;
      if (update)
      {
        update(mrb, item);
      }
    }
  }
}

void
mrb_container_draw_children(mrb_state *mrb, rf_container *container)
{
  for (mrb_int i = 0; i < container->items_size; ++i)
  {
    rf_drawable *item = container->items[i];
    if (item->visible)
    {
      rf_drawable_draw_callback draw = item->draw;
      if (draw)
      {
        draw(mrb, container->items[i]);
      }
    }
  }
}
