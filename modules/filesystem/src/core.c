#include <ogss/file.h>

#include <mruby.h>
#include <mruby/variable.h>
#include <mruby/string.h>

#include <rayfork.h>
#include <physfs.h>

#define E_FILE_ERROR mrb_exc_get(mrb, "FileError")
#define PHYSFS_ERROR_STR PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())

struct mrb_file
{
  mrb_state         *mrb;
  PHYSFS_File       *file;
  enum mrb_file_mode mode;
};

static mrb_file *
alloc_file(mrb_state *mrb, const char *name, PHYSFS_File *fp)
{
  if (!fp)
  {
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to open file '%s' (%s)", name, PHYSFS_ERROR_STR);
  }
  mrb_file *file = (mrb_file *)mrb_malloc(mrb, sizeof *file);
  file->mrb = mrb;
  file->file = fp;
  return file;
}

mrb_file *
mrb_file_open_read(mrb_state *mrb, const char *name)
{
  PHYSFS_File *fp;
  fp = PHYSFS_openRead(name);
  mrb_file *file = alloc_file(mrb, name, fp);
  file->mode = MRB_FILE_READ;
  return file;
}

mrb_file *
mrb_file_open_write(mrb_state *mrb, const char *name)
{
  PHYSFS_File *fp;
  fp = PHYSFS_openWrite(name);
  mrb_file *file = alloc_file(mrb, name, fp);
  file->mode = MRB_FILE_WRITE;
  return file;
}

mrb_file *
mrb_file_open_append(mrb_state *mrb, const char *name)
{
  PHYSFS_File *fp;
  fp = PHYSFS_openAppend(name);
  mrb_file *file = alloc_file(mrb, name, fp);
  file->mode = MRB_FILE_WRITE;
  return file;
}

void
mrb_file_close(mrb_file *file)
{
  int code = PHYSFS_close(file->file);
  if (!code)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to close file (%s)", PHYSFS_ERROR_STR);
  }
  mrb_free(file->mrb, file);
}

size_t
mrb_file_read(mrb_file *file, size_t buffer_size, char *buffer)
{
  PHYSFS_sint64 read = PHYSFS_readBytes(file->file, buffer, buffer_size);
  if (read < 0)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to read file (%s)", PHYSFS_ERROR_STR);
  }
  return (size_t)read;
}

size_t
mrb_file_write(mrb_file *file, size_t buffer_size, char *buffer)
{
  PHYSFS_sint64 write = PHYSFS_writeBytes(file->file, buffer, buffer_size);
  if (write < (PHYSFS_sint64)buffer_size)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to write file (%s)", PHYSFS_ERROR_STR);
  }
  return buffer_size;
}

size_t
mrb_file_tell(mrb_file *file)
{
  PHYSFS_sint64 pos = PHYSFS_tell(file->file);
  if (pos == -1)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to get file position (%s)", PHYSFS_ERROR_STR);
  }
  return (size_t)pos;
}

size_t
mrb_file_length(mrb_file *file)
{
  PHYSFS_sint64 size = PHYSFS_fileLength(file->file);
  if (size == -1)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to get file size (%s)", PHYSFS_ERROR_STR);
  }
  return (size_t)size;
}

mrb_bool
mrb_file_eof(mrb_file *file)
{
  return PHYSFS_eof(file->file);
}

mrb_bool
mrb_file_exists(const char *filename)
{
  return PHYSFS_exists(filename);
}

void
mrb_file_seek(mrb_file *file, size_t position)
{
  int seek = PHYSFS_seek(file->file, position);
  if (!seek)
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Unable to change file position (%s)", PHYSFS_ERROR_STR);
  }
}

static int
rf_mrb_file_size(void* user_data, const char* filename)
{
  mrb_state *mrb = (mrb_state *)user_data;
  mrb_file *f = mrb_file_open_read(mrb, filename);
  int size = (int)mrb_file_length(f);
  mrb_file_close(f);
  return size;
}

