#include <mruby.h>
#include <mruby/data.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/class.h>

#include <ogss/table.h>

#define HEAD_PACK mrb_str_new_cstr(mrb, "L5")
#define BODY_PACK mrb_str_new_cstr(mrb, "S")

static void
mrb_table_free(mrb_state *mrb, void *ptr)
{
  if (ptr)
  {
    rf_table *table = (rf_table *)ptr;
    mrb_free(mrb, table->elements);
    mrb_free(mrb, table);
  }
}

const struct mrb_data_type mrb_table_data_type = {
  "Table", mrb_table_free
};

static inline rf_table *
alloc_table(mrb_state *mrb, mrb_int length)
{
  rf_table *table = mrb_malloc(mrb, sizeof(*table));
  table->elements = mrb_malloc(mrb, length * sizeof(uint16_t));
  return table;
}

static inline uint16_t
table_get_value(rf_table *table, mrb_int x, mrb_int y, mrb_int z)
{
  if (x < 0 || x >= table->xsize || y < 0 || y >= table->ysize || z < 0 || z >= table->zsize)
  {
    return 0;
  }
  size_t i = x + y * table->xsize + z * table->xsize * table->ysize;
  return table->elements[i];
}

static inline void
table_set_value(rf_table *table, mrb_int x, mrb_int y, mrb_int z, mrb_int value)
{
  if (x < 0 || x >= table->xsize || y < 0 || y >= table->ysize || z < 0 || z >= table->zsize)
  {
    return;
  }
  if (value < 0)  value = 0;
  if (value > UINT16_MAX) value = UINT16_MAX;
  size_t i = x + y * table->xsize + z * table->xsize * table->ysize;
  table->elements[i] = (uint16_t)value;
}

static inline void
table_copy(rf_table *new_table, rf_table *old_table)
{
  for (mrb_int z = 0; z < new_table->zsize; ++z)
  {
    for (mrb_int y = 0; y < new_table->ysize; ++y)
    {
      for (mrb_int x = 0; x < new_table->xsize; ++x)
      {
        table_set_value(new_table, x, y, z, table_get_value(old_table, x, y, z));
      }
    }
  }
}

static mrb_value
mrb_table_initialize(mrb_state *mrb, mrb_value self)
{
  rf_table *table;
  mrb_int xsize, ysize, zsize;
  ysize = zsize = 1;
  mrb_get_args(mrb, "i|ii", &xsize, &ysize, &zsize);
  if (xsize < 1 || ysize < 1 || zsize < 1)
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Table size must be positive");
  }
  if (xsize > UINT32_MAX) xsize = UINT32_MAX;
  if (ysize > UINT32_MAX) ysize = UINT32_MAX;
  if (zsize > UINT32_MAX) zsize = UINT32_MAX;    
  mrb_int length = xsize * ysize * zsize;
  table = alloc_table(mrb, length);
  table->xsize = (uint32_t)xsize;
  table->ysize = (uint32_t)ysize;
  table->zsize = (uint32_t)zsize;
  for (mrb_int i = 0; i < length; ++i)
  {
    table->elements[i] = 0;
  }
  DATA_TYPE(self) = &mrb_table_data_type;
  DATA_PTR(self) = table;
  return mrb_nil_value();
}

static mrb_value
mrb_table_initialize_copy(mrb_state *mrb, mrb_value self)
{
  rf_table *table, *original;
  mrb_get_args("d", &original, &mrb_table_data_type);
  table = alloc_table(mrb, original->xsize * original->ysize * original->zsize);
  table->xsize = original->xsize;
  table->ysize = original->ysize;
  table->zsize = original->zsize;
  table_copy(table, original);
  DATA_TYPE(self) = &mrb_table_data_type;
  DATA_PTR(self) = table;  
  return self;
}

static mrb_value
mrb_table_xsize(mrb_state *mrb, mrb_value self)
{
  rf_table *table = mrb_get_table(mrb, self);
  return mrb_fixnum_value(table->xsize);
}

static mrb_value
mrb_table_ysize(mrb_state *mrb, mrb_value self)
{
  rf_table *table = mrb_get_table(mrb, self);
  return mrb_fixnum_value(table->ysize);
}

static mrb_value
mrb_table_zsize(mrb_state *mrb, mrb_value self)
{
  rf_table *table = mrb_get_table(mrb, self);
  return mrb_fixnum_value(table->zsize);
}

static mrb_value
mrb_table_get(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y, z;
  rf_table *table = mrb_get_table(mrb, self);
  y = 0;
  z = 0;
  mrb_get_args(mrb, "i|ii", &x, &y, &z);
  return mrb_fixnum_value(table_get_value(table, x, y, z));
}

static mrb_value
mrb_table_set(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y, z, value, a, b, c;
  rf_table *table = mrb_get_table(mrb, self);
  switch (mrb_get_args(mrb, "ii|ii", &x, &a, &b, &c))
  {
    case 2: {
      y = z = 0;
      value = a;
      break;
    }
    case 3: {
      y = a;
      z = 0;
      value = b;
      break;
    }
    case 4: {
      y = a;
      z = b;
      value = c;
      break;
    }
  }
  table_set_value(table, x, y, z, value);
  return mrb_fixnum_value(value);
}

