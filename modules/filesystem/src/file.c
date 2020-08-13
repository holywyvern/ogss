#include <mruby.h>
#include <mruby/error.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/variable.h>

#include <ogss/file.h>

#include <string.h>

#define E_EOFERROR mrb_exc_get(mrb, "EOFError")
#define E_FILE_ERROR mrb_exc_get(mrb, "FileError")

#define PATH_VAR mrb_intern_lit(mrb, "#path")

static void
file_free(mrb_state mrb, void *ptr)
{
  if (ptr)
  {
    mrb_file *file = ptr;
    mrb_file_close(file);
  }
}

const mrb_data_type mrb_file_data_type = {
  "File", file_free
};

static mrb_value
file_read_secure(mrb_state *mrb, mrb_value self)
{
  mrb_value file = mrb_ary_entry(self, 0);
  mrb_value block = mrb_ary_entry(self, 1);
  return mrb_funcall(mrb, block, "call", 1, file);
}

static mrb_value
file_initialize(mrb_state *mrb, mrb_value self)
{
  const char *name, *mode;
  mrb_value block = mrb_nil_value();
  mode = NULL;
  mrb_get_args(mrb, "z|z&", &name, &mode, &block);
  DATA_TYPE(self) = &mrb_file_data_type;
  if (!mode) mode = "r";
  if (strcmp(mode, "w") == 0 || strcmp(mode, "wb") == 0)
  {
    DATA_PTR(self) = mrb_file_open_write(mrb, name);
  }
  else if (strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0)
  {
    DATA_PTR(self) = mrb_file_open_read(mrb, name);
  }
  else if (strcmp(mode, "+") == 0 || strcmp(mode, "w+") == 0)
  {
    DATA_PTR(self) = mrb_file_open_append(mrb, name);
  }
  else
  {
    mrb_raisef(mrb, E_FILE_ERROR, "Unknown file mode: %s", mode);
  }
  if (mrb_test(block))
  {
    mrb_bool error;
    mrb_value items[] = {self, block};
    mrb_value array = mrb_ary_new_from_values(mrb, 2, items);
    mrb_value result = mrb_protect(mrb, file_read_secure, array, &error);
    mrb_file_close(DATA_PTR(self));
    DATA_PTR(self) = NULL;
    if (error)
    {
      mrb_exc_raise(mrb, result);
    }
  }
  mrb_iv_set(mrb, self, PATH_VAR, mrb_str_new_cstr(mrb, name));
  return self;
}

static mrb_value
file_close(mrb_state *mrb, mrb_value self)
{
  mrb_file_close(mrb_get_file(mrb, self));
  DATA_PTR(self) = NULL;
  return mrb_nil_value();
}

static mrb_value
file_eofQ(mrb_state *mrb, mrb_value self)
{
  mrb_bool eof = mrb_file_eof(mrb_get_file(mrb, self));
  return mrb_bool_value(eof);
}

static mrb_value
file_size(mrb_state *mrb, mrb_value self)
{
  size_t size = mrb_file_length(mrb_get_file(mrb, self));
  return mrb_fixnum_value((mrb_int)size);
}

static mrb_value
file_seek(mrb_state *mrb, mrb_value self)
{
  mrb_int pos;
  mrb_get_args(mrb, "i", &pos);
  mrb_file_seek(mrb_get_file(mrb, self), (size_t)pos);
  return self;
}

static mrb_value
file_tell(mrb_state *mrb, mrb_value self)
{
  size_t pos = mrb_file_tell(mrb_get_file(mrb, self));
  return mrb_fixnum_value((mrb_int)pos);
}

static mrb_value
file_getbyte(mrb_state *mrb, mrb_value self)
{
  mrb_file *f = mrb_get_file(mrb, self);
  unsigned char chr;
  if (mrb_file_eof(f)) return mrb_nil_value();
  mrb_file_read(f, 1, (char *)&chr);
  return mrb_fixnum_value(chr);
}

static mrb_value
file_getc(mrb_state *mrb, mrb_value self)
{
  mrb_file *f = mrb_get_file(mrb, self);
  char chr;
  if (mrb_file_eof(f)) return mrb_nil_value();
  mrb_file_read(f, 1, &chr);
  return mrb_str_new(mrb, &chr, 1);
}

