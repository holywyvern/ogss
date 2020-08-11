#include <mruby.h>
#include <rayfork.h>

static void *
rf_mrb_alloc(void *mrb, int size)
{
  return mrb_malloc((mrb_state *)mrb, (size_t)size);
}

static void
rf_mrb_free(void *mrb, void *data)
{
  mrb_free((mrb_state *)mrb, data);
}

rf_allocator
mrb_get_allocator(mrb_state *mrb)
{
  rf_allocator alloc;
  alloc.user_data = mrb;
  alloc.alloc_proc = rf_mrb_alloc;
  alloc.free_proc = rf_mrb_free;
  return alloc;
}

void
mrb_ogss_core_gem_init(mrb_state *mrb)
{
}

void
mrb_ogss_core_gem_final(mrb_state *mrb)
{
}