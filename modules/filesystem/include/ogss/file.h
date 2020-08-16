#ifndef OGSS_FILE_H
#define OGSS_FILE_H 1

#include <mruby.h>
#include <mruby/data.h>

#include <rayfork.h>

#ifdef __cplusplus
extern "C" {
#endif

#define E_LOAD_ERROR mrb_exc_get(mrb, "LoadError")

enum mrb_file_mode
{
  MRB_FILE_READ,
  MRB_FILE_WRITE,
};

typedef struct mrb_file mrb_file;

extern const mrb_data_type mrb_file_data_type;

void
mrb_setup_filesystem(mrb_state *mrb);

void
mrb_close_filesystem(mrb_state *mrb);

mrb_file *
mrb_file_open_write(mrb_state *mrb, const char *name);

mrb_file *
mrb_file_open_append(mrb_state *mrb, const char *name);

mrb_file *
mrb_file_open_read(mrb_state *mrb, const char *name);

mrb_file *
mrb_file_open_read_with_extensions(mrb_state *mrb, const char *name, const char **extensions);

void
mrb_file_close(mrb_file *file);

enum mrb_file_mode
mrb_file_get_mode(mrb_file *file);

size_t
mrb_file_read(mrb_file *file, size_t buffer_size, char *buffer);

size_t
mrb_file_write(mrb_file *file, size_t buffer_size, char *buffer);

size_t
mrb_file_tell(mrb_file *file);

size_t
mrb_file_length(mrb_file *file);

mrb_bool
mrb_file_eof(mrb_file *file);

void
mrb_file_flush(mrb_file *file);

mrb_bool
mrb_file_exists(const char *filename);

mrb_bool
mrb_file_is_file(const char *filename);

mrb_bool
mrb_file_is_folder(const char *filename);

mrb_bool
mrb_file_exists_with_extensions(mrb_state *mrb, const char *filename, const char **extensions);

void
mrb_file_seek(mrb_file *file, size_t position);

void
mrb_file_set_write(mrb_state *mrb, const char *org, const char *name);

rf_io_callbacks
mrb_get_io_callbacks(mrb_state *mrb);

rf_io_callbacks
mrb_get_io_callbacks_for_extensions(mrb_state *mrb, const char **extensions);

mrb_bool
mrb_file_mkdir(mrb_state *mrb, const char *name);

mrb_bool
mrb_file_remove(mrb_state *mrb, const char *name);

static inline mrb_file *
mrb_get_file(mrb_state *mrb, mrb_value obj)
{
  mrb_file *file;
  Data_Get_Struct(mrb, obj, &mrb_file_data_type, file);
  if (!file)
  {
    mrb_raise(mrb, mrb_exc_get(mrb, "FileError"), "File closed");
  }
  return file;
}

static inline mrb_bool
mrb_is_file(mrb_value obj)
{
  return mrb_data_p(obj) && DATA_TYPE(obj) == &mrb_file_data_type;
}

#ifdef __cplusplus
}
#endif

#endif
