#ifndef ORGF_ALLOC_H
#define ORGF_ALLOC_H 1

#include <mruby.h>
#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

rf_allocator
mrb_get_allocator(mrb_state *mrb);

#ifdef __cplusplus
}
#endif

#endif