static bool
rf_mrb_file_read(void* user_data, const char* filename, void* dst, int dst_size)
{
  mrb_state *mrb = (mrb_state *)user_data;
  mrb_file *f = mrb_file_open_read(mrb, filename);
  size_t read = mrb_file_read(f, dst_size, (char *)dst);
  mrb_file_close(f);
  return read > 0;
}

void
mrb_rf_file_callbacks(mrb_state *mrb, rf_io_callbacks *cb)
{
  cb->file_size_proc = rf_mrb_file_size;
  cb->read_file_proc = rf_mrb_file_read;
  cb->user_data = mrb;
}

void
mrb_file_set_write(mrb_state *mrb, const char *org, const char *name)
{
  const char *prefs = PHYSFS_getPrefDir(org, name);
  if (!PHYSFS_setWriteDir(prefs))
  {
    mrb_raisef(mrb, E_FILE_ERROR, "Failed to setup write directory (%s)", PHYSFS_ERROR_STR);
  }
  PHYSFS_mount(prefs, "@saves", 0);
}

enum mrb_file_mode
mrb_file_get_mode(mrb_file *file)
{
  return file->mode;
}

mrb_bool
mrb_file_mkdir(mrb_state *mrb, const char *name)
{
  return (mrb_bool)PHYSFS_mkdir(name);
}

mrb_bool
mrb_file_remove(mrb_state *mrb, const char *name)
{
  return (mrb_bool)PHYSFS_delete(name);
}

void
mrb_file_flush(mrb_file *file)
{
  if (!PHYSFS_flush(file->file))
  {
    mrb_state *mrb = file->mrb;
    mrb_raisef(mrb, E_FILE_ERROR, "Failed to flush file (%s)", PHYSFS_ERROR_STR);
  }
}

static PHYSFS_Allocator physfs_allocator;
static mrb_state *current_physfs_mrb;

void
mrb_init_file(mrb_state *mrb);

void
mrb_init_file_utils(mrb_state *mrb);

static void *
physfs_malloc(PHYSFS_uint64 size)
{
  return mrb_malloc(current_physfs_mrb, (size_t)size);
}

static void *
physfs_realloc(void *ptr, PHYSFS_uint64 size)
{
  return mrb_realloc(current_physfs_mrb, ptr, (size_t)size);
}

static void
physfs_free(void *ptr)
{
  mrb_free(current_physfs_mrb, ptr);
}

void
mrb_setup_filesystem(mrb_state *mrb)
{
  if (!PHYSFS_isInit())
  {
    int physfs_init = 1;
    mrb_value argv0 = mrb_gv_get(mrb, mrb_intern_lit(mrb, "$0"));
    current_physfs_mrb = mrb;
    physfs_allocator.Malloc = physfs_malloc;
    physfs_allocator.Realloc = physfs_realloc;
    physfs_allocator.Free = physfs_free;
    physfs_allocator.Init = NULL;
    physfs_allocator.Deinit = NULL;
    PHYSFS_setAllocator(&physfs_allocator);
    if (mrb_string_p(argv0))
    {
      const char *str = mrb_str_to_cstr(mrb, argv0);
      physfs_init = PHYSFS_init(str);
      PHYSFS_mount(str, NULL, 1);
    }
    else
    {
      physfs_init = PHYSFS_init(NULL);
    }
    if (!physfs_init)
    {
      mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to initialize file system (%s)", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
    PHYSFS_permitSymbolicLinks(1);
    PHYSFS_mount(PHYSFS_getBaseDir(), NULL, 1);
    mrb_file_set_write(mrb, "OGSS Games", "Default");
  }
}

void
mrb_close_filesystem(mrb_state *mrb)
{
  if (PHYSFS_isInit() && current_physfs_mrb == mrb)
  {
    if (!PHYSFS_deinit())
    {
      mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to deinitialize file system");
    }
  }
}