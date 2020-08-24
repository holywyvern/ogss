#ifndef OGSS_TABLE_H
#define OGSS_TABLE_H 1

#include <mruby.h>
#include <mruby/data.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct mrb_data_type mrb_table_data_type;

typedef struct rf_table rf_table;

struct rf_table
{
  uint32_t xsize, ysize, zsize;
  uint16_t *elements;
};

static inline rf_table *
mrb_get_table(mrb_state *mrb, mrb_value obj)
{
  rf_table *ptr;
  Data_Get_Struct(mrb, obj, &mrb_table_data_type, ptr);
  return ptr;
}

static inline mrb_bool
mrb_table_p(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_table_data_type;
}

uint16_t
mrb_table_get_value(rf_table *table, mrb_int x, mrb_int y, mrb_int z);

#ifdef __cplusplus
}
#endif

#endif /* OGSS_TABLE_H */
