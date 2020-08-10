#include <mruby.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/object.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/data.h>
#include <mruby/error.h>

#include <ogss/file.h>

enum { MAJOR_VERSION = 4, MINOR_VERSION = 8 };

struct marshal_ctx
{
  enum
  {
    MARSHAL_FILE,
    MARSHAL_STRING,
  } type;
  union
  {
    mrb_value as_value;
    mrb_file *as_file;
    struct
    {
      size_t pos;
      size_t len;
      const char *ptr;
    } as_string;
  };
  mrb_value symbols;
};


static inline void
new_file_source(mrb_file *file, struct marshal_ctx *marshal)
{
  marshal->as_file = file;
  marshal->type = MARSHAL_FILE;
}

static inline void
new_string_source(size_t size, const char *ptr, struct marshal_ctx *marshal)
{
  marshal->as_string.len = size;
  marshal->as_string.ptr = ptr;
  marshal->as_string.pos = 0;
  marshal->type = MARSHAL_STRING;
}

static inline void
new_value_source(mrb_state *mrb, mrb_value str, struct marshal_ctx *marshal)
{
  marshal->as_value = str;
  marshal->type = MARSHAL_STRING;
}

static void
marshal_ctx_putc(mrb_state *mrb, struct marshal_ctx *src, char c)
{
  if (src->type = MARSHAL_FILE)
  {
    mrb_file_write(src->as_file, 1, &c);
    return;
  }
  src->as_value = mrb_str_cat(mrb, src->as_value, &c, 1);
}

static void
marshal_ctx_puts(mrb_state *mrb, struct marshal_ctx *src, char *buff, size_t buff_size)
{
  if (src->type = MARSHAL_FILE)
  {
    mrb_file_write(src->as_file, buff_size, buff);
    return;
  }
  src->as_value = mrb_str_cat(mrb, src->as_value, buff, buff_size);
}

static inline mrb_bool
is_struct(mrb_state *mrb, mrb_value v)
{
  return mrb_class_defined(mrb, "Struct") && mrb_obj_is_kind_of(mrb, v, mrb_class_get(mrb, "Struct"));
}

static inline mrb_bool
is_regexp(mrb_state *mrb, mrb_value v)
{
  return mrb_class_defined(mrb, "Regexp") && mrb_obj_is_kind_of(mrb, v, mrb_class_get(mrb, "Regexp"));
}