static inline mrb_value
read_file_line(mrb_state *mrb, mrb_file *f, const char *sep, mrb_int limit, mrb_bool append_newline)
{
  if (limit < 0) limit = (mrb_int)mrb_file_length(f);
  if (!sep)
  {
    char *buffer = mrb_malloc(mrb, sizeof(char) * (limit + 1));
    buffer[limit] = '\0';
    size_t read = mrb_file_read(f, (size_t)limit, buffer);
    mrb_value str = mrb_str_new(mrb, buffer, read);
    mrb_free(mrb, buffer);
    return str;
  }
  mrb_int bytes = 0;
  mrb_value result = mrb_str_new_cstr(mrb, "");
  size_t sep_len = strlen(sep);
  char *buf = mrb_malloc(mrb, sizeof(char) * (sep_len + 1));
  buf[sep_len] = '\0';
  while (bytes < limit)
  {
    size_t read = mrb_file_read(f, sep_len, buf);
    bytes += read;
    if (strncmp(sep, buf, read) != 0)
    {
      result = mrb_str_cat(mrb, result, buf, read);
      if (mrb_file_eof(f)) break;
    }
    else
    {
      if (append_newline)
      {
        mrb_value expr = mrb_str_new_cstr(mrb, "((\r\n?)|\n)$");
        mrb_value regex = mrb_obj_new(mrb, mrb_class_get(mrb, "Regexp"), 1, &expr);
        mrb_value replace = mrb_str_new_cstr(mrb, sep);
        result = mrb_str_cat(mrb, result, buf, read);
        result = mrb_funcall(mrb, result, "gsub", 2, regex, replace);
      }
      break;
    }
  }
  mrb_free(mrb, buf);
  return result;
}

static mrb_value
file_reads(mrb_state *mrb, mrb_value self, mrb_bool include_newline)
{
  const char *sep;
  mrb_int limit;
  mrb_file *f = mrb_get_file(mrb, self);
  if (mrb_file_eof(f)) return mrb_nil_value();
  sep = "\n";
  limit = -1;
  switch (mrb_get_argc(mrb))
  {
    case 0: {
      sep = "\n";
      limit = -1;
      break;
    }
    case 1: {
      mrb_value value;
      mrb_get_args(mrb, "o", &value);
      if (mrb_nil_p(value))
      {
        sep = NULL;
      }
      else if (mrb_fixnum_p(value) || mrb_float_p(value))
      {
        limit = mrb_int(mrb, value);
      }
      else
      {
        sep = mrb_str_to_cstr(mrb, mrb_to_str(mrb, value));
      }
      break;
    }
    case 2: {
      mrb_value value;
      mrb_get_args(mrb, "zo", &sep, &value);
      if (mrb_fixnum_p(value) || mrb_float_p(value))
      {
        limit = mrb_int(mrb, value);
      }
      break;
    }
    default: {
      mrb_value value, *rest;
      mrb_int rest_len;
      mrb_get_args(mrb, "zio*", &sep, &limit, &value, &rest, &rest_len);
      break;
    }
  }
  return read_file_line(mrb, f, sep, limit, include_newline);
}

static mrb_value
file_gets(mrb_state *mrb, mrb_value self)
{
  return file_reads(mrb, self, TRUE);
}

static mrb_value
file_readline(mrb_state *mrb, mrb_value self)
{
  mrb_value result = file_reads(mrb, self, TRUE);
  if (mrb_nil_p(result)) mrb_raise(mrb, E_EOFERROR, "End of file reached");
  return result;
}

static mrb_value
file_s_existsQ(mrb_state *mrb, mrb_value self)
{
  const char *filename;
  mrb_get_args(mrb, "z", &filename);
  return mrb_bool_value(mrb_file_exists(filename));
}

static mrb_value
file_s_setup_write(mrb_state *mrb, mrb_value self)
{
  const char *org, *name;
  mrb_get_args(mrb, "zz", &org, &name);
  mrb_file_set_write(mrb, org, name);
  return mrb_nil_value();
}

static mrb_value
file_closedQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(DATA_PTR(self) == NULL);
}

static mrb_value
file_readableQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(mrb_file_get_mode(mrb_get_file(mrb, self)) == MRB_FILE_READ);
}

static mrb_value
file_writableQ(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(mrb_file_get_mode(mrb_get_file(mrb, self)) == MRB_FILE_WRITE);
}

static mrb_value
file_path(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, PATH_VAR);
}

