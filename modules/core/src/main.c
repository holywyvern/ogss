#include <mruby.h>
#include <rayfork.h>

static void *
mrb_allocator_wrapper(struct rf_allocator *alloc, rf_source_location source_location, rf_allocator_mode mode, rf_allocator_args args)
{
  switch (mode)
  {
    case RF_AM_ALLOC:
    {
      return mrb_malloc(alloc->user_data, args.size_to_allocate_or_reallocate);
    }
    case RF_AM_FREE:
    {
      mrb_free(alloc->user_data, args.pointer_to_free_or_realloc);
      break;
    }
    case RF_AM_REALLOC:
    {
      return mrb_realloc(
        alloc->user_data, args.pointer_to_free_or_realloc, args.size_to_allocate_or_reallocate
      );
    }
    default: break;
  }
  return 0;
}

rf_allocator
mrb_get_allocator(mrb_state *mrb)
{
  rf_allocator alloc;
  alloc.user_data = mrb;
  alloc.allocator_proc = mrb_allocator_wrapper;
  return alloc;
}

void
mrb_orgf_core_gem_init(mrb_state *mrb)
{
}

void
mrb_orgf_core_gem_final(mrb_state *mrb)
{
}