static void
dump_array(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_module(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_class(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_false(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_true(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_symbol(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_string(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_hash(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_data(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_struct(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_regexp(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_object(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_fixnum(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

static void
dump_float(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj);

#define TAG(c) marshal_ctx_putc(mrb, src, c)

static void
dump_marshal(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  switch (mrb_type(obj))
  {
    case MRB_TT_ARRAY: { dump_array(mrb, src, obj); break; }
    case MRB_TT_CLASS: { dump_class(mrb, src, obj); break; }
    case MRB_TT_MODULE: { dump_module(mrb, src, obj); break; }
    case MRB_TT_FALSE: { dump_false(mrb, src, obj); break; }
    case MRB_TT_TRUE: { dump_true(mrb, src, obj); break; }
    case MRB_TT_SYMBOL: { dump_symbol(mrb, src, obj); break; }
    case MRB_TT_STRING: { dump_string(mrb, src, obj); break; }
    case MRB_TT_HASH: { dump_hash(mrb, src, obj); break; }
    case MRB_TT_DATA: { dump_data(mrb, src, obj); break; }
    case MRB_TT_FIXNUM: { TAG('i'); dump_fixnum(mrb, src, obj); break; }
    case MRB_TT_FLOAT: { TAG('f'); dump_float(mrb, src, obj); break; }
    default:
    {
      if (is_struct(mrb, obj))
      {
        dump_struct(mrb, src, obj);
      }
      else if (is_regexp(mrb, obj))
      {
        dump_regexp(mrb, src, obj);
      }
      else
      {
        dump_object(mrb, src, obj);
      }
      break;
    }
  }
}

static void
dump_array(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  TAG('[');
  mrb_int len = RARRAY_LEN(obj);
  dump_fixnum(mrb, src, mrb_fixnum_value(len));
  for (mrb_int i = 0; i < len; ++i)
  {
    dump_marshal(mrb, src, mrb_ary_entry(obj, i));
  }
}

static void
dump_module(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
}

static void
dump_class(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
}

static void
dump_false(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  if (mrb_nil_p(obj))
  {
    TAG('0');
    return;
  }
  TAG('F');
}

static void
dump_true(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  TAG('T');
}

static void
dump_symbol(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  TAG(':');
  mrb_value str = mrb_to_str(mrb, mrb_funcall(mrb, obj, "to_s", 0));
  marshal_ctx_puts(mrb, src, RSTRING_PTR(str), RSTRING_LEN(str));
}

static void
dump_string(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  TAG('"');
  marshal_ctx_puts(mrb, src, RSTRING_PTR(obj), RSTRING_LEN(obj));
}

static void
dump_hash(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
}

static void
dump_data(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
}

static void
dump_struct(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
}

static void
dump_regexp(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
}

static void
dump_object(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
}

static void
dump_fixnum(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  mrb_int v = mrb_int(mrb, obj);
  if(v == 0)
  {
    signed char c = 0;
    marshal_ctx_putc(mrb, src, c);
    return;
  }
  if (0 < v && v < 123)
  {
    char c = (char)(v + 5);
    marshal_ctx_putc(mrb, src, c);
    return;
  }
  if(-124 < v && v < 0)
  {
    char c = (char)((v - 5) & 0xff);
    marshal_ctx_putc(mrb, src, c);
    return;
  }
  char buf[sizeof(mrb_int)];
  mrb_int x = v;
  size_t i = 1;
  for(; i <= sizeof(mrb_int); ++i) {
    buf[i] = x & 0xff;
    x = x < 0 ? ~((~x) >> 8) : (x >> 8);
    if(x ==  0) { buf[0] = (char) i; break; }
    if(x == -1) { buf[0] = (char)-i; break; }
  }
  marshal_ctx_puts(mrb, src, buf, sizeof(mrb_int));
}

static void
dump_float(mrb_state *mrb, struct marshal_ctx *src, mrb_value obj)
{
  mrb_value str = mrb_to_str(mrb, mrb_funcall(mrb, obj, "to_s", 0));
  marshal_ctx_puts(mrb, src, RSTRING_PTR(str), RSTRING_LEN(str));
}

static mrb_value
marshal_dump(mrb_state *mrb, mrb_value self)
{
  struct marshal_ctx src;
  mrb_value source, target;
  target = mrb_str_new_cstr(mrb, "");
  mrb_get_args(mrb, "o|o", &source, &target);
  if (mrb_is_file(target))
  {
    new_file_source(mrb_get_file(mrb, source), &src);
  }
  else
  {
    target = mrb_funcall(mrb, target, "to_s", 0);
    new_value_source(mrb, target, &src);
  }
  marshal_ctx_putc(mrb, &src, MAJOR_VERSION);
  marshal_ctx_putc(mrb, &src, MINOR_VERSION);
  dump_marshal(mrb, &src, source);
  return target;
}

static mrb_bool
marshal_eof(struct marshal_ctx *marshal)
{
  if (marshal->type = MARSHAL_FILE)
  {
    return mrb_file_eof(marshal->as_file);
  }
  return marshal->as_string.pos >= marshal->as_string.len;
}

static uint8_t
marshal_getc(struct marshal_ctx *marshal)
{
  if (marshal_eof(marshal))
  {
    return 0;
  }
  if (marshal->type = MARSHAL_FILE)
  {
    uint8_t byte;
    mrb_file_read(marshal->as_file, 1, (char *)&byte);
    return byte;
  }
  size_t pos = marshal->as_string.pos;
  marshal->as_string.pos++;
  return marshal->as_string.ptr[pos];
}

static size_t
marshal_gets(mrb_state *mrb, struct marshal_ctx *marshal, char *buffer, size_t size)
{
  if (marshal_eof(marshal))
  {
    return 0;
  }
  if (marshal->type = MARSHAL_FILE)
  {
    return mrb_file_read(marshal->as_file, size, buffer);
  }
  const char *ptr = &(marshal->as_string.ptr[marshal->as_string.pos]);
  size_t rest = marshal->as_string.len - marshal->as_string.pos;
  strncpy_s(buffer, size, ptr, rest);
  return min(rest, size);
}

static mrb_value
load_symbol(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_table_symbol(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_fixnum(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_float(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_iv(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_link(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_subclass(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_dumpable(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_marshalable(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_object(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_regexp(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_string(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_array(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_hash(mrb_state *mrb, struct marshal_ctx *src, mrb_bool has_default);

static mrb_value
load_struct(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_module(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_class(mrb_state *mrb, struct marshal_ctx *src);

static mrb_value
load_marshal(mrb_state *mrb, struct marshal_ctx *src)
{
  if (marshal_eof(src))
  {
    mrb_raise(mrb, E_RANGE_ERROR, "cannot load end of input");
  }
  uint8_t c = marshal_getc(src);
  switch (c)
  {
    case '0': return mrb_nil_value();
    case 'T': return mrb_true_value();
    case 'F': return mrb_false_value();
    case ':': return load_symbol(mrb, src);
    case ';': return load_table_symbol(mrb, src);
    case 'i': return load_fixnum(mrb, src);
    case 'f': return load_float(mrb, src);
    case 'I': return load_iv(mrb, src);
    case '@': return load_link(mrb, src);
    case 'C': return load_subclass(mrb, src);
    case 'u': return load_dumpable(mrb, src);
    case 'U': return load_marshalable(mrb, src);
    case 'o': return load_object(mrb, src);
    case '/': return load_regexp(mrb, src);
    case '"': return load_string(mrb, src);
    case '[': return load_array(mrb, src);
    case '{': return load_hash(mrb, src, FALSE);
    case '}': return load_hash(mrb, src, TRUE);
    case 'S': return load_struct(mrb, src);
    case 'm': return load_module(mrb, src); 
    case 'c': return load_class(mrb, src);
    default:
    {
      char str[] = {c, '\0'};
      mrb_raisef(mrb, E_TYPE_ERROR, "Unsupported marshal format '%s'", str);
    }
  }
  return mrb_nil_value();
}

static mrb_value
load_symbol(mrb_state *mrb, struct marshal_ctx *src)
{
  return mrb_str_intern(mrb, load_string(mrb, src));
}

static mrb_value
load_table_symbol(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_int id = mrb_int(mrb, load_fixnum(mrb, src));
  mrb_assert(id < RARRAY_LEN(src->symbols));
  return mrb_symbol_value(mrb_symbol(RARRAY_PTR(src->symbols)[id]));
}

static mrb_value
load_fixnum(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_int c = (mrb_int)marshal_getc(src);
  if (c > 0)
  {
    if (4 < c && c < 128)
    {
      return mrb_fixnum_value(c - 5);
    }
    if (c > (mrb_int)sizeof(mrb_int))
    {
      mrb_raise(mrb, E_TYPE_ERROR, "Number is too big");
    }
    mrb_int ret = 0;
    for(mrb_int i = 0; i < c; ++i) {
      ret |= ((mrb_int)marshal_getc(src)) << (8 * i);
    }
    return mrb_fixnum_value(ret);
  }
  if (c < 0)
  {
    if(-129 < c && c < -4)
    {
      return mrb_fixnum_value(c + 5);
    }
    mrb_int len = -c;
    if (len > (mrb_int)sizeof(mrb_int))
    {
      mrb_raise(mrb, E_TYPE_ERROR, "Number is too big");
    }
    mrb_int ret = ~0;
    for(mrb_int i = 0; i < len; ++i) {
      ret &= ~(0xff << (8 * i));
      ret |= (mrb_int)(marshal_getc(src)) << (8 * i);
    }
    return mrb_fixnum_value(ret);
  }
  return mrb_fixnum_value(0);
}

static mrb_value
load_float(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_value str = load_string(mrb, src);
  return mrb_Float(mrb, str);
}

static void
load_properties(mrb_state *mrb, struct marshal_ctx *src, mrb_value result)
{
  mrb_int len = mrb_int(mrb, load_fixnum(mrb, src));
  int arena = mrb_gc_arena_save(mrb);
  for(mrb_int i = 0; i < len; ++i)
  {
    mrb_value symbol = load_marshal(mrb, src);
    if (!mrb_symbol_p(symbol))
    {
      mrb_raise(mrb, E_TYPE_ERROR, "Value is not a symbol");
    }
    mrb_value str = mrb_funcall(mrb, symbol, "to_s", 0);
    if (RSTRING_LEN(str) == 1 && RSTRING_PTR(str)[0] == 'E')
    {
      load_marshal(mrb, src); // ignore encoding
    }
    mrb_iv_set(mrb, result, mrb_symbol(symbol), load_marshal(mrb, src));
  }
  mrb_gc_arena_restore(mrb, arena);
}

static mrb_value
load_iv(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_value result = load_marshal(mrb, src);
  load_properties(mrb, src, result);
  return result;
}

static mrb_value
load_link(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_raise(mrb, E_TYPE_ERROR, "Marshal links are not supported yet");
  return mrb_nil_value();
}

static mrb_value
load_subclass(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_raise(mrb, E_TYPE_ERROR, "Subclasses are not supported yet");
  return mrb_nil_value();
}

static struct RClass *
load_class_struct(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_value class_value = load_marshal(mrb, src);
  if (!mrb_symbol_p(class_value))
  {
    mrb_raise(mrb, E_TYPE_ERROR, "Class name is not a symbol");
  }
  mrb_value str = mrb_funcall(mrb, class_value, "to_s", 0);
  return mrb_class_get(mrb, mrb_str_to_cstr(mrb, str));
}

static mrb_value
load_dumpable(mrb_state *mrb, struct marshal_ctx *src)
{
  struct RClass *klass = load_class_struct(mrb, src);
  return mrb_funcall(mrb, mrb_obj_value(klass), "_load", 1, load_string(mrb, src));
}

static mrb_value
load_marshalable(mrb_state *mrb, struct marshal_ctx *src)
{
  struct RClass *klass = load_class_struct(mrb, src);
  mrb_value data = load_marshal(mrb, src);
  mrb_value obj = mrb_obj_value(mrb_obj_alloc(mrb, MRB_INSTANCE_TT(klass), klass));
  return mrb_funcall(mrb, obj, "marshal_load", 1, data);
}

static mrb_value
load_object(mrb_state *mrb, struct marshal_ctx *src)
{
  struct RClass *klass = load_class_struct(mrb, src);
  mrb_value result = mrb_obj_value(mrb_obj_alloc(mrb, MRB_INSTANCE_TT(klass), klass));
  load_properties(mrb, src, result);
  return result;
}

static mrb_value
load_regexp(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_value args[] = { load_string(mrb, src), mrb_fixnum_value(marshal_getc(src)) };
  return mrb_obj_new(mrb, mrb_class_get(mrb, "Regexp"), 2, args);
}

static mrb_value
load_bytes(mrb_state *mrb, struct marshal_ctx *src, size_t len)
{
  mrb_value str = mrb_str_new_capa(mrb, len);
  size_t new_len = marshal_gets(mrb, src, RSTRING_PTR(str), RSTRING_CAPA(str));
  return mrb_str_new(mrb, RSTRING_PTR(str), RSTRING_LEN(str));
}

static mrb_value
load_string(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_int len = mrb_int(mrb, load_fixnum(mrb, src));
  return load_bytes(mrb, src, len);
}

static mrb_value
load_array(mrb_state *mrb, struct marshal_ctx *src)
{
  mrb_int len = mrb_int(mrb, load_fixnum(mrb, src));
  mrb_value array = mrb_ary_new_capa(mrb, len);
  for (mrb_int i = 0; i < len; ++i)
  {
    int arena = mrb_gc_arena_save(mrb);
    mrb_value value = load_marshal(mrb, src);
    mrb_ary_set(mrb, array, i, value);
    mrb_gc_arena_restore(mrb, arena);
  }
  return array;
}

static mrb_value
load_hash(mrb_state *mrb, struct marshal_ctx *src, mrb_bool has_default)
{
  mrb_int len = mrb_int(mrb, load_fixnum(mrb, src));
  mrb_value hash = mrb_hash_new_capa(mrb, len);
  for (mrb_int i = 0; i < len; ++i)
  {
    int arena = mrb_gc_arena_save(mrb);
    mrb_value k = load_marshal(mrb, src);
    mrb_value v = load_marshal(mrb, src);
    mrb_hash_set(mrb, hash, k, v);
    mrb_gc_arena_restore(mrb, arena);
  }
  if (has_default)
  {
    int arena = mrb_gc_arena_save(mrb);
    mrb_funcall(mrb, hash, "default=", 1, load_marshal(mrb, src));
    mrb_gc_arena_restore(mrb, arena);
  }
  return hash;
}


static mrb_value
load_struct(mrb_state *mrb, struct marshal_ctx *src)
{
  struct RClass *klass = load_class_struct(mrb, src);
  mrb_int member_count = mrb_int(mrb, load_fixnum(mrb, src));
  mrb_value struct_symbols = mrb_iv_get(mrb, mrb_obj_value(klass), mrb_intern_lit(mrb, "__members__"));
  mrb_check_type(mrb, struct_symbols, MRB_TT_ARRAY);
  if (member_count != RARRAY_LEN(struct_symbols))
  {
    mrb_raisef(mrb, E_TYPE_ERROR, "%s is not compatible, size differs", mrb_class_name(mrb, klass));
  }
  mrb_value values = mrb_ary_new_capa(mrb, member_count);
  for (mrb_int i = 0; i < member_count; ++i)
  {
    int arena = mrb_gc_arena_save(mrb);
    mrb_value symbol = load_marshal(mrb, src);
    if (!mrb_obj_eq(mrb, symbol, mrb_ary_entry(struct_symbols, i)))
    {
      mrb_raisef(mrb, E_TYPE_ERROR, "Struct member doesn't match");
    }
    mrb_ary_push(mrb, values, load_marshal(mrb, src));
    mrb_gc_arena_restore(mrb, arena);
  }
  return mrb_obj_new(mrb, klass, member_count, RARRAY_PTR(values));
}

static mrb_value
load_module(mrb_state *mrb, struct marshal_ctx *src)
{
  return mrb_obj_value(mrb_module_get(mrb, mrb_str_to_cstr(mrb, load_string(mrb, src))));
}

static mrb_value
load_class(mrb_state *mrb, struct marshal_ctx *src)
{
  return mrb_obj_value(mrb_class_get(mrb, mrb_str_to_cstr(mrb, load_string(mrb, src))));
}

static mrb_value
marshal_load(mrb_state *mrb, mrb_value self)
{
  struct marshal_ctx src;
  mrb_value source;
  mrb_get_args(mrb, "o", &source);
  if (mrb_is_file(source))
  {
    new_file_source(mrb_get_file(mrb, source), &src);
  }
  else
  {
    source = mrb_funcall(mrb, source, "to_s", 0);
    new_string_source(RSTRING_LEN(source), RSTRING_PTR(source), &src);
  }
  uint8_t major = marshal_getc(&src);
  uint8_t minor = marshal_getc(&src);
  if (major != MAJOR_VERSION || minor > MINOR_VERSION)
  {
    mrb_raisef(mrb, E_TYPE_ERROR, "Invalid marshal version (%d.%d) expected (%d.%d)",
      (mrb_int)major, (mrb_int)minor, (mrb_int)MAJOR_VERSION, (mrb_int)MINOR_VERSION
    );
  }
  return load_marshal(mrb, &src);
}

void
mrb_ogss_marshal_gem_init(mrb_state *mrb)
{
  struct RClass *marshal = mrb_define_module(mrb, "Marshal");
  mrb_define_module_function(mrb, marshal, "dump", marshal_dump, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));
  mrb_define_module_function(mrb, marshal, "load", marshal_load, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, marshal, "restore", marshal_load, MRB_ARGS_REQ(1));

  mrb_define_const(mrb, marshal, "MAJOR_VERSION", mrb_fixnum_value(MAJOR_VERSION));
  mrb_define_const(mrb, marshal, "MINOR_VERSION", mrb_fixnum_value(MINOR_VERSION));
}

void
mrb_ogss_marshal_gem_final(mrb_state *mrb)
{
}