static mrb_value
mrb_table_resize(mrb_state *mrb, mrb_value self)
{
  rf_table new_table;
  mrb_int xsize, ysize, zsize;
  rf_table *table = mrb_get_table(mrb, self);
  ysize = table->ysize;
  zsize = table->zsize;
  mrb_get_args(mrb, "i|ii", &xsize, &ysize, &zsize);
  if (xsize < 1 || ysize < 1 || zsize < 1)
  {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Table size must be positive");
  }
  if (xsize > UINT32_MAX) xsize = UINT32_MAX;
  if (ysize > UINT32_MAX) ysize = UINT32_MAX;
  if (zsize > UINT32_MAX) zsize = UINT32_MAX;
  new_table.xsize = (uint32_t)xsize;
  new_table.ysize = (uint32_t)ysize;
  new_table.zsize = (uint32_t)zsize;
  mrb_int length = new_table.xsize * new_table.ysize * new_table.zsize;
  new_table.elements = mrb_malloc(mrb, length * sizeof(uint16_t));
  table_copy(&new_table, table);
  mrb_free(mrb, table->elements);
  table->xsize = xsize;
  table->ysize = ysize;
  table->zsize = zsize;
  table->elements = new_table.elements;
  return mrb_nil_value();
}

static mrb_value
mrb_table_to_a(mrb_state *mrb, mrb_value self)
{
  rf_table *table = mrb_get_table(mrb, self);
  mrb_int length = table->xsize * table->ysize * table->zsize;
  mrb_value result = mrb_ary_new_capa(mrb, length);
  for (mrb_int i = 0; i < length; ++i)
  {
    mrb_ary_set(mrb, result, i, mrb_fixnum_value(table->elements[i]));
  }
  return result;
}

static mrb_value
mrb_table_dump(mrb_state *mrb, mrb_value self)
{
  rf_table *table = mrb_get_table(mrb, self);
  mrb_int length = table->xsize * table->ysize * table->zsize;
  mrb_value values[] = {
    mrb_fixnum_value(3),
    mrb_fixnum_value(table->xsize),
    mrb_fixnum_value(table->ysize),
    mrb_fixnum_value(table->zsize),
    mrb_fixnum_value(length),
  };
  mrb_value info = mrb_ary_new_from_values(mrb, 5, values);
  mrb_value data = mrb_ary_new_capa(mrb, length);
  for (mrb_int i = 0; i < length; ++i)
  {
    mrb_ary_set(mrb, data, i, mrb_fixnum_value(table->elements[i]));
  }
  mrb_value pack = mrb_funcall(mrb, BODY_PACK, "*", 1, mrb_fixnum_value(length));
  info = mrb_funcall(mrb, info, "pack", 1, HEAD_PACK);
  data = mrb_funcall(mrb, data, "pack", 1, pack);
  return mrb_str_append(mrb, info, data);
}

static mrb_value
mrb_table_load(mrb_state *mrb, mrb_value self)
{
  mrb_value data;
  mrb_get_args(mrb, "S", &data);
  mrb_value unpack = mrb_funcall(mrb, data, "unpack", 1, HEAD_PACK);
  mrb_value args[] = {
    mrb_ary_entry(unpack, 1),
    mrb_ary_entry(unpack, 2),
    mrb_ary_entry(unpack, 3)
  };
  mrb_int length = mrb_int(mrb, mrb_ary_entry(unpack, 4));
  mrb_value result = mrb_obj_new(mrb, mrb_class_ptr(self), 3, args);
  mrb_value pack = mrb_funcall(mrb, BODY_PACK, "*", 1, mrb_fixnum_value(length));
  mrb_value sub = mrb_str_substr(mrb, data, 20, RSTRING_LEN(data));
  mrb_value values = mrb_funcall(mrb, sub, "unpack", 1, pack);
  rf_table *table = mrb_get_table(mrb, result);
  for (mrb_int i = 0; i < length; ++i)
  {
    table->elements[i] = mrb_int(mrb, mrb_ary_entry(values, i));
  }
  return result;
}

void
mrb_init_table(mrb_state *mrb)
{
  struct RClass *table = mrb_define_class(mrb, "Table", mrb->object_class);
  MRB_SET_INSTANCE_TT(table, MRB_TT_DATA);

  mrb_define_method(mrb, table, "initialize", mrb_table_initialize, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(2));
  mrb_define_method(mrb, table, "initialize_copy", mrb_table_initialize_copy, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, table, "xsize", mrb_table_xsize, MRB_ARGS_NONE());
  mrb_define_method(mrb, table, "ysize", mrb_table_ysize, MRB_ARGS_NONE());
  mrb_define_method(mrb, table, "zsize", mrb_table_zsize, MRB_ARGS_NONE());

  mrb_define_method(mrb, table, "[]", mrb_table_get, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(2));
  mrb_define_method(mrb, table, "[]=", mrb_table_set, MRB_ARGS_REQ(2)|MRB_ARGS_OPT(2));

  mrb_define_method(mrb, table, "resize", mrb_table_resize, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(2));

  mrb_define_method(mrb, table, "to_a", mrb_table_to_a, MRB_ARGS_NONE());

  mrb_define_method(mrb, table, "_dump", mrb_table_dump, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, table, "_load", mrb_table_load, MRB_ARGS_REQ(1));
}