static mrb_value
file_rewind(mrb_state *mrb, mrb_value self)
{
  mrb_file_seek(mrb_get_file(mrb, self), 0);
  return self;
}

static mrb_value
file_flush(mrb_state *mrb, mrb_value self)
{
  mrb_file_flush(mrb_get_file(mrb, self));
  return self;
}

static mrb_value
file_print(mrb_state *mrb, mrb_value self)
{
  mrb_value *args;
  mrb_int count;
  mrb_file *fp = mrb_get_file(mrb, self);
  mrb_get_args(mrb, "*", &args, &count);
  for (mrb_int i = 0; i < count; ++i)
  {
    mrb_value str = mrb_funcall(mrb, args[i], "to_s", 0);
    mrb_file_write(fp, RSTRING_LEN(str), RSTRING_PTR(str));
  }
  return mrb_nil_value();
}

static mrb_value
file_puts(mrb_state *mrb, mrb_value self)
{
  file_print(mrb, self);
  mrb_file_write(mrb_get_file(mrb, self), strlen("\n"), "\n");
  return mrb_nil_value();
}

static mrb_value
file_read(mrb_state *mrb, mrb_value self)
{
  mrb_int bytes = 0;
  mrb_value obj = mrb_nil_value();
  mrb_file *fp = mrb_get_file(mrb, self);
  mrb_get_args(mrb, "|iS", &bytes, &obj);
  if (bytes <= 0)
  {
    bytes = mrb_file_length(fp);
  }
  if (!mrb_string_p(obj))
  {
    obj = mrb_str_new_cstr(mrb, "");
  }
  char *buffer = mrb_malloc(mrb, sizeof(char) * bytes);
  size_t read = mrb_file_read(fp, bytes, buffer);
  mrb_funcall(mrb, obj, "clear", 0);
  return mrb_str_cat(mrb, obj, buffer, read);
}

void
mrb_init_file(mrb_state *mrb)
{
  struct RClass *error = mrb_define_class(mrb, "IOError", mrb->eStandardError_class);
  error = mrb_define_class(mrb, "FileError", error);
  mrb_define_class(mrb, "EOFError", error);
  mrb_define_class(mrb, "LoadError", error);
  struct RClass *file = mrb_define_class(mrb, "File", mrb->object_class);
  MRB_SET_INSTANCE_TT(file, MRB_TT_DATA);

  mrb_define_method(mrb, file, "initialize", file_initialize, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1)|MRB_ARGS_BLOCK());

  mrb_define_method(mrb, file, "close", file_close, MRB_ARGS_NONE());

  mrb_define_method(mrb, file, "closed?", file_closedQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "readable?", file_readableQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "writable?", file_writableQ, MRB_ARGS_NONE());

  mrb_define_method(mrb, file, "eof?", file_eofQ, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "eof", file_eofQ, MRB_ARGS_NONE());

  mrb_define_method(mrb, file, "pos", file_tell, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "pos=", file_seek, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, file, "position", file_tell, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "position=", file_seek, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, file, "rewind", file_rewind, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "flush", file_flush, MRB_ARGS_NONE());

  mrb_define_method(mrb, file, "size", file_size, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "length", file_size, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "count", file_size, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "seek", file_seek, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, file, "tell", file_tell, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "to_path", file_path, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "path", file_path, MRB_ARGS_NONE());

  mrb_define_method(mrb, file, "getbyte", file_getbyte, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "getc", file_getc, MRB_ARGS_NONE());
  mrb_define_method(mrb, file, "gets", file_gets, MRB_ARGS_OPT(2));
  mrb_define_method(mrb, file, "readline", file_readline, MRB_ARGS_OPT(2));
  mrb_define_method(mrb, file, "read", file_read, MRB_ARGS_OPT(2));

  mrb_define_method(mrb, file, "puts", file_puts, MRB_ARGS_ANY());
  mrb_define_method(mrb, file, "print", file_print, MRB_ARGS_ANY());
  mrb_define_method(mrb, file, "write", file_print, MRB_ARGS_ANY());

  mrb_define_class_method(mrb, file, "exist?", file_s_existsQ, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "exists?", file_s_existsQ, MRB_ARGS_REQ(1));

  mrb_define_class_method(mrb, file, "setup_write", file_s_setup_write, MRB_ARGS_REQ(2));
}